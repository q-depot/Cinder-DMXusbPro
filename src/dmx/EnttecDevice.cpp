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
const auto StartOfMessage = 0x7E;
const auto EndOfMessage = 0xE7;
const auto DataStartCode = 0x00;

enum MessageLabel {
//	ReprogramFirmware = 1,
//	ProgramFlashPage = 2,
	GetWidgetParameters = 3,
	SetWidgetParameters = 4,
	ReceivedDMXPacket = 5,
	OutputOnlySendDMXPacket = 6,
	SendRMDPacket = 7,
	ReceiveDMXOnChange = 8,
	ReceivedDMXChangeOfStatePacket = 9,
	GetWidgetSerialNumber = 10,
	SendRDMDiscovery = 11
};

uint8_t leastSignificantByte(int value) {
	return value & 0xFF;
}

uint8_t mostSignificantByte(int value) {
	return (value >> 8) & 0xFF;
}

int combinedNumber(uint8_t lsb, uint8_t msb) {
	return (msb << 8) + lsb;
}

} // namespace

EnttecDevice::EnttecDevice(const std::string &device_name, int device_fps)
{
	_target_frame_time = std::chrono::milliseconds(1000 / device_fps);
	_message_body.assign(512, 0);
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
		_serial->flush();
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

void EnttecDevice::applySettings(const dmx::EnttecDevice::Settings &settings) {
	std::lock_guard<std::mutex> lock(_data_mutex);

	const auto message = std::array<uint8_t, 8> {
		StartOfMessage,
		MessageLabel::SetWidgetParameters,
		leastSignificantByte(0), // no user settings to pass through
		mostSignificantByte(0),
		settings.break_time,
		settings.mark_after_break_time,
		settings.device_fps,
		EndOfMessage
	};

	_target_frame_time = std::chrono::milliseconds(1000 / settings.device_fps);
	_serial->writeBytes(message.data(), message.size());
}

std::future<EnttecDevice::Settings> EnttecDevice::loadSettings() const {
	return std::async(std::launch::async, [this] () {
		std::lock_guard<std::mutex> lock(_data_mutex);

		const auto message = std::array<uint8_t, 5> {
			StartOfMessage,
			MessageLabel::GetWidgetParameters,
			leastSignificantByte(0),
			mostSignificantByte(0),
			EndOfMessage
		};

		if (_serial) {
			auto response = std::array<uint8_t, 8> {};
			_serial->writeBytes(message.data(), message.size());
			_serial->readBytes(response.data(), response.size());

			CI_ASSERT(response[0] == StartOfMessage);

			auto response_type = response[1];
			if (response_type == MessageLabel::GetWidgetParameters) {
				auto firmware_number = combinedNumber(response[2], response[3]);
				auto settings = Settings {
					firmware_number,
					response[4],
					response[5],
					response[6]
				};
				return settings;
			}
			else {
				CI_LOG_E("Unexpected response from DMX device");
				return Settings{ 0, 0, 0, 0 };
			}

		}
		else {
			return Settings{ 0, 0, 0, 0 };
		}
	});
}

void EnttecDevice::writeData()
{
	std::lock_guard<std::mutex> lock(_data_mutex);

	if (_serial) {
		const auto data_size = _message_body.size() + 1; // account for data start code
		const auto header = std::array<uint8_t, 5> {
			StartOfMessage,
			MessageLabel::OutputOnlySendDMXPacket,
			leastSignificantByte(data_size),
			mostSignificantByte(data_size),
			DataStartCode
		};
		_serial->writeBytes(header.data(), header.size());
		_serial->writeBytes(_message_body.data(), _message_body.size());
		_serial->writeByte(EndOfMessage);
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

namespace dmx {

std::ostream& operator << (std::ostream& os, const EnttecDevice::Settings& settings) {
    os << "Firmware version: " << settings.firmware_number;
		os << ", Break time: " << (int)settings.break_time;
		os << ", Mark after break time: " << (int)settings.mark_after_break_time;
		os << ", Device fps: " << (int)settings.device_fps;
    return os;
}

}