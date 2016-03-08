#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "PathTracer.h"
#include <atomic>

using namespace ci;
using namespace ci::app;
using namespace std;

class PTApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	void cleanup() override;

private:
	gl::Texture2dRef imgTex;

	std::atomic_bool frameDone;
    bool             renderFrame;

	//RAM image buffer
	char* frameBuffer;
	size_t frameBufferSize;
	size_t frameBufferWidth;
	size_t frameBufferHeight;
	size_t frameBufferDepth;
	std::shared_ptr<std::thread> frameThread;

	//Scene for path tracing
	pt::Scene	ptScene;
};

void PTApp::setup()
{
	ptScene.renderables.push_back(pt::CreateSphereRenderable(
		glm::vec3(1e5 + 1, 40.8, 81.6), 1e5,
		glm::vec3(.999,0,0), pt::Material::DIFFUSE)); // Left

	ptScene.renderables.push_back(pt::CreateSphereRenderable(
		glm::vec3(-1e5 + 99, 40.8, 81.6), 1e5,
		glm::vec3(0,.999,0), pt::Material::DIFFUSE)); // Right

	ptScene.renderables.push_back(pt::CreateSphereRenderable(
		glm::vec3(50, 40.8, 1e5), 1e5,
		glm::vec3(.75, .75, .75), pt::Material::DIFFUSE)); // Back

	ptScene.renderables.push_back(pt::CreateSphereRenderable(
		glm::vec3(50, 40.8, -1e5 + 170), 1e5,
		glm::vec3(0,0,0), pt::Material::DIFFUSE)); // Front

	ptScene.renderables.push_back(pt::CreateSphereRenderable(
		glm::vec3(50, 1e5, 81.6), 1e5,
		glm::vec3(.75, .75, .75), pt::Material::DIFFUSE)); // Bottom

	ptScene.renderables.push_back(pt::CreateSphereRenderable(
		glm::vec3(50, -1e5 + 81.6, 81.6), 1e5,
		glm::vec3(.75, .75, .75), pt::Material::DIFFUSE)); // Top

	ptScene.renderables.push_back(pt::CreateSphereRenderable(
		glm::vec3(27, 16.5, 47), 16.5,
		glm::vec3(.999), pt::Material::DIFFUSE)); // Object

	ptScene.renderables.push_back(pt::CreateSphereRenderable(
		glm::vec3(73, 16.5, 78), 16.5,
		glm::vec3(.999), pt::Material::DIFFUSE)); // Object

	ptScene.renderables.push_back(pt::CreateSphereRenderable(
		glm::vec3(50,81.6-16.5,81.6), 1.5,
		glm::vec3(0), pt::Material::DIFFUSE, glm::vec3(400, 400, 400))); // Light


	frameBufferWidth = getWindowWidth();
	frameBufferHeight = getWindowHeight();
	frameBufferDepth = 3;
	frameBufferSize = frameBufferWidth * frameBufferHeight * frameBufferDepth;

	frameBuffer = (char*)malloc(frameBufferSize * sizeof(char));

	frameDone = false;
    renderFrame = false;

	frameThread = std::shared_ptr<std::thread>(new std::thread([this](){

        pt::PathTracer::Trace(ptScene,
                              frameBufferWidth,
                              frameBufferHeight,
                              4,
                              glm::dvec3(50, 52, 295.6),
                              glm::dvec3(0, -0.042612, -1),
                              .5135,
                              frameBuffer);

		frameDone = true;

	}));
    
    
    
	
	imgTex = gl::Texture2d::create((void*)0, GL_RGB, frameBufferWidth, frameBufferHeight);
    imgTex->setTopDown(true);

}

void PTApp::mouseDown( MouseEvent event )
{
}

void PTApp::update()
{
	if (frameDone)
	{
		imgTex->update(frameBuffer, GL_RGB, GL_UNSIGNED_BYTE, 0, frameBufferWidth, frameBufferHeight);
		frameDone = false;
        renderFrame = true;
	}
}

void PTApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
    gl::disable(GL_CULL_FACE);
    
    gl::color(1, 1, 1);
    
    if(renderFrame)
        gl::draw(imgTex, Rectf(0, 0, getWindowWidth(), getWindowHeight()));
    else
        gl::drawString("progress: ", vec2(5,5));
    
    

}

void PTApp::cleanup()
{
	frameThread->join();
	free(frameBuffer);
}

CINDER_APP( PTApp, RendererGl )
