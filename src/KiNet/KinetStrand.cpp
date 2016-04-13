//
//  KinetStrand.cpp
//  ckLightsExample
//
//  Created by Alex Olivier on 11/17/14.
//
//

// A class for Philips ColorKinetics lighting products
// This class represents a ColorKinetics LMX Flex lighting strand

// The ckLMXClass sends packets to update all colors in the strand

// By default, KinetStrand is initialized to use 50 lights
// Custom light strands can have anywhere from 5-60 lights
// Pass in iStrandSize to adjust number of nodes per strand

#include "KinetStrand.h"
#include "cinder/Log.h"

using namespace std;
using namespace kinet;
using asio::ip::udp;
using namespace ci;

// Initialize a KinetStrand with a given port and number of nodes
KinetStrand::KinetStrand(shared_ptr<asio::ip::udp::socket> udpSocket, int iPort,
                         int iStrandSize)
    : _udpSocket(udpSocket) {

  // CHECK if port is between 1-8!
  // Multistrand Power Supplies support up to 8 ports
  try {
    if ((iPort < 1) || (iPort > 8)) {
      throw range_error("CKLMX Port must be from 1-8!");
    }
  } catch (exception &e) {
    CI_LOG_E("ERROR: " << e.what());
  }

  // Set port number and UDP Manager
  port = iPort;

  // Initialize payload portion of data packet char[] to all 0's
  // This way, if we don't create a light object, we don't send that light any
  // data
  for (int i = 24; i < kinet_total.size(); i++) {
    kinet_total.at(i) = 0;
  }

  setLMXPort(iPort);

  try {
    if ((iStrandSize < 0) || (iStrandSize > 60)) {
      throw range_error("Strand size must be from 0-60!");
    }
  } catch (exception &e) {
    CI_LOG_E("ERROR: " << e.what());
  }

  setStrandSize(iStrandSize);
}

KinetStrand::~KinetStrand() {}

// Store the port in our Kinet packet
void KinetStrand::setLMXPort(int iPort) {

  // Set port number in data packet
  kinet_total.at(16) = char(iPort);
}

// Recalculate data packet length based on number of nodes in the strand
void KinetStrand::setStrandSize(int iNumNodes) {

  // Calculate data packet length based on number of nodes
  // 3 channels per node
  payloadLength = 3 * iNumNodes;

  // Set data packet length in data packet
  kinet_total.at(20) = payloadLength;
}

// Send our Kinet packet data
void KinetStrand::sendKinet() {

  // For all created light objects..
  for (auto l : lights) {

    // Get color from light object
    vec3 color = l->getColor();

    // Get node index from light object
    int i = l->getIndex();

    // TODO: Error handling for bad index

    // Calculate payload index of current light
    // 24 is the payload's offset in the data packet
    int j = 24 + i * 3;

    // Set RGB values for this light index
    // in the data packet
    kinet_total.at(j) = color.x;
    kinet_total.at(j + 1) = color.y;
    kinet_total.at(j + 2) = color.z;
  }

  // Calculate total data length
  // (Extra data in kinet_total is ignored)
  //  int dataLength = 24 + payloadLength;

  _udpSocket->send(asio::buffer(kinet_total, kinet_total.size()));
}

// Create and add a new light object
// THIS IS 0-indexed!
shared_ptr<KinetLight> KinetStrand::addLight(int iIndex) {

  auto existingLight = getLight(iIndex);

  if (existingLight) {
    return existingLight;
  }

  try {
    if ((iIndex < 0) || (iIndex > 59)) {
      throw range_error(
          "Kinet light index must be from 1-60! Cannot add light at index " +
          to_string(iIndex));
    }
    shared_ptr<KinetLight> l(new KinetLight(iIndex));
    lights.push_back(l);

    return l;
  } catch (exception &e) {
    CI_LOG_E("ERROR: " << e.what());
  }

  return nullptr;
}

// Get a created light object
// TODO: Do we just let the user perform error-handling for null lights?
shared_ptr<KinetLight> KinetStrand::getLight(int iIndex) {

  for (auto l : lights) {

    if (l->getIndex() == iIndex) {

      return l;
    }
  }

  shared_ptr<KinetLight> null_light;

  return null_light;
}

// Get all lights
vector<shared_ptr<KinetLight>> KinetStrand::getLights() { return lights; }

// Apply one color to all lights in strand
// Mostly for testing purposes
void KinetStrand::setColor(float iR, float iG, float iB) {

  for (auto l : lights) {

    l->setColor(iR, iG, iB);
  }
}

void KinetStrand::setColor(vec3 iColor) {

  setColor(iColor.x, iColor.y, iColor.z);
}
