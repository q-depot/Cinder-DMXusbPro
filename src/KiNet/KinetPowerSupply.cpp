//
//  KinetPowerSupply.cpp
//  ckLightsExample
//
//  Created by Alex Olivier on 11/17/14.
//
//

// A class for Philips ColorKinetics lighting products
// This class represents a ColorKinetics power supply
// and managers all network handling

// The power supply class uses ofxUDPManager to connect to
// the proper destination port for the Kinet protocol

// If enabled, power supplies send sync packets after to
// update all ports simultaneously

// The manager class holds all references to KinetPowerSupply objects

#include "KinetPowerSupply.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"

using namespace std;
using asio::ip::udp;
using namespace kinet;

int KinetPowerSupply::powerSupplyNum = 3;

// Create a power supply object with the given ip address
// Create a udpManager object
KinetPowerSupply::KinetPowerSupply(std::string ipAddress)
    : ipAddress(ipAddress), _udpSocket(nullptr) {

  baseAddress = 1000 * powerSupplyNum;
  powerSupplyNum++;

  connectUDP();
}

KinetPowerSupply::~KinetPowerSupply() {}

void KinetPowerSupply::connectUDP() {

  if (!udpSetup) {

    auto app = cinder::app::AppBase::get();

    try {
      auto asioAddress = asio::ip::address_v4::from_string(ipAddress.c_str());
      auto endpoint = udp::endpoint(asioAddress, destPort);

      asio::error_code errCode;

      _udpSocket = make_shared<udp::socket>(app->io_service());
      _udpSocket->connect(endpoint, errCode);

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

// Create and add new KinetStrand object on a port
// NOTE: Ports are indexed starting at 1
shared_ptr<KinetStrand> KinetPowerSupply::addStrand(int iPort,
                                                    int iStrandSize) {

  auto existingStrand = getStrand(iPort);

  // If a strand already exists on this port, return it
  if (existingStrand) {
    return existingStrand;
  }

  // Otherwise, make a new strand
  shared_ptr<KinetStrand> s(new KinetStrand(_udpSocket, iPort, iStrandSize));

  strands.push_back(s);

  return s;
}

// Get a pointer to a KinetStrand object on the given port
// TODO: Should we leave error handling up to user?
shared_ptr<KinetStrand> KinetPowerSupply::getStrand(int iPort) {

  for (auto s : strands) {

    if (s->getPort() == iPort) {

      return s;
    }
  }

  shared_ptr<KinetStrand> null_ps;

  return null_ps;
}

vector<shared_ptr<KinetStrand>> KinetPowerSupply::getStrands() {
  return strands;
}

// Call send kinet on all KinetStrands!
void KinetPowerSupply::sendKinet() {

  if (!loggedFailure) {

    for (auto strand : strands) {
      strand->sendKinet();
    }

    if (udpSetup) {

      // Sending sync packet
      _udpSocket->send(asio::buffer(sync_packet, sync_packet.size()));
    }
  }
}

// Return ip address
std::string KinetPowerSupply::getIPAddress() { return ipAddress; }
