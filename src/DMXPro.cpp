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


DMXPro::DMXPro( const string &deviceName ) : mDMXPacket(NULL), mSerialDeviceName(deviceName), mSerial(NULL)
{
	mThreadSleepFor = 1000 / DMXPRO_FRAME_RATE;

	init();

	setZeros();
}


DMXPro::~DMXPro()
{
	setZeros();

	ci::sleep(50);

	mRunSendDataThread = false;
	if ( mSendDataThread.joinable() )
		mSendDataThread.join();

	if ( mSerial )
	{
		mSerial->flush();
		mSerial = NULL;
	}


	delete []mDMXPacket;

	console() << "shutdown DMXPro" << endl;
}

void DMXPro::shutdown(bool send_zeros)
{
	if ( mSerial )
	{
		if (send_zeros)
			setZeros();					// send zeros to all channels

		ci::sleep( mThreadSleepFor*2 );
		mSerial->flush();
		mSerial = NULL;
		ci::sleep(50);
	}
	console() << "DMXPro > shutdown!" << endl;
}


void DMXPro::init(bool initWithZeros)
{
	console() << "DMXPro > Initializing device" << endl;

	initDMX();
	initSerial(initWithZeros);
}


void DMXPro::initSerial(bool initWithZeros)
{
	if ( mSerial )
	{
		if (initWithZeros)
		{
			setZeros();					// send zeros to all channels
			console() << "DMXPro > Init serial with zeros() before disconnect" << endl;
			ci::sleep(100);
		}
		mSerial->flush();
		mSerial = NULL;
		ci::sleep(50);
	}

	try
	{
		const Serial::Device dev = Serial::findDeviceByNameContains(mSerialDeviceName);
		mSerial = Serial::create( dev, DMXPRO_BAUD_RATE );
		console() << "DMXPro > Connected to usb DMX interface: " << dev.getName() << endl;
	}
	catch( ... )
	{
		console() << "DMXPro > There was an error initializing the usb DMX device" << endl;
		mSerial = NULL;
	}

	mSendDataThread = std::thread( &DMXPro::sendDMXData, this );
}


void DMXPro::initDMX()
{
	delete []mDMXPacket;
	mDMXPacket	= NULL;
	mDMXPacket	= new unsigned char[DMXPRO_PACKET_SIZE];

	// LAST 4 dmx channels seem not to be working, 508-511 !!!

	for (int i=0; i < DMXPRO_PACKET_SIZE; i++)                      // initialize all channels with zeros, data starts from [5]
		mDMXPacket[i] = 0;

	mDMXPacket[0] = DMXPRO_START_MSG;								// DMX start delimiter 0x7E
	mDMXPacket[1] = DMXPRO_SEND_LABEL;								// set message type
	mDMXPacket[2] = (int)DMXPRO_DATA_SIZE & 0xFF;					// Data Length LSB
	mDMXPacket[3] = ((int)DMXPRO_DATA_SIZE >> 8) & 0xFF;            // Data Length MSB
	mDMXPacket[4] = 0;                                              // NO IDEA what this is for!
	mDMXPacket[DMXPRO_PACKET_SIZE-1] = DMXPRO_END_MSG;              // DMX start delimiter 0xE7
}


void DMXPro::sendDMXData()
{
	mRunSendDataThread = true;

	while( mSerial && mRunSendDataThread )
	{
		std::unique_lock<std::mutex> dataLock(mDMXDataMutex);                           // get DMX packet UNIQUE lock
		mSerial->writeBytes( mDMXPacket, DMXPRO_PACKET_SIZE );                          // send data
		dataLock.unlock();                                                              // unlock data
		std::this_thread::sleep_for( std::chrono::milliseconds( mThreadSleepFor ) );
	}

	console() << "DMXPro > sendDMXData() thread exited!" << endl;
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
}

void DMXPro::writeData()
{
	std::lock_guard<std::mutex> lock(mBodyMutex);
	auto header = std::array<uint8_t, 5>{
		DMXPRO_START_MSG,
		DMXPRO_SEND_LABEL,
		(uint8_t)(mBody.size() & 0xFF),        // data length least significant byte
		(uint8_t)((mBody.size() >> 8) & 0xFF), // data length most significant byte
		0 // used in previous implementation (though marked as having unknown purpose); perhaps DMXPro device has an offset?
	};

	mSerial->writeBytes(header.data(), header.size());
	mSerial->writeBytes(mBody.data(), mBody.size());
	mSerial->writeByte(DMXPRO_END_MSG);
}

void DMXPro::setValue(int value, int channel)
{
	if ( channel < 0 || channel > DMXPRO_PACKET_SIZE-2 )
	{
		console() << "DMXPro > invalid DMX channel: " << channel << endl;
		return;
	}
	// DMX channels start from byte [5] and end at byte [DMXPRO_PACKET_SIZE-2], last byte is EOT(0xE7)
	value = math<int>::clamp(value, 0, 255);
	std::unique_lock<std::mutex> dataLock(mDMXDataMutex);			// get DMX packet UNIQUE lock
	// LA NOTE: added -1 so that dmx addresses would start at 1, not 0
	mDMXPacket[ 5 + channel - 1 ] = value;                                  // update value
	dataLock.unlock();													// unlock mutex
}


void DMXPro::setZeros()
{
	std::lock_guard<std::mutex> lock(mBodyMutex);
	for (auto &dataPoint : mBody) {
		dataPoint = 0;
	}

	for (int i=5; i < DMXPRO_PACKET_SIZE-2; i++)                        // DMX channels start form byte [5] and end at byte [DMXPRO_PACKET_SIZE-2], last byte is EOT(0xE7)
		mDMXPacket[i] = 0;
}
