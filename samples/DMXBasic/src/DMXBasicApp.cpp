#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DMXBasicApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void DMXBasicApp::setup()
{
}

void DMXBasicApp::mouseDown( MouseEvent event )
{
}

void DMXBasicApp::update()
{
}

void DMXBasicApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( DMXBasicApp, RendererGl )
