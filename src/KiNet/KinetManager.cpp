//
//  KinetManager.cpp
//  ckLightsExample
//
//  Created by Alex Olivier on 11/17/14.
//
//

// A manager class for Philips ColorKinetics lighting products
// The manager class can perform high-level testing functions
// such as cycling all lights or strands

// The manager class holds all references to KinetPowerSupply objects

#include "KinetManager.h"

using namespace std;
using namespace kinet;
using namespace ci;

KinetManager::KinetManager() {
	
	// Set default data send interval to be equivalent to 30 Hz
	if (framerate > 0) {
		dataSendInterval = 1.0f / framerate;
	}
	
	// Create a timer for timing cycling and data sending
	timer = clock();
}

KinetManager::~KinetManager() {}

// Create and addd a new power supply with a given IP Address
// IP Address should be valid
// TODO: Error handling?
shared_ptr<KinetPowerSupply> KinetManager::addPowerSupply(std::string iIPAddress) {
	
	auto existingSuppy = getPowerSupply(iIPAddress);
	
	// Check if a powersupply already exists with this ipAddress
	if (existingSuppy) {
		return existingSuppy;
	}
	// Otherwise, make a new shared pointer to the power supply
	shared_ptr<KinetPowerSupply> ps(new KinetPowerSupply(iIPAddress));
	
	powerSupplies.push_back(ps);
	
	return ps;
}

// Get a power supply, if one has been created with the given IP Address
// TODO: Error handling?  Do we leave this up to the user to check if it's
// valid?
shared_ptr<KinetPowerSupply> KinetManager::getPowerSupply(std::string iIPAddress) {
	
	for (auto ps : powerSupplies) {
		
		if (ps->getIPAddress() == iIPAddress) {
			
			return ps;
		}
	}
	
	shared_ptr<KinetPowerSupply> null_ps;
	
	return null_ps;
}

// Get a vector of all power supplies
vector<shared_ptr<KinetPowerSupply>> KinetManager::getPowerSupplies() {
	
	return powerSupplies;
}

// Update all strands, send data if it's time
// Update any test cycles
void KinetManager::update() {
	
	// Use system time for timing purposes
	clock_t thisTime = clock();
	float timeDiff = (thisTime - timer) / 100000.0f;
	
	elapsedTime += timeDiff;
	
	float iTime = elapsedTime;
	
	// If we've enabled auto data sending, send data if
	// enough time has passed according to our data send interval
	if (autoSendingEnabled && shouldSendData(iTime)) {
		lastDataSendTime = iTime;
		sendKinet();
	}
	
	timer = clock();
}

// Set our data sending framerate
void KinetManager::setFramerate(float iFPS) {
	
	if (iFPS > 0) {
		
		dataSendInterval = 1.0 / iFPS;
	}
	
	framerate = iFPS;
}

// Check if we should send data, according to our
// data framerate
bool KinetManager::shouldSendData(float iTime) {
	
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

// Send kinet data for all power supplies
// This is a cascading function -- sendKinet() is called on all
// KinetPowerSupply objects, which in turn call sendKinet()
// on all ckLMXStrand objects
int KinetManager::sendKinet() {
	
	for (auto ps : powerSupplies) {
		
		ps->sendKinet();
	}
	
	return 1; // TODO: Associate this value with sending success
}


// Set color for all lights
void KinetManager::setColor(vec3 iColor) {
	
	for (auto ps : powerSupplies) {
		
		for (auto s : ps->getStrands()) {
			
			s->setColor(iColor);
		}
	}
}

// Set color for lights
// This can be as specific as an individual light, or it can be used to light up
// a whole strand
void KinetManager::setColor(vec3 iColor, string iAddress, int iPort,
												 int iLight) {
	
	for (auto ps : powerSupplies) {
		
		if (ps->getIPAddress() == iAddress) {
			
			for (auto s : ps->getStrands()) {
				
				if ((s->getPort() == iPort) || (iPort == -1)) {
					
					for (auto l : s->getLights()) {
						
						if ((l->getIndex() == iLight) || (iLight == -1)) {
							
							l->setColor(iColor);
						}
					}
				}
			}
		}
	}
}

