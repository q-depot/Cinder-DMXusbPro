//
//  KinetPowerSupply.h
//  ckLightsExample
//
//  Created by Alex Olivier on 11/17/14.
//
//

#pragma once

#include "KinetStrand.h"
#include "asio/asio.hpp"

namespace kinet {

class KinetPowerSupply {

public:
  KinetPowerSupply(std::string ipAddress);
  ~KinetPowerSupply();

  void connectUDP();

  std::shared_ptr<KinetStrand> addStrand(int iPort,
                                         int iStrandSize = DEFAULT_STRAND_SIZE);
  std::shared_ptr<KinetStrand> getStrand(int iPort);
  std::vector<std::shared_ptr<KinetStrand>> getStrands();

  void sendKinet();

  std::string getIPAddress();

protected:
  std::vector<std::shared_ptr<KinetStrand>> strands;
	std::shared_ptr<asio::ip::udp::socket> udpSocket;

  std::string ipAddress;

  bool loggedFailure = false;
	bool udpSetup = false;
	bool _isConnected = false;
	
	std::shared_ptr<asio::ip::udp::socket> _udpSocket;

  int baseAddress;
  int destPort = 6038; // All KiNet packets must use this as destination port

  static int powerSupplyNum;

  // Data packet for power supply sync
  // Sent after all KiNet strand packets are sent, if sync is enabled
  // Let's us ensure that we apply new data to all strands simultaneously
  std::array<char, 16> sync_packet = {
      {0x04, 0x01, char(220), char(74), // magic number
       0x02, 0x00,                      // KiNet version
       0x09, 0x01,                      // Packet type, SYNC
       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
};
}
