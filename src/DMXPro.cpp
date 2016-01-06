/*
 *  DMXPro.cpp
 *
 *  Created by Andrea Cuius
 *  The MIT License (MIT)
 *  Copyright (c) 2014 Nocte Studio Ltd.
 *
 *  www.nocte.co.uk
 *
 */

#include "cinder/app/App.h"
#include "cinder/Utilities.h"
#include <iostream>
#include "DMXPro.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const auto DMXPRO_START_MSG   = 0x7E;  // Start of message delimiter
const auto DMXPRO_END_MSG     = 0xE7;  // End of message delimiter
const auto DMXPRO_SEND_LABEL  = 6;     // Output Only Send DMX Packet Request
const auto DMXPRO_BAUD_RATE   = 57600; // virtual COM doesn't control the usb, this is just a dummy value
const auto DMXPRO_FRAME_RATE  = 35;    // dmx send frame rate
const auto DMXPRO_START_CODE  = 0;

const auto BodySize = 513; // account for start code in message size
const auto MessageHeader = std::array<uint8_t, 5> {
	DMXPRO_START_MSG,
	DMXPRO_SEND_LABEL,
	(uint8_t)(BodySize & 0xFF),        // data length least significant byte
	(uint8_t)((BodySize >> 8) & 0xFF), // data length most significant byte
	DMXPRO_START_CODE
};

const auto MessageFooter = std::array<uint8_t, 1> {
	DMXPRO_END_MSG
};

const auto MessageSize = BodySize + MessageHeader.size() + MessageFooter.size();

DMXPro::DMXPro()
{
	mTargetFrameTime = std::chrono::milliseconds(1000 / DMXPRO_FRAME_RATE);
	mBody.assign(512, 0);
}

DMXPro::~DMXPro()
{
	CI_LOG_I("Shutting down DMX connection: " << mSerialDeviceName);

	stopLoop();
	// For now, turn out the lights as the previous implementation did so.
	// In future, perhaps it is better to let the user specify what to do.
	fillBuffer(0);
	writeData();
	closeConnection();
}

void DMXPro::closeConnection() {
	stopLoop();

	if (mSerial) {
		mSerial->flush();
		mSerial.reset();
	}
}

void DMXPro::startLoop() {
	mRunSendDataThread = true;
	mSendDataThread = std::thread(&DMXPro::dataSendLoop, this);
}

void DMXPro::stopLoop() {
	mRunSendDataThread = false;
	if (mSendDataThread.joinable()) {
		mSendDataThread.join();
	}
}

bool DMXPro::connect(const std::string &deviceName)
{
	closeConnection();
	mSerialDeviceName = deviceName;

	try
	{
		const Serial::Device dev = Serial::findDeviceByNameContains(mSerialDeviceName);
		mSerial = Serial::create( dev, DMXPRO_BAUD_RATE );

		startLoop();
		return true;
	}
	catch(const std::exception &exc)
	{
		CI_LOG_E("Error initializing DMX device: " << exc.what());
		return false;
	}
}

void DMXPro::dataSendLoop()
{
	ci::ThreadSetup threadSetup;
	CI_LOG_I("Starting DMX loop.");
	mRunSendDataThread = true;

	auto before = std::chrono::high_resolution_clock::now();
	while (mSerial && mRunSendDataThread)
	{
		writeData();

		auto after = std::chrono::high_resolution_clock::now();
		auto actualFrameTime = after - before;
		before = after;
		std::this_thread::sleep_for(mTargetFrameTime - actualFrameTime);
	}

	CI_LOG_I("Exiting DMX loop.");
}

void DMXPro::writeData()
{
	std::lock_guard<std::mutex> lock(mBodyMutex);

	if (mSerial) {
		mSerial->writeBytes(MessageHeader.data(), MessageHeader.size());
		mSerial->writeBytes(mBody.data(), mBody.size());
		mSerial->writeBytes(MessageFooter.data(), MessageFooter.size());
	}
}

void DMXPro::bufferData(const uint8_t *data, size_t size)
{
	std::lock_guard<std::mutex> lock(mBodyMutex);
	mBody.assign(data, data + size);
}

void DMXPro::fillBuffer(uint8_t value)
{
	std::lock_guard<std::mutex> lock(mBodyMutex);
	mBody.assign(512, value);
}

#pragma mark - DMX Color Buffer

DMXColorBuffer::DMXColorBuffer()
{
	_data.fill(0);
}

void DMXColorBuffer::setValue(uint8_t value, size_t channel) {
	channel = std::min(channel, _data.size() - 1);
	_data[channel] = value;
}

void DMXColorBuffer::setValue(const ci::Color8u &color, size_t channel) {
	channel = std::min(channel, _data.size() - 3);
	_data[channel] = color.r;
	_data[channel + 1] = color.g;
	_data[channel + 2] = color.b;
}
