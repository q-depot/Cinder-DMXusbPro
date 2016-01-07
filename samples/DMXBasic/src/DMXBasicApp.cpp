#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "dmx/EnttecDevice.h"
#include "dmx/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DMXBasicApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;
private:
	dmx::EnttecDeviceRef				_device;
	dmx::ColorBuffer						_buffer;
	dmx::EnttecDevice::Settings _settings;
};

void DMXBasicApp::setup()
{
    auto devices = Serial::getDevices();
    for (auto &device: devices) {
        console() << device.getName() << " at " << device.getPath() << endl;
    }
	// Connect to enttec device and run data loop at 30 fps.
	_device = std::make_shared<dmx::EnttecDevice>("tty.usbserial-EN", 30);
	// Load the device settings synchronously.
	_settings = _device->loadSettings().get();
	console() << _settings << endl;

    // As of now, it is unclear whether applying the settings works as expected.
    _settings.device_fps = 30;
    _device->applySettings(_settings);
}

void DMXBasicApp::update()
{
    auto t = fract(getElapsedSeconds());
    for (auto c = 0; c <= 512 - 3; c += 3) {
        auto hue = fract(t + (c / 512.0f));
        auto color = Color(CM_HSV, hue, 0.5, 0.5);
        _buffer.setChannelValues(c, color);
    }
	_device->bufferData(_buffer);
}

void DMXBasicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( DMXBasicApp, RendererGl )
