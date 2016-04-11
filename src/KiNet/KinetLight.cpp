//
//  KinetLight.cpp
//  ckLightsExample
//
//  Created by Alex Olivier on 11/17/14.
//
// A class for Philips ColorKinetics lighting products
// This class represents an individual light on a ColorKinetics LMX Flex
// lighting strand
// This object stores color information

#include "KinetLight.h"

using namespace kinet;
using namespace std;
using namespace cinder;

// Create a light at the given index
// TODO: Provide some range error handling
KinetLight::KinetLight(int index) : index(index) {}

KinetLight::~KinetLight() {}

// Set color to be sent to the node (0-255)
void KinetLight::setColor(float iR, float iG, float iB) {
  color = vec3(iR, iG, iB);
}

// Set color
void KinetLight::setColor(vec3 iColor) { color = iColor; }

// Get color
vec3 KinetLight::getColor() { return color; }

// Get node index
int KinetLight::getIndex() { return index; }
