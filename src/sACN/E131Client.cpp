#include "E131Client.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"

using namespace dmx;
using namespace std;
using asio::ip::udp;

E131Client::E131Client(string ipAddress) : ipAddress(ipAddress) {

  for (int i = 0; i < 512; i++) {
    sac_packet[125 + i] = 0x00;
  }

  // Universally Unique Identifier generated at:  https://www.uuidgenerator.net/
  const vector<char> cid = {char(0x71), char(0x2d), char(0xfb), char(0x4c),
                            char(0xe6), char(0xa3), char(0x4f), char(0xeb),
                            char(0xaa), char(0x99), char(0xe5), char(0x94),
                            char(0x84), char(0xb4), char(0x5b), char(0x1d)};

  setSourceName("Sosolimited device");
  setCid(cid);
  setLengthFlags();
  setPriority(200); // Default top priority
  setUniverse(1);

  // Set default data send interval to be equivalent to 30 Hz
  if (framerate > 0) {
    dataSendInterval = 1.0f / framerate;
  }

  // Create a timer for timing cycling and data sending
  timer = clock();

  // Connect to UDP endpoint
  connectUDP();
}

void E131Client::connectUDP() {

  if (!udpSetup) {

    auto app = cinder::app::AppBase::get();

    try {
      auto asioAddress = asio::ip::address_v4::from_string(ipAddress.c_str());
      auto endpoint = udp::endpoint(asioAddress, destPort);

      asio::error_code errCode;

      _socket = make_shared<udp::socket>(app->io_service());
      _socket->connect(endpoint, errCode);

      if (!errCode) {
        CI_LOG_I("Connected to socket!");
        udpSetup = true;
        _isConnected = true;
      } else {
        CI_LOG_E("Error connceting to socket, error code: " << errCode);
      }

    } catch (std::exception &e) {

      CI_LOG_E("Exception resolving endpoint " + ipAddress);
    }
  }
}

void E131Client::setChannel(int channel, int value, int universe) {

  setUniverse(universe);

  if ((channel > 0) && (channel < 512)) {
    sac_packet[126 + channel] = char(value);

  } else {
    CI_LOG_E("Channel must be between 1 and 512 for DMX protocol.");
  }
}

void E131Client::setUniverse(int universe) {

  _universe = universe;

  if (universeSequenceNum.count(_universe) == 0) {
    universeSequenceNum[_universe] = 0;
  }

  // Set header with appropriate universe, high and low byte
  sac_packet[113] = universe >> 8;
  sac_packet[114] = universe;
}

void E131Client::setPriority(int priority) {

  if ((priority >= 0) && (priority <= 200)) {
    sac_packet[108] = char(priority);
  } else {
    CI_LOG_W("Priority must be between 0-200");
  }
}

// This should remain the same per physical device
void E131Client::setCid(const std::vector<char> cid) {

  int length = cid.size();

  if (length != 16) {
    CI_LOG_W("CID must be of length 16!");
    return;
  }

  int start_index = 22;
  for (int i = 0; i < 16; i++) {
    sac_packet[start_index + i] = cid[i];
  }
}

// UTF-8 encoded string, null terminated
void E131Client::setSourceName(std::string name) {

  char *cstr = new char[name.length() + 1];
  std::strcpy(cstr, name.c_str());

  int max_length = strlen(cstr);

  // Field is 64 bytes, with a null terminator
  if (max_length > 63) {
    max_length = 63;
  };

  for (int i = 0; i < max_length; i++) {
    sac_packet[44 + i] = cstr[i];
  }
  sac_packet[107] = '\n';
}

void E131Client::setLengthFlags() {

  // 16-bit field with the PDU (Protocol Data Unit) length
  // encoded in the lower 12 bits
  // and 0x7 encoded in the top 4 bits

  // There are 3 PDUs in each packet (Root layer, Framing Layer, and DMP layer)

  // To calculate PDU length for RLP section, subtract last index in DMP layer
  // (637 for full payload) from the index before RLP length flag (15)

  // Set length for
  short val = 0x026e;        // Index 637-15 = 622 (0x026e)
  char lowByte = 0xff & val; // Get the lower byte
  char highByte =
      (0x7 << 4) | (val >> 8); // bitshift so 0x7 is in the top 4 bits

  // Set length for Root Layer (RLP)
  sac_packet[16] = highByte;
  sac_packet[17] = lowByte;

  val = 0x0258;                       // Index 637-37 = 600 (0x0258)
  lowByte = 0xff & val;               // Get the lower byte
  highByte = (0x7 << 4) | (val >> 8); // bitshift so 0x7 is in the top 4 bits

  // Set length for Framing Layer
  sac_packet[38] = highByte;
  sac_packet[39] = lowByte; // different length!

  val = 0x020B;                       // Index 637-114 = 523 (0x020B)
  lowByte = 0xff & val;               // Get the lower byte
  highByte = (0x7 << 4) | (val >> 8); // bitshift so 0x7 is in the top 4 bits

  // Set length for DMP Layer
  sac_packet[115] = highByte;
  sac_packet[116] = lowByte;
}

// Set our data sending framerate
void E131Client::setFramerate(float iFPS) {

  if (iFPS > 0) {
    dataSendInterval = 1.0 / iFPS;
  }
  framerate = iFPS;
}

// Check if we should send data, according to our
// data framerate
bool E131Client::shouldSendData(float iTime) {

  if (!useFramerate) {
    return true;
  } else {

    float diff = iTime - lastDataSendTime;

    if ((diff >= dataSendInterval) && (framerate > 0)) {
      return true;
    }
  }
  return false;
}

void E131Client::sendDMX() {

  _socket->send(asio::buffer(sac_packet, packet_length));
  universeSequenceNum[_universe] = universeSequenceNum[_universe] + 1;

  // Increment current universe counter.
}

// Update all strands, send data if it's time
// Update any test cycles
void E131Client::update() {

  // Use system time for timing purposes
  clock_t thisTime = clock();
  float timeDiff = (thisTime - timer) / 100000.0f;

  elapsedTime += timeDiff;

  float iTime = elapsedTime;

  // If we've enabled auto data sending, send data if
  // enough time has passed according to our data send interval
  if (autoSendingEnabled && shouldSendData(iTime)) {
    lastDataSendTime = iTime;
    sendDMX();

    sac_packet[111] = universeSequenceNum[_universe];
  }

  timer = clock();
}
