#Cinder-DMXUsbPro
CinderBlock for the [Enttec DMX USB Pro](https://www.enttec.com/?main_menu=Products&pn=70304)

##Get the code
```
cd CINDER_PATH/blocks  
git clone git@github.com:q-depot/Cinder-DMXusbPro.git
```

##How to use it

* DMX channel starts from 0
* Values range is 0-255 (1 byte per channel)

__DMXPro runs a thread in the background to send the data at 35fps.__  
DMX is __not realiable__ with higher frame rates, I recommend not to change this value.


```C++
#include "DMXPro.h"

DMXProRef	mDmxController;

// ...

void BasicSampleApp::setup()
{
	// list all the available serial devices
	// DMX Pro is usually something like "tty.usbserial-EN.."
	DMXPro::listDevices();

	// create a device passing the device name or a partial device name
	// useful if you want to swap device without changing the name
    mDmxController  = DMXPro::create( "tty.usbserial-EN" );

	if ( mDmxController->isConnected() )
		console() << "DMX device connected" << endl;
}

void BasicSampleApp::update()
{
	int dmxChannel	= 2;
	int dmxValue 	= 255;

	if ( mDmxController && mDmxController->isConnected() )
    {
    	// send value 255 to channel 2
        mDmxController->setValue( dmxValue, dmxChannel );
    }
}

```

--

### License
The MIT License (MIT)

Copyright (c) 2014 Nocte Studio Ltd. - [www.nocte.co.uk](http://www.nocte.co.uk)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.