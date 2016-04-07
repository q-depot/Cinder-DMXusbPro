#include "E131Client.h"
#include "cinder/app/App.h"

using namespace soso;
using namespace std;
using asio::ip::udp;

E131Client::E131Client(string ipAddress) : ipAddress(ipAddress) {

  for (int i = 0; i < 512; i++) {
    sac_packet[125 + i] = 0x00;
  }

  setSourceName("Sosolimited device");
  setCid();
  setLength();
  setPriority(200); // Default top priority
  setUniverse(1);

  // Set default data send interval to be equivalent to 30 Hz
  if (framerate > 0) {
    dataSendInterval = 1.0f / framerate;
  }

  // Create a timer for timing cycling and data sending
  timer = clock();

  connectUDP();
}

void E131Client::connectUDP() {

  if (!udpSetup) {

    auto app = cinder::app::AppBase::get();
    auto asioAddress = asio::ip::address_v4::from_string(ipAddress.c_str());
    auto endpoint = udp::endpoint(asioAddress, destPort);

    asio::error_code errCode;

    _socket = make_shared<udp::socket>(app->io_service());
    _socket->connect(endpoint, errCode);

    if (!errCode) {
      cout << "Connected to socket!" << endl;
      udpSetup = true;
    } else {
      cout << "Error connceting to socket, error code: " << errCode << endl;
    }
  }
}

void E131Client::setChannel(int channel, int value, int universe) {

  setUniverse(universe);

  // Cast value to char
  char val = char(value);

  if ((channel > 0) && (channel < 512)) {

    sac_packet[126 + channel] = val;

  } else {
    // bad
  }
}

void E131Client::setUniverse(int universe) {

  // Set header with appropriate universe, high and low byte
  sac_packet[113] = universe >> 8;
  sac_packet[114] = universe;
}

void E131Client::setPriority(int priority) {

  if ((priority > 0) && (priority <= 200)) {
    sac_packet[108] = char(priority);
  } else {
    cout << "Priority must be between 1-200" << endl;
  }
}

// This should remain the same per physical device
void E131Client::setCid() {

  // https://www.uuidgenerator.net/
  // 71 2d fb 4c-e6 a3-4f eb-aa 99-e5 94 84 b4 5b 1d
  sac_packet[22] = 0x71;
  sac_packet[23] = 0x2d;
  sac_packet[24] = 0xfb;
  sac_packet[25] = 0x4c;
  sac_packet[26] = 0xe6;
  sac_packet[27] = 0xa3;
  sac_packet[28] = 0x4f;
  sac_packet[29] = 0xeb;
  sac_packet[30] = 0xaa;
  sac_packet[31] = 0x99;
  sac_packet[32] = 0xe5;
  sac_packet[33] = 0x94;
  sac_packet[34] = 0x84;
  sac_packet[35] = 0xb4;
  sac_packet[36] = 0x5b;
  sac_packet[37] = 0x1d;
}

// UTF-8 encoded string, null terminated
void E131Client::setSourceName(std::string name) {

  char *cstr = new char[name.length() + 1];
  std::strcpy(cstr, name.c_str());

  int max_length = strlen(cstr);

  // field is 64 bytes, with a null terminator
  if (max_length > 63) {
    max_length = 63;
  };

  for (int i = 0; i < max_length; i++) {
    sac_packet[44 + i] = cstr[i];
  }
  sac_packet[107] = '\n';
}

void E131Client::setLength() {

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
  sac_packet[39] = 0x58; // different length!

  val = 0x020B;                       // Index 637-114 = 523 (0x020B)
  lowByte = 0xff & val;               // Get the lower byte
  highByte = (0x7 << 4) | (val >> 8); // bitshift so 0x7 is in the top 4 bits

  // Set length for DMP Layer
  sac_packet[115] = highByte;
  sac_packet[116] = 0xb;
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

  //	s.send_to(asio::buffer(request, request_length), endpoint);

  _socket->send(asio::buffer(sac_packet, packet_length));
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
  }

  sequenceNum++;
  sac_packet[111] = sequenceNum;
  timer = clock();
}
