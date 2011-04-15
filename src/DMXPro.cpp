 

#include "DMXPro.h"

using namespace ci;
using namespace ci::app;
using namespace std;


DMXPro::DMXPro(string serialDevicePath, int channels_N)
:
mDMXPacket(NULL)
{
	mChannels	= channels_N;
	mDMXPacketSize = mChannels + 5;
	delete []mDMXPacket;
	mDMXPacket	= NULL;
	mDMXPacket	= new unsigned char[mDMXPacketSize];
	
	mDMXPacket[0] = DMX_PRO_START_MSG;									// DMX start delimiter 0x7E
	mDMXPacket[1] = DMX_PRO_LABEL;										// set message type
	mDMXPacket[2] = mChannels & 255;									// Data Length LSB
	mDMXPacket[3] = (mChannels >> 8) & 255;								// Data Length MSB
	
	for (int i=4; i < (mChannels + 4); i++)								// initialize all channels with zeros
		mDMXPacket[i] = 0;

	mDMXPacket[mDMXPacketSize-1] = DMX_PRO_END_MSG;
	
	mIsConnected = false;
	
	mThreadSleepFor = 1000 / DMX_FRAME_RATE;
	
	initSerial(serialDevicePath);
	
	mHasNewValues = true;												// send zeros on start up
	
	// start thread to send data at the specific DMX_FRAME_RATE 
	if (mIsConnected)
		thread sendDMXDataThread( &DMXPro::sendDMXData, this);
}


void DMXPro::sendDMXData() {
	
	boost::unique_lock<boost::mutex> dataLock(mDMXDataMutex);			// get DMX packet UNIQUE lock 
	dataLock.unlock();
	
	while(true) {
		if(mHasNewValues) {
			dataLock.lock();											// lock data
			sendPacket();												// send packet
			mHasNewValues = false;
			dataLock.unlock();											// unlock data
			boost::this_thread::sleep(boost::posix_time::milliseconds(mThreadSleepFor));
		}
	}
}


void DMXPro::initSerial(string devicePath){
	try {
		Serial::Device dev = findDeviceByPathContains(devicePath);
		mSerial = Serial(dev, BAUD_RATE);
		console() << "Connected to usb DMX interface: " << devicePath << endl;
		mIsConnected = true;
	}
	catch( ... ) {
		console() << "There was an error initializing the usb DMX device" << endl;
		mIsConnected = false;
	}
}


Serial::Device DMXPro::findDeviceByPathContains( const string &searchString) {
	const std::vector<Serial::Device> &devices = Serial::getDevices();
	for( std::vector<Serial::Device>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt ) {
		if( deviceIt->getPath().find( searchString ) != std::string::npos )
			return *deviceIt;
	}
	return Serial::Device();
}


void DMXPro::sendPacket(){
	mSerial.writeBytes(mDMXPacket, mDMXPacketSize);
}


void DMXPro::sendValue(int value, int channel, bool force) {
	
	value = math<int>::clamp(value, 0, 255);
	
	mHasNewValues = force;
	
	if( mDMXPacket[4+channel] == value )
		return;
	
	boost::unique_lock<boost::mutex> dataLock(mDMXDataMutex);			// get DMX packet UNIQUE lock 
	mDMXPacket[4+channel] = value;										// update value
	mHasNewValues = true;
	dataLock.unlock();													// unlock mutex
}


void DMXPro::sendZeros(){
	for (int i=4; i < (mChannels + 4); i++)
		mDMXPacket[i] = 0;
	
	mHasNewValues = true;
}


int DMXPro::getValue(int channel){
	return (int)(mDMXPacket[4+channel]);
}

