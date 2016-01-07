//
// EnttecDevice.h
//
// Created by Soso Limited.
// Hardware specifications:
// API:        https://www.enttec.com/docs/dmx_usb_pro_api_spec.pdf
// Mk2 manual: https://d2lsjit0ao211e.cloudfront.net/pdf/manuals/70314.pdf
// Based on Andrea Cuius' DMXUSBPro
//
//

#pragma once

#include "cinder/Thread.h"
#include "cinder/Serial.h"

namespace dmx {

///
/// RAII-style controller for a EnttecDevice box.
/// Only streams values out to the box. Not used for configuring the DMX hardware.
///
class EnttecDevice {

public:
	/// Construct a DMX controller that connects to device and sends data at given FPS.
	/// Device can run between 1 and 40 FPS.
	explicit EnttecDevice(const std::string &device_name, int device_fps = 30);
	~EnttecDevice();

	// Remove copy constructor explicitly to get better error messages (member mutex is non-copyable).
	EnttecDevice(const EnttecDevice&) = delete;
	EnttecDevice& operator=(const EnttecDevice&) = delete;

	/// Connect to a serial device. Returns true on successful connection.
	bool connect(const std::string &device_name);
	bool isConnected() const { return _serial != nullptr; }
	/// Disconnect from serial device.
	void closeConnection();

	/// Send data to DMX device. Called repeatedly from a separate thread. Threadsafe.
	void writeData();

	/// Buffer all message data to be sent on the next DMX update. Threadsafe.
	void bufferData(const uint8_t *data, size_t size);
	template <typename Buffer>
	void bufferData(const Buffer &buffer) { bufferData(buffer.data(), buffer.size()); }
	/// Fill the data buffer with a single value. Threadsafe.
	void fillBuffer(uint8_t value);

	/// Return the name of the connected device. If not connected, returns an empty string.
	std::string  getDeviceName() { return _serial ? _serial->getDevice().getName() : ""; }
	/// Start update loop (called automatically by connect).
	void startLoop();
	/// Stop update loop.
	void stopLoop();

	/// Set the DMX hardware's framerate and the rate at which we send new data to the hardware.
	void setFramerate(int device_fps);

private:
	std::vector<uint8_t>	_message_body;
	std::mutex						_data_mutex;
	std::chrono::high_resolution_clock::duration _target_frame_time;
	ci::SerialRef         _serial;
	std::thread           _loop_thread;
	std::atomic_bool      _do_loop;

	/// Write data in a loop (runs on a secondary thread).
	void dataSendLoop();
};

using EnttecDeviceRef = std::shared_ptr<EnttecDevice>;

} // namespace dmx
