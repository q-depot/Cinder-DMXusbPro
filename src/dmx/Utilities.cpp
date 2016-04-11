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

void ColorBuffer::setChannelValue(size_t channel, uint8_t value) {
	channel = std::min(channel, _data.size() - 1);
	_data.at(channel) = value;
}

void ColorBuffer::setChannelValues(size_t channel, const ci::Color8u &color) {
	channel = std::min(channel, _data.size() - 3);
	_data.at(channel) = color.r;
	_data.at(channel + 1) = color.g;
	_data.at(channel + 2) = color.b;
}

void ColorBuffer::setChannelValues(size_t channel, ci::ivec3 color) {
	channel = std::min(channel, _data.size() - 3);
	color = glm::clamp(color, ci::ivec3(0), ci::ivec3(255));
	_data.at(channel) = color.r;
	_data.at(channel + 1) = color.g;
	_data.at(channel + 2) = color.b;
}
