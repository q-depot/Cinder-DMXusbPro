//
//  EnttecDevice.cpp
//
//  Created by Soso Limited.
//
//

#include "EnttecDevice.h"
#include "cinder/app/App.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace dmx;

namespace {

const auto EnttecDeviceDummyBaudRate = 57600;
const auto BodySize = size_t(512);
const auto DataSize = BodySize + 1; // account for start code in message size
const auto MessageHeader = std::array<uint8_t, 5> {
	0x7E,  // Start of message
	6,     // Output Only Send DMX Packet Request
	(uint8_t)(DataSize & 0xFF),        // data length least significant byte
	(uint8_t)((DataSize >> 8) & 0xFF), // data length most significant byte
	0      // start code for data
};

const auto MessageFooter = std::array<uint8_t, 1> {
	0xE7 // End of message
};

} // namespace

EnttecDevice::EnttecDevice(const std::string &device_name, int device_fps)
{
	_target_frame_time = std::chrono::milliseconds(1000 / device_fps);
	_message_body.assign(BodySize, 0);
	connect(device_name);
}

EnttecDevice::~EnttecDevice()
{
	stopLoop();
	// For now, turn out the lights as the previous implementation did so.
	// In future, perhaps it is better to let the user specify what to do.
	fillBuffer(0);
	writeData();
	closeConnection();
}

void EnttecDevice::closeConnection() {
	stopLoop();

	if (_serial) {
		CI_LOG_I("Shutting down serial connection: " << _serial->getDevice().getPath());
		_serial->flush();
		_serial.reset();
	}
}

void EnttecDevice::startLoop() {
	_do_loop = true;
	_loop_thread = std::thread(&EnttecDevice::dataSendLoop, this);
}

void EnttecDevice::stopLoop() {
	_do_loop = false;
	if (_loop_thread.joinable()) {
		_loop_thread.join();
	}
}

bool EnttecDevice::connect(const std::string &device_name)
{
	closeConnection();

	try
	{
		const Serial::Device dev = Serial::findDeviceByNameContains(device_name);
		_serial = Serial::create(dev, EnttecDeviceDummyBaudRate);

		startLoop();
		return true;
	}
	catch(const std::exception &exc)
	{
		CI_LOG_E("Error initializing DMX device: " << exc.what());
		CI_ASSERT(_serial == nullptr);
		return false;
	}
}

void EnttecDevice::dataSendLoop()
{
	ci::ThreadSetup thread_setup;
	CI_LOG_I("Starting DMX loop.");

	auto before = std::chrono::high_resolution_clock::now();
	while (_do_loop)
	{
		writeData();

		auto after = std::chrono::high_resolution_clock::now();
		auto actual_frame_time = after - before;
		before = after;
		std::this_thread::sleep_for(_target_frame_time - actual_frame_time);
	}

	CI_LOG_I("Exiting DMX loop.");
}

void EnttecDevice::writeData()
{
	std::lock_guard<std::mutex> lock(_data_mutex);

	if (_serial) {
		_serial->writeBytes(MessageHeader.data(), MessageHeader.size());
		_serial->writeBytes(_message_body.data(), _message_body.size());
		_serial->writeBytes(MessageFooter.data(), MessageFooter.size());
	}
}

void EnttecDevice::bufferData(const uint8_t *data, size_t size)
{
	std::lock_guard<std::mutex> lock(_data_mutex);
	size = std::min(size, _message_body.size());
	std::memcpy(_message_body.data(), data, size);
}

void EnttecDevice::fillBuffer(uint8_t value)
{
	std::lock_guard<std::mutex> lock(_data_mutex);
	std::memset(_message_body.data(), value, _message_body.size());
}
