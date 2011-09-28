

#pragma once

#include "cinder/app/AppBasic.h"
#include <iostream>
#include "cinder/Serial.h"
#include "cinder/Thread.h"
#include "Logger.h"

#define DMX_PRO_START_MSG		0x7E		// Start of message delimiter
#define DMX_PRO_END_MSG			0xE7		// End of message delimiter
#define DMX_PRO_LABEL			6			// Output Only Send DMX Packet Request
#define	BAUD_RATE				57600		// virtual COM doesn't control the usb, this is just a dummy value
#define DMX_FRAME_RATE			38			// dmx send frame rate



class DMXPro{

public:

	DMXPro(std::string serialDevicePath, int channels_N);

	~DMXPro() {};

	void init(bool initWithZeros = true);

	ci::Serial::Device findDeviceByPathContains( const std::string &searchString);

	static void listDevices() {
		const std::vector<ci::Serial::Device> &devices( ci::Serial::getDevices(true) );
		for( std::vector<ci::Serial::Device>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt ) {
			Logger::log("DMX usb pro > List serial devices: " + deviceIt->getPath() );
		}
	};

	void	sendZeros();
	
	bool	isConnected() { return mSerial != NULL; };
	
	void	setValue(int value, int channel);
	
	void	reconnect();
	
	void	shutdown(bool send_zeros = true);
	
	
private:
	
	void initDMX();
	void initSerial(bool initWithZeros);

	void			sendDMXData();
	
	int				mChannels;				// number of channels in the packet
	int				mDMXPacketSize;			// DMX packet size == mChannel + 5
	unsigned char	*mDMXPacket;			// DMX packet, it contains bytes
	ci::Serial		*mSerial;				// serial interface
	int				mThreadSleepFor;		// sleep for N ms, this is based on the FRAME_RATE
	boost::mutex	mDMXDataMutex;			// mutex unique lock
	std::string		mSerialDevicePath;		// usb serial device path
};

