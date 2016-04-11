//
//  KinetLight.h
//  ckLightsExample
//
//  Created by Alex Olivier on 11/17/14.
//
//

#pragma once

namespace kinet {

class KinetLight {

public:
  KinetLight(int iIndex);
  ~KinetLight();

  /// Set KiNet Color value
  void setColor(float iR, float iG, float iB);
  void setColor(ci::vec3 iColor);

  /// Get KiNet color value
  ci::vec3 getColor();

  /// Get KiNet Node index
  int getIndex();

protected:
  /// Color to be sent to ckLight
  ci::vec3 color;

  /// CK Node index
  int index;
};
}