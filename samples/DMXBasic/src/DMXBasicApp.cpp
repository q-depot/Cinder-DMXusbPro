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
	// This sometimes stalls while waiting for a response from the DMX box.
	// Will eventually add a timeout to the serial read and throw an exception on failure.
//	_settings = _device->loadSettings().get();
//	console() << _settings << endl;

	// Start sending data to device on an interval in a secondary thread.
	_device->startLoop();

	// As of now, applying the settings does not work as expected.
//    _settings.device_fps = 40;
//    _device->applySettings(_settings);

	CI_ASSERT(_buffer.lastColorIndex() == 507);
}

void DMXBasicApp::update()
{
	auto t = fract(getElapsedSeconds() * 0.5);
	for (auto c = 0; c <= _buffer.lastColorIndex(); c += 3) {
		auto hue = fract(t + (c / 2048.0f));
		auto color = Color(CM_HSV, hue, 0.8, 0.3);
		_buffer.setChannelValues(c, color);
	}

	// Update the data to send to the DMX device.
	_device->bufferData(_buffer);
}

void DMXBasicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	auto pos = vec2(10.0f, 10.0f);
	// retrieve RGB triples from buffer.
	for (auto i = 0; i <= _buffer.lastColorIndex(); i += 3) {
		auto color = *reinterpret_cast<const Color8u*>(&_buffer.data()[i]);
		gl::color(color);
		gl::drawSolidCircle(pos, 8.0f);
		pos.x += 24.0f;
		if (pos.x > getWindowWidth() - 10.0f) {
			pos.y += 32.0f;
			pos.x = 10.0f;
		}
	}
}

CINDER_APP( DMXBasicApp, RendererGl )
