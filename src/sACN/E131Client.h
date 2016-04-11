//
//  E131Client.h
//
//  Created by Alex Olivier on 04/05/16.
//
//

#pragma once

#include "asio/asio.hpp"

namespace dmx {

class E131Client {
public:
  E131Client(std::string ipAddress);

  void setChannel(int channel, int value, int universe = 1);

  void update();
  void enableAutoSending(bool iEnable); // Whether or not data is automatically
  // sent each update frame
  void
  setFramerate(float iFPS); // Data sending framerate, by default this is 30 fps

  void setUseFramerate(bool iEnable) { useFramerate = iEnable; }

  void setPriority(int priority);
  void setCid(const std::vector<char> cid);
  void setSourceName(std::string name);
	bool isConnected(){ return _isConnected; }

private:
  void connectUDP();
  void sendDMX();
  void setUniverse(int universe);
  void setLengthFlags();

  bool shouldSendData(float iTimeSinceDataSent);

  // BIG ENDIAN
  char sac_packet[638] = {
      // ROOT LAYER (RLP)
      0x00, 0x10,                   // Define RLP Preamble Size
      0x00, 0x00,                   // RLP Post amble size
      0x41, 0x53, 0x43, 0x2d, 0x45, // E131 Packet identifier
      0x31, 0x2e, 0x31, 0x37, 0x00, 0x00, 0x00, 0x00,
      0x00, // FILL OUT, low 12 bits = PDU length
      // High 4 bits = 0x7
      0x00, 0x00, 0x00, 0x04, // Identifies RLP Data as 1.31 Protocol PDU
      0x00, 0x00, 0x00, 0x00, // CID, Sender's unique CID
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      // FRAMING LAYER
      0x00, 0x00,             // Low 12 bits = PDU length, High 4 bits=07
      0x00, 0x00, 0x00, 0x02, // Identifies 1.31 data as DMP protocol PDU

      0x00, 0x00, 0x00, 0x00, // User assigned name of source
      0x00, 0x00, 0x00, 0x00, // UTF-8 string, null-terminated
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      0x64,       // DATA PRIORITY, 0-200, default of 100 (0x64)
      0x00, 0x00, // RESERVED, transmitters send 0, receivers ignore
      0x00,       // Sequence number to detect dupliate or out of order packets
      0x00, // Options flag, bit 7 = preview data, bit 6 = stream terminated
      0x00, 0x00, // UNIVERSE number

      // DMP Layer
      0x00, 0x00, // Protocol flags + length, Low 12 bits = PDU length
      // High 4 bits = 0x7
      0x02,                    // Identifies DMP set property message PDU
      static_cast<char>(0xA1), // Identifies format of address and data
      0x00, 0x00, // Indicates DMX_start.  Code is at DMP address 0.
      0x00, 0x01, // Indicates each property is 1 octet / byte
      0x02, 0x01, // Indicates 1+the number of slots in packet
      0x00,       // DMX start code (0 is standard)
      char(512)   // DMX payload (all 512 channels)
  };

  int packet_length = 638; // Length when all 512 DMX channels are sent
  int destPort = 5568;     // Default port for sACN protocol!
  int priority = 100;

  // Framerate-lock sending or send as fast as we can
  bool useFramerate = false;
  bool autoSendingEnabled = true;

  float dataSendInterval = 0;
  float lastDataSendTime = 0;
  float framerate = 30.0f;

  bool _isConnected = false;

  float timeSinceDataSent = 0;
  clock_t timer;

  float elapsedTime = 0;
  bool udpSetup = false;

  std::string ipAddress;
  std::shared_ptr<asio::ip::udp::socket> _socket;

  int _universe = 1;
  std::map<int, char> universeSequenceNum;
};
}
