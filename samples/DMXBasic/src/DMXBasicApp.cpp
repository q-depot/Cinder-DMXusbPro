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
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
private:
	dmx::EnttecDeviceRef _device;
	dmx::ColorBuffer     _buffer;
};

void DMXBasicApp::setup()
{
	// Connect to enttec device and run data loop at 30 fps.
	_device = std::make_shared<dmx::EnttecDevice>("tty.usbserial-ENWER12L", 30);
}

void DMXBasicApp::mouseDown( MouseEvent event )
{
	auto channel = mix<size_t>(0, _buffer.size(), (float)event.getPos().x / getWindowWidth());
	auto value = mix(0, 255, (float)event.getPos().y / getWindowHeight());
	_buffer.setChannelValues(channel, Color8u(value, value, value));
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
