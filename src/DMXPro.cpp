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

using namespace ci;
using namespace ci::app;
using namespace std;


DMXPro::DMXPro( const string &deviceName, DeviceMode mode ) : mSerialDeviceName(deviceName)
{
    mDeviceMode             = mode;
    mSerial                 = nullptr;
	mSenderThreadSleepFor   = 1000 / DMXPRO_FRAME_RATE;
    
	init();
	
	setZeros();
}


DMXPro::~DMXPro()
{
    shutdown(true);
}


void DMXPro::shutdown(bool send_zeros)
{
    console() << "DMXPro shutting down.." << endl;

	if ( mSerial )
	{
		if (send_zeros)
			setZeros();					// send zeros to all channels
		
		ci::sleep( mSenderThreadSleepFor * 2 );
        
        mRunDataThread = false;

        if ( mDataThread.joinable() )
        {
            console() << "thread is joinable" << endl;
            mDataThread.join();
        }
        else
            console() << "cannot join thread!" << endl;
        
        mSerial->flush();
        mSerial = nullptr;
	}
    else
    {
        mDataThread.detach();
    }
    
    console() << "DMXPro > shutdown!" << endl;
}


void DMXPro::init(bool initWithZeros) 
{
    console() << "DMXPro > Initializing device" << endl;

	initDMX();
    initSerial(initWithZeros);
    
    mDataThread = std::thread( &DMXPro::processDMXData, this );
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
        mSerial = nullptr;
		ci::sleep(50);	
	}
	
	try 
    {
        Serial::Device dev = Serial::findDeviceByNameContains(mSerialDeviceName);
        mSerial = Serial::create( dev, DMXPRO_BAUD_RATE );
        console() << "DMXPro > Connected to usb DMX interface: " << dev.getName() << endl;
	}
	catch( ... ) 
    {
        console() << "DMXPro > There was an error initializing the usb DMX device" << endl;
		mSerial = nullptr;
	}
}


void DMXPro::initDMX()
{
    // LAST 4 dmx channels seem not to be working, 508-511 !!!
    
    for (int i=0; i < DMXPRO_PACKET_SIZE; i++)                      // initialize all channels with zeros, data starts from [5]
		mDMXPacketOut[i] = 0;
    
    mDMXPacketOut[0] = DMXPRO_START_MSG;							// DMX start delimiter 0x7E
	mDMXPacketOut[1] = DMXPRO_SEND_LABEL;							// set message type
    mDMXPacketOut[2] = (int)DMXPRO_DATA_SIZE & 0xFF;				// Data Length LSB
    mDMXPacketOut[3] = ((int)DMXPRO_DATA_SIZE >> 8) & 0xFF;         // Data Length MSBc
	mDMXPacketOut[4] = 0;                                           // DMX start code
	mDMXPacketOut[DMXPRO_PACKET_SIZE-1] = DMXPRO_END_MSG;           // DMX start delimiter 0xE7
    
    // init incoming DMX data
    for( size_t k=0; k < 512; k++ )
        mDMXDataIn[k] = 0;
}


void DMXPro::processDMXData()
{
    console() << "DMXPro::processDMXData() start thread" << endl;
    
    mRunDataThread = true;
    
    if ( mDeviceMode == DeviceMode::SENDER )
    {
        while( mSerial && mRunDataThread )
        {
            std::unique_lock<std::mutex> dataLock(mDMXDataMutex);                               // get DMX packet UNIQUE lock
            mSerial->writeBytes( mDMXPacketOut, DMXPRO_PACKET_SIZE );                           // send data
            dataLock.unlock();                                                                  // unlock data
            std::this_thread::sleep_for( std::chrono::milliseconds( mSenderThreadSleepFor ) );
        }
    }
    
    
    else if ( mDeviceMode == DeviceMode::RECEIVER )
    {
        unsigned char   value = '*';    // set to something different than packet label or start msg
        uint32_t        packetDataSize;

        while( mSerial && mRunDataThread )
        {
            // wait for start message and label
		    
            /*
            while ( value != DMXPRO_START_MSG && mSerial->getNumBytesAvailable() > 0 )
		    {
			    value = mSerial->readByte();
		    }
                
            if (  mSerial->getNumBytesAvailable() == 0 || mSerial->readByte() != DMXPRO_RECEIVE_PACKET_LABEL )
			{
                value = '*';
                continue;
            }
            */
            
            while ( mRunDataThread && value != DMXPRO_RECEIVE_PACKET_LABEL )
            {
                while ( mRunDataThread && value != DMXPRO_START_MSG )
                {
                    if ( mSerial->getNumBytesAvailable() > 0  )
                        value = mSerial->readByte();
                }
                
                if ( mSerial->getNumBytesAvailable() > 0  )
                    value = mSerial->readByte();
            }            
            
            // read header
            if ( mSerial->getNumBytesAvailable() < 2 )
                continue;
            
            packetDataSize = mSerial->readByte();                       // LSB
            packetDataSize += ( (uint32_t)mSerial->readByte() ) << 8;   // MSB

	        // Check Length is not greater than allowed
	        if ( packetDataSize <= 514 ) // dmx data + 2 start zeros
            {
	            // Read the actual Response Data
                mSerial->readAvailableBytes( mDMXPacketIn, packetDataSize );

                // finally check the end code
                if ( mSerial->getNumBytesAvailable() > 0 && mSerial->readByte() == DMXPRO_END_MSG )
                {
                    // valid packet, parse DMX data
                    // the first 2 bytes are 0(by specs there should only be 1 zero, not sure where the other one comes from!) 
                    std::unique_lock<std::mutex> dataLock(mDMXDataMutex);
                    for( size_t k=2; k < packetDataSize; k++ )
                    {
                        mDMXDataIn[k-2] = mDMXPacketIn[k];
                    }
                    dataLock.unlock();
                }      

                else                    // invalid packet, reset
                    value = '*';
            }

            else                        // invalid packet, reset
                value = '*';

            std::this_thread::sleep_for( std::chrono::milliseconds( 16 ) );
        }
    }
    
    mRunDataThread = false;
    
    console() << "DMXPro > sendDMXData() thread exited!" << endl;
}


void DMXPro::setValue(int value, int channel) 
{    
	if ( channel < 0 || channel > DMXPRO_PACKET_SIZE-2 )
	{
        console() << "DMXPro > invalid DMX channel: " << channel << endl;
        return;
	}
    // DMX channels start form byte [5] and end at byte [DMXPRO_PACKET_SIZE-2], last byte is EOT(0xE7)        
	value = math<int>::clamp(value, 0, 255);
	std::unique_lock<std::mutex> dataLock(mDMXDataMutex);			// get DMX packet UNIQUE lock
	mDMXPacketOut[ 5 + channel ] = value;                                  // update value
	dataLock.unlock();													// unlock mutex
}


size_t DMXPro::getValue(int channel) 
{    
	if ( channel < 0 || channel > 512 )
	{
        console() << "DMXPro > invalid DMX channel: " << channel << endl;
        return 0;
	}

    size_t val;
	
    std::unique_lock<std::mutex> dataLock(mDMXDataMutex);
	val = mDMXDataIn[channel];
	dataLock.unlock();	

    return val;
}


void DMXPro::setZeros()
{
    for (int i=5; i < DMXPRO_PACKET_SIZE-2; i++)                        // DMX channels start form byte [5] and end at byte [DMXPRO_PACKET_SIZE-2], last byte is EOT(0xE7)
		mDMXPacketOut[i] = 0;
}

