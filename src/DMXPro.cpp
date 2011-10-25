 

#include "DMXPro.h"

using namespace ci;
using namespace ci::app;
using namespace std;


DMXPro::DMXPro(string serialDevicePath, int channels_N) : mDMXPacket(NULL), mSerialDevicePath(serialDevicePath), mSerial(NULL)
{	
	mChannels	= channels_N;
	mDMXPacketSize = mChannels + 5;
	
	mThreadSleepFor = 1000 / DMX_FRAME_RATE;

	init();
	
	sendZeros();
}


void DMXPro::shutdown(bool send_zeros)
{
	if ( mSerial )
	{
		if (send_zeros)
			sendZeros();					// send zeros to all channels
		
		ci::sleep(mThreadSleepFor*2);	
		mSerial->flush();	
		delete mSerial;
		mSerial = NULL;
		ci::sleep(50);	
	}
	Logger::log("DMXPro > shutdown!");
}


void DMXPro::init(bool initWithZeros) 
{
	Logger::log("DMXPro > Initializing device");
	initDMX();
	initSerial(initWithZeros);
}


void DMXPro::initSerial(bool initWithZeros)
{
	if ( mSerial )
	{
		if (initWithZeros)
		{
			sendZeros();					// send zeros to all channels
			Logger::log("DMXPro > Init serial with zeros() before disconnect" );
			ci::sleep(100);	
		}
		mSerial->flush();	
		delete mSerial;
		mSerial = NULL;
		ci::sleep(50);	
	}
	
	try {
		Serial::Device dev = findDeviceByPathContains(mSerialDevicePath);
		mSerial = new Serial(dev, BAUD_RATE);
		Logger::log("DMXPro > Connected to usb DMX interface: " + mSerialDevicePath );
	}
	catch( ... ) {
		Logger::log("DMXPro > There was an error initializing the usb DMX device");
		mSerial = NULL;
	}
	thread sendDMXDataThread( &DMXPro::sendDMXData, this);				// start thread to send data at the specific DMX_FRAME_RATE 
}


void DMXPro::initDMX()
{
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
}


void DMXPro::sendDMXData() 
{
	while(mSerial) {
//		sendPacket();
		boost::unique_lock<boost::mutex> dataLock(mDMXDataMutex);			// get DMX packet UNIQUE lock 
		mSerial->writeBytes(mDMXPacket, mDMXPacketSize);					// send data
		dataLock.unlock();													// unlock data
		
		boost::this_thread::sleep(boost::posix_time::milliseconds(mThreadSleepFor));
	}
	Logger::log("DMXPro > sendDMXData() thread exited!");
}


Serial::Device DMXPro::findDeviceByPathContains( const string &searchString) 
{
	const std::vector<Serial::Device> &devices = Serial::getDevices();
	for( std::vector<Serial::Device>::const_iterator deviceIt = devices.begin(); deviceIt != devices.end(); ++deviceIt ) {
		if( deviceIt->getPath().find( searchString ) != std::string::npos )
			return *deviceIt;
	}
	return Serial::Device();
}

/*
void DMXPro::sendPacket(){
	if ( !mIsConnected )
		return;
	
	boost::unique_lock<boost::mutex> dataLock(mDMXDataMutex);			// get DMX packet UNIQUE lock 
	mSerial->writeBytes(mDMXPacket, mDMXPacketSize);						// send data
	dataLock.unlock();													// unlock data
}
*/

void DMXPro::setValue(int value, int channel) {
	if ( channel < 1 || channel > 512 )
	{
		Logger::log( "DMXPro > invalid DMX channel: " + toString(channel) );
		exit(-1);
	}
	value = math<int>::clamp(value, 0, 255);
	boost::unique_lock<boost::mutex> dataLock(mDMXDataMutex);			// get DMX packet UNIQUE lock 
	mDMXPacket[4+channel] = value;										// update value
	dataLock.unlock();													// unlock mutex
	//console() << "DMX " << value << endl;
}


void DMXPro::sendZeros(){
	for (int i=4; i < (mChannels + 4); i++)
		mDMXPacket[i] = 0;
}

/*
 not sure this is correct, not needed at the moment
int DMXPro::getValue(int channel){
	return (int)(mDMXPacket[4+channel]);
}

*/