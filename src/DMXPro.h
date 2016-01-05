/*
 *  DMXPro.h
 *
 *  Created by Andrea Cuius
 *  The MIT License (MIT)
 *  Copyright (c) 2014 Nocte Studio Ltd.
 *
 *  www.nocte.co.uk
 *
 */

 ///
 /// View device specifications here: https://www.enttec.com/docs/dmx_usb_pro_api_spec.pdf
 /// Heavily modified from Andrea Cuius' implementation
 ///

#pragma once

#include "cinder/Thread.h"
#include "cinder/Serial.h"

const auto DMXPRO_START_MSG   = 0x7E;  // Start of message delimiter
const auto DMXPRO_END_MSG     = 0xE7;  // End of message delimiter
const auto DMXPRO_SEND_LABEL  = 6;     // Output Only Send DMX Packet Request
const auto DMXPRO_BAUD_RATE   = 57600; // virtual COM doesn't control the usb, this is just a dummy value
const auto DMXPRO_FRAME_RATE  = 35;    // dmx send frame rate
const auto DMXPRO_START_CODE  = 0;

// TODO: test this alarming claim:
//////////////////////////////////////////////////////////
// LAST 4 dmx channels seem not to be working, 508-511 !!!
//////////////////////////////////////////////////////////

/// Buffer to simplify building up data for DMX transmission.
class DMXColorBuffer {
public:
	DMXColorBuffer();
	/// Set an individual channel value.
	void setValue(uint8_t value, size_t channel);
	/// Set a color value across three channels.
	void setValue(const ci::Color8u &color, size_t channel);

	const uint8_t* data() const { return _data.data(); }
	size_t size() const { return _data.size(); }
private:
	std::array<uint8_t, 512> _data;
};

class DMXPro;
using DMXProRef = std::shared_ptr<DMXPro>;

///
/// RAII-style controller for a DMXPro box.
///
class DMXPro {

public:
	DMXPro();
	~DMXPro();

	DMXPro(const DMXPro&) = delete;
	DMXPro& operator=(const DMXPro&) = delete;

	/// Connect to a serial device.
	bool connect(const std::string &deviceName);
	/// Disconnect from serial device.
	void closeConnection();

	/// Send data to DMX device. Called repeatedly from a separate thread. Threadsafe.
	void writeData();

	/// Buffer all message data to be sent on the next DMX update. Threadsafe.
	void bufferData(const std::vector<uint8_t> &data);
	void bufferData(const uint8_t *data, size_t size);
	void bufferData(const DMXColorBuffer &buffer) { bufferData(buffer.data(), buffer.size()); }
	/// Fill the data buffer with a single value. Threadsafe.
	void fillBuffer(uint8_t value);
	/// Set an individual channel value using 1-indexed positions.
	[[deprecated("This method can result in incomplete data being sent over the wire. Use bufferData with a DMXColorBuffer instead.")]]
	void	setValue(uint8_t value, int channel);

	std::string  getDeviceName() { return mSerialDeviceName; }
	/// Start update loop (called automatically by connect).
	void startLoop();
	/// Stop update loop.
	void stopLoop();

private:
	std::vector<uint8_t>	mBody;
	std::mutex						mBodyMutex;
	std::chrono::high_resolution_clock::duration mTargetFrameTime;
	ci::SerialRef         mSerial;
	std::string           mSerialDeviceName;
	std::thread           mSendDataThread;
	std::atomic_bool      mRunSendDataThread;

	void initSerial();
	/// Write data in a loop (runs on a secondary thread).
	void dataSendLoop();
};

#pragma mark - Convenience functions

inline void printSerialDevicesToConsole() {
	const std::vector<ci::Serial::Device> &devices( ci::Serial::getDevices(true) );

	ci::app::console() << "--- DMX usb pro > List serial devices ---" << std::endl;
	for (auto &device : devices) {
		ci::app::console() << device.getName() << std::endl;
	}

	ci::app::console() << "-----------------------------------------" << std::endl;
}

inline std::vector<std::string> listSerialDevicePaths()
{
	std::vector<std::string> devicesList;

	const std::vector<ci::Serial::Device> &devices( ci::Serial::getDevices(true) );

	for(auto &device : devices) {
		devicesList.push_back( device.getPath() );
	}

	return devicesList;
}
