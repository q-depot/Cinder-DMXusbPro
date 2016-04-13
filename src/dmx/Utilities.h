//
//  Utilities.h
//
//  Created by Soso Limited.
//
//

#pragma once

#include "cinder/Serial.h"
namespace dmx {

/// Buffer to simplify building up color data for DMX transmission.
/// Contains 512 channels of data.
class ColorBuffer {
public:
	ColorBuffer();
	/// Set an individual channel value. Zero indexed.
	void setChannelValue(size_t channel, uint8_t value);
	/// Set a color value. Colors occupy three channels, starting with the provided channel. Zero indexed.
	void setChannelValues(size_t channel, const ci::Color8u &color);
	void setChannelValues(size_t channel, ci::ivec3 color);

	const uint8_t* data() const { return _data.data(); }
	size_t         size() const { return _data.size(); }
	/// Returns the last safe index for assignment of data occupying \step bytes.
	size_t         safeIndex(size_t step) const { return std::floor<size_t>(_data.size() / (double)step) * step - step; }
	/// Returns the last usable contiguous color index.
	size_t         lastColorIndex() const { return safeIndex(3); }

private:
	std::array<uint8_t, 512> _data;
};

} // namespace dmx
