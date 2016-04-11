//
//  KinetStrand.h
//  ckLightsExample
//
//  Created by Alex Olivier on 11/17/14.
//
//

#pragma once

#include "KinetLight.h"
#include "asio/asio.hpp"

namespace kinet {

class KinetStrand {

#define DEFAULT_STRAND_SIZE 60

public:
  /// Create a KinetStrand object on a given port and with a given strand size
  /// Default port is 1, default strand size is 50
  KinetStrand(std::shared_ptr<asio::ip::udp::socket> udpSocket, int port = 1,
              int iStrandSize = DEFAULT_STRAND_SIZE);
  ~KinetStrand();

  /// Send KiNet data for this strand, using UDP
  void sendKinet();

  /// Add light at the specified index
  // TODO: Provide a way to add a light with an automatically generated index
  std::shared_ptr<KinetLight> addLight(int iIndex);
  std::shared_ptr<KinetLight> getLight(int iIndex);
  std::vector<std::shared_ptr<KinetLight>> getLights();

  /// Get strand port number
  int getPort() { return port; }

  /// Set KiNet light color
  void setColor(float iR, float iG, float iB);
  void setColor(ci::vec3 iColor);

protected:
  /// Reference to UDP manager
  std::shared_ptr<asio::ip::udp::socket> _udpSocket;

  /// All LMX Lights for this strand
  std::vector<std::shared_ptr<KinetLight>> lights;

  int port;

  // These headers are provided for reference from the Arduino sketch
  /*
         unsigned char kinet_header[12] = {
         0x04, 0x01, 0xDC, 0x4A,       // magic number
         0x02, 0x00,                   // KiNet version
         0x08, 0x01,                   // Packet type
         0x00, 0x00, 0x00, 0x00};      // Unused, sequence number

         unsigned char kinet_PORT_header[12] = {
         0xFF, 0xFF, 0xFF, 0xFF,       // universe, FFFF FFFF is "don't care"
         0x01,                         // Port on device controller
         0x00,                         // pad, unused
         0x01, 0x00,                   // Flags
         0x96, 0x00,                   // Set length, setting to 150 (send all
     values)
         0x00, 0x00};                  // start code
         */

  // This is the v2 kinet header and v2 kinet port header

  // Total KiNet data packet.  Includes header information and an empty payload
  // for up to 60 nodes.  Unused node data will be ignored during data sending.
  std::array<char, 204> kinet_total = {
      {0x04, 0x01, char(220), char(74), // Magic number
       0x02, 0x00,                      // KiNet version (1 or 2)
       0x08, 0x01,                      // Packet type
       0x00, 0x00, 0x00, 0x00, char(255), char(255), char(255),
       char(255), // universe, FFFF FFFF is "don't care"
       0x01, // Port on device controller -- TODO: port will need to be set when
       // using sPDS-480-24V
       0x00,       // pad, unused
       0x04, 0x00, // Flags, originally 0x01,0x00, 04 00 for requiring a sync
       // packet after sending data
       char(180), 0x00, // Set length of payload, Defaulting to 180 (max payload
                        // size for 60-node strands)
       // We modify this value based on number of nodes in the strand
       0x00, 0x00}};
	// LMX starts at index 24

  /// Set Port on LMX power supply (1-8)
  void setLMXPort(int iPort);

  /// Set number of nodes on CK strand (max for custom strands is 60, regularly
  /// manufactured strands are 50 nodes)
  void setStrandSize(int iNumNodes);

protected:
  int payloadLength =
      180; // Default is maximum data packet size (60 nodes * 3 channels)
	int packetLength = 204;
};
}