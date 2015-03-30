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

#ifndef DMX_USB_PRO
#define DMX_USB_PRO

#pragma once

#include "cinder/Thread.h"
#include "cinder/Serial.h"


#define DMXPRO_START_MSG		0x7E		// Start of message delimiter
#define DMXPRO_END_MSG			0xE7		// End of message delimiter
#define DMXPRO_SEND_LABEL		6			// Output Only Send DMX Packet Request
#define DMXPRO_BAUD_RATE		57600		// virtual COM doesn't control the usb, this is just a dummy value
#define DMXPRO_FRAME_RATE		35			// dmx send frame rate
#define DMXPRO_DATA_SIZE       	513         // include first byte 0x00, what's that?
#define DMXPRO_PACKET_SIZE      518         // data + 4 bytes(DMXPRO_START_MSG, DMXPRO_SEND_LABEL, DATA_SIZE_LSB, DATA_SIZE_MSB) at the beginning and 1 byte(DMXPRO_END_MSG) at the end

//////////////////////////////////////////////////////////
// LAST 4 dmx channels seem not to be working, 508-511 !!!
//////////////////////////////////////////////////////////

class DMXPro;
typedef std::shared_ptr<DMXPro> DMXProRef;

class DMXPro{

public:
  
    static DMXProRef create( const std::string &deviceName )
    {
        return DMXProRef( new DMXPro( deviceName ) );
    }
    
	~DMXPro();

	void init(bool initWithZeros = true);

	static void listDevices()
    {
        const std::vector<ci::Serial::Device> &devices( ci::Serial::getDevices(true) );

        ci::app::console() << "--- DMX usb pro > List serial devices ---" << std::endl;
        
        for( std::vector<ci::Serial::Device>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt )
            ci::app::console() << deviceIt->getName() << std::endl;

        ci::app::console() << "-----------------------------------------" << std::endl;
    }

    static std::vector<std::string> getDevicesList()
    {
        std::vector<std::string> devicesList;

		const std::vector<ci::Serial::Device> &devices( ci::Serial::getDevices(true) );
		
        for( std::vector<ci::Serial::Device>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt )
            devicesList.push_back( deviceIt->getPath() );
        
        return devicesList;
	}

	void	setZeros();
	
	bool	isConnected() { return mSerial != NULL; }
	
	void	setValue(int value, int channel);
	
	void	reconnect();
	
	void	shutdown(bool send_zeros = true);
	
    std::string  getDeviceName() { return mSerialDeviceName; }
	
private:
    
    DMXPro( const std::string &deviceName );

	void initDMX();
    
	void initSerial(bool initWithZeros);

	void			sendDMXData();

private:
    
	unsigned char	*mDMXPacket;			// DMX packet, it contains bytes
	ci::Serial		*mSerial;				// serial interface
	int				mThreadSleepFor;		// sleep for N ms, this is based on the FRAME_RATE
	std::mutex      mDMXDataMutex;			// mutex unique lock
	std::string		mSerialDeviceName;		// usb serial device name
    std::thread     mSendDataThread;
    bool            mRunSendDataThread;
    
    
private:
    // disallow
    DMXPro(const DMXPro&);
    DMXPro& operator=(const DMXPro&);
    
};


#endif