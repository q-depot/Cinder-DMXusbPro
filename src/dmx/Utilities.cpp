//
//  Utilities.cpp
//
//  Created by Soso Limited.
//
//

#include "Utilities.h"
#include "cinder/Log.h"

using namespace dmx;

ColorBuffer::ColorBuffer()
{
	_data.fill(0);
}

void ColorBuffer::setValue(uint8_t value, size_t channel) {
	channel = std::min(channel, _data.size() - 1);
	_data[channel] = value;
}

void ColorBuffer::setValue(const ci::Color8u &color, size_t channel) {
	channel = std::min(channel, _data.size() - 3);
	_data[channel] = color.r;
	_data[channel + 1] = color.g;
	_data[channel + 2] = color.b;
}
