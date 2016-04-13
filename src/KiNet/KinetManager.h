//
//  KinetManager.h
//  ckLightsExample
//
//  Created by Alex Olivier on 11/17/14.
//
//

#pragma once

#include "KinetPowerSupply.h"
#include <ctime>

namespace kinet {
class KinetManager {

public:
  KinetManager();
  ~KinetManager();

	std::shared_ptr<KinetPowerSupply> addPowerSupply(std::string iIPAddress);
	std::shared_ptr<KinetPowerSupply> getPowerSupply(std::string iIPAddress);

	std::vector<std::shared_ptr<KinetPowerSupply>> getPowerSupplies();

  void update();
  int sendKinet();

  void turnOffLights();
	void setColor(ci::vec3 iColor);
	void setColor(ci::vec3 color, std::string iAddress, int iPort = -1, int iLight = -1);

  void enableAutoSending(bool iEnable); // Whether or not data is automatically
  // sent each update frame
  void
  setFramerate(float iFPS); // Data sending framerate, by default this is 30 fps
  void setUseFramerate(bool iEnable) { useFramerate = iEnable; }

protected:

  bool shouldSendData(float iTimeSinceDataSent);

protected:
	std::vector<std::shared_ptr<KinetPowerSupply>> powerSupplies;
	std::vector<std::shared_ptr<KinetStrand>> allStrands;
	std::vector<std::shared_ptr<KinetLight>> allLights;

  // Framerate-lock sending or send as fast as we can
  bool useFramerate = false;
  bool autoSendingEnabled = true;

  float dataSendInterval = 0;
  float lastDataSendTime = 0;
  float framerate = 30.0f;

  float timeSinceDataSent = 0;
  clock_t timer;

  float elapsedTime = 0;
};
}