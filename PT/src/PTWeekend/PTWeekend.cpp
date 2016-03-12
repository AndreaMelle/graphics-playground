#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "ptUtil.h"
#include "ptTests.h"
#include "ptGeometry.h"

using namespace ci;
using namespace ci::app;
using namespace std;



class PTWeekend : public App {
public:
    void setup() override;
    void mouseDown( MouseEvent event ) override;
    void update() override;
    void draw() override;
    void cleanup() override;
    
private:
    gl::Texture2dRef imgTex;
    
    double			 renderTime;
    
    char* frameBuffer;
    size_t frameBufferSize;
    size_t frameBufferWidth;
    size_t frameBufferHeight;
    size_t frameBufferDepth;
};

void PTWeekend::setup()
{
    frameBufferWidth = getWindowWidth();
    frameBufferHeight = getWindowHeight();
    frameBufferDepth = 3;
    frameBufferSize = frameBufferWidth * frameBufferHeight * frameBufferDepth;
    
    frameBuffer = (char*)malloc(frameBufferSize * sizeof(char));
    
    imgTex = gl::Texture2d::create((void*)0, GL_RGB, frameBufferWidth, frameBufferHeight);
    imgTex->setTopDown(true);
    
    //imgTex->update(frameBuffer, GL_RGB, GL_UNSIGNED_BYTE, 0, frameBufferWidth, frameBufferHeight);
    
    //pt::test::test_gradient_ppm(frameBufferWidth, frameBufferHeight);
    //pt::test::test_ray_ppm(frameBufferWidth, frameBufferHeight);
    //pt::test::test_sphere_ppm(frameBufferWidth, frameBufferHeight);
    //pt::test::test_normals_ppm(frameBufferWidth, frameBufferHeight);
    //pt::test::test_primitive_list_ppm(frameBufferWidth, frameBufferHeight);
    //pt::test::test_noise_ppm(frameBufferWidth, frameBufferHeight);
    //pt::test::test_aa_ppm(frameBufferWidth, frameBufferHeight, 100);
    //pt::test::test_diffuse_ppm(frameBufferWidth, frameBufferHeight, 32);
    pt::test::test_diffuse_metal_ppm(frameBufferWidth, frameBufferHeight, 32);
    
    

//    auto img = loadImage(fs::path("gradient.ppm"));
//    imgTex = gl::Texture2d::create(img);
    
}

void PTWeekend::mouseDown( MouseEvent event ) { }

void PTWeekend::update() { }

void PTWeekend::draw()
{
    gl::clear( Color( 0, 0, 0 ) );
    gl::disable(GL_CULL_FACE);
    
    gl::color(1, 1, 1);
    
    gl::draw(imgTex, Rectf(0, 0, getWindowWidth(), getWindowHeight()));
}

void PTWeekend::cleanup()
{
    free(frameBuffer);
}

CINDER_APP(PTWeekend, RendererGl, [](App::Settings* settings) {
//    settings->setConsoleWindowEnabled(true);
})
