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
	void mouseDrag(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
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
    _settings.device_fps = 30;
    _device->applySettings(_settings);
//    _settings = _device->loadSettings().get();
//    console() << "after change: " << _settings << endl;
}

void DMXBasicApp::mouseDrag( MouseEvent event )
{
    auto channel = mix<size_t>(0, _buffer.size(), (float)event.getPos().x / getWindowWidth());
    auto value = mix(0, 255, (float)event.getPos().y / getWindowHeight());
    _buffer.setChannelValue(channel, value);
}

void DMXBasicApp::keyDown(cinder::app::KeyEvent event)
{

}

void DMXBasicApp::update()
{
	_device->bufferData(_buffer);
}

void DMXBasicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( DMXBasicApp, RendererGl )
