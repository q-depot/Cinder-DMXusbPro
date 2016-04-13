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

const auto EnttecDeviceDummyBaudRate = 56000;
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
		std::lock_guard<std::mutex> lock(_data_mutex);
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
		std::lock_guard<std::mutex> lock(_data_mutex);

		const Serial::Device dev = Serial::findDeviceByNameContains(device_name);
		_serial = Serial::create(dev, EnttecDeviceDummyBaudRate);
		_serial->flush();

		// Drain serial.
		auto old_stuff = std::vector<uint8_t>();
		old_stuff.resize(_serial->getNumBytesAvailable());
		_serial->readAvailableBytes(old_stuff.data(), old_stuff.size());
		return true;
	}
	catch(const std::exception &exc)
	{
		CI_LOG_E("Error initializing DMX device: " << exc.what());
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
			_serial->writeBytes(message.data(), message.size());

			auto response = std::vector<uint8_t>();
			auto response_is_valid = [&response] {
				if (! response.empty()) {
					return response.back() == EndOfMessage;
				}
				return false;
			};

			while (! response_is_valid()) {
				auto byte = _serial->readByte();
				if (byte == StartOfMessage) {
					response = { byte };
				}
				else {
					response.push_back(byte);
				}

				CI_LOG_D((int)byte);
			}

			const auto message_body_start = 4; // start, type, data lsb, data msb
			// skip two bytes (lsb, msb of message size, it seems)
			const auto firmware_index_lsb = message_body_start;
			const auto firmware_index_msb = message_body_start + 1;
			const auto break_time_index = message_body_start + 2;
			const auto mark_after_break_time_index = message_body_start + 3;
			const auto device_fps_index = message_body_start + 4;

			auto firmware_number = combinedNumber(response[firmware_index_lsb], response[firmware_index_msb]);
			auto settings = Settings {
				firmware_number,
				response[break_time_index],
				response[mark_after_break_time_index],
				response[device_fps_index]
			};
			return settings;
		}
		else {
			CI_LOG_W("Not connected via serial, returning empty settings.");
			return Settings{ 0, 0, 0, 0 };
		}
	});
}

void EnttecDevice::writeData()
{
	std::lock_guard<std::mutex> lock(_data_mutex);

	if (_serial) {
		// May need to include 2 or 3 more bytes in data size so it represents complete message size. (label, lsb, msb)
		// The documentation suggests otherwise, but the Mk2 sends back undocumented bytes when querying settings.
		// If we seem to be missing the last handful of channels of data when testing the full 512 channels, this could be why.
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