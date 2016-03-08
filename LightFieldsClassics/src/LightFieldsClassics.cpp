#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/TriMesh.h"
#include <glm/gtx/intersect.hpp>
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class LightFieldsClassics : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void drawScene();
    
    void mouseDown(MouseEvent event) override;
    void mouseUp(MouseEvent event) override;
    void mouseWheel(MouseEvent event) override;
    void mouseDrag(MouseEvent event)  override;
    
    void keyDown(KeyEvent ke) override;
    
private:
    
    CameraPersp         mViewer;
    gl::Texture2dRef    mLightfieldTex;
    
    gl::FboRef          mFbo;
    gl::FboRef          mUVFbo;
    
    gl::GlslProgRef     mLightfieldShader;
    gl::GlslProgRef     mUVShader;
    
    int                 mUUVScale;
    int                 mUScreenSize;
    int                 mUUVTex;
    int                 mUTex;
    
    gl::VboMeshRef      mVboQuad;
    
    params::InterfaceGlRef  mParams;
    float                   mFrameRate;
    float                   mViewerFOV;
    vec3                    mViewerPosition;
    vec3                    mViewerOrientation;
    float                   mUVPlaneDepth;
    vec3                    mModelOrientation;
    float                   mSTPlaneScale;
    float                   mUVPlaneScale;
    float                   mRotationPivot;
    
    
    vec3                   mLightfieldUVScale;
    
    
    gl::BatchRef		mWirePlane;
    gl::BatchRef        mSTPlane;
    gl::BatchRef        mUVPlane;
    
    CameraPersp			mCamera;
    CameraUi			mCamUi;
    CameraPersp         mViewerViz;
};

void LightFieldsClassics::setup()
{
    mViewerPosition = vec3(0,0,1.2f);
    mViewerFOV = 45.0f;
    
    mViewer.setPerspective(mViewerFOV, 1.0f, 0.1f, 100.0f);
    mViewer.setEyePoint(mViewerPosition);
    mViewer.setViewDirection(vec3(0,0,-1));
    
    mViewerOrientation = glm::eulerAngles(mViewer.getOrientation());
    
    mModelOrientation = vec3(0,0,0);
    
    mLightfieldUVScale = vec3(-8, 8, 0);
    mUVPlaneDepth = -0.5;
    
    mSTPlaneScale = 1.0f;
    mUVPlaneScale = 1.0f;
    mRotationPivot = mUVPlaneDepth / 2.0f;
    
    mParams = params::InterfaceGl::create("Params", toPixels(ivec2(300, 300)));
    mParams->setPosition(toPixels(ivec2(getWindowHeight() + 5,5)));
    
    mParams->addParam("Frame rate", &mFrameRate, "", true);
    mParams->addParam("FOV", &mViewerFOV, "", false);
    (mParams->addParam("ST Scale", &mSTPlaneScale, false)).step(0.01f);
    (mParams->addParam("UV Scale", &mUVPlaneScale, false)).step(0.01f);
    (mParams->addParam("UV Z", &mUVPlaneDepth, false)).step(0.01f);
    
    mParams->addParam("Viewer position", &mViewerPosition, "", false);
    mParams->addParam("Viewer orientation", &mViewerOrientation, "", false);
    
    (mParams->addParam("Model pivot", &mRotationPivot, false)).step(0.01f);
    mParams->addParam("Model orientation", &mModelOrientation, "", false);
    
    mParams->addButton("Compute pivot", [this](){
       
        mRotationPivot = mUVPlaneDepth / 2.0f;
        
    });
    
    auto img = loadImage(loadAsset("dragon-uv.jpg"));
    mLightfieldTex = gl::Texture2d::create(img);
    mLightfieldTex->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    
    gl::Fbo::Format format;
    format.setSamples(2);
    mFbo = gl::Fbo::create(getWindowHeight(), getWindowHeight(), format.depthTexture());
    mUVFbo = gl::Fbo::create(getWindowHeight(), getWindowHeight(), format.depthTexture());
    
    mLightfieldShader = gl::GlslProg::create(loadAsset("lightfield.vert"), loadAsset("lightfield.frag"));
    mUVShader = gl::GlslProg::create(loadAsset("uv.vert"), loadAsset("uv.frag"));
    
    mLightfieldShader->findUniform("uvScale", &mUUVScale);
    mLightfieldShader->findUniform("screenSize", &mUScreenSize);
    mLightfieldShader->findUniform("uvTex", &mUUVTex);
    mLightfieldShader->findUniform("tex", &mUTex);
    
    vector<gl::VboMesh::Layout> bufferLayout = {
        gl::VboMesh::Layout().usage( GL_DYNAMIC_DRAW ).attrib( geom::Attrib::POSITION, 3 ),
        gl::VboMesh::Layout().usage( GL_DYNAMIC_DRAW ).attrib( geom::Attrib::TEX_COORD_0, 2 )
    };
    
    auto plane = geom::Plane().size(vec2(1,1))
    .subdivisions(ivec2(1))
    .origin(vec3(0,0,0))
    .normal(vec3(0,0,-1));
    
    mVboQuad = gl::VboMesh::create(plane, bufferLayout);
    
    mCamera.lookAt( vec3( 2.0f, 3.0f, 1.0f ), vec3( 0 ) );
    mCamera.setPerspective( 40.0f, (float)(getWindowWidth() - getWindowHeight()) / (float)getWindowHeight(), 0.01f, 100.0f );
    mCamUi = CameraUi(&mCamera);
    mCamUi.setWindowSize(toPixels(ivec2(getWindowWidth() - getWindowHeight(), getWindowHeight())));
    
    
    auto colorShader = gl::getStockShader(gl::ShaderDef().color());
    mWirePlane = gl::Batch::create(geom::WirePlane().size(vec2(10)).subdivisions(ivec2(10)), colorShader);
    
    mSTPlane = gl::Batch::create(geom::WirePlane()
                                 .size(vec2(1,1))
                                 .origin(vec3(0,0,0))
                                 .normal(vec3(0,0,-1)), colorShader);
    
    mUVPlane = gl::Batch::create(geom::WirePlane()
                                 .size(vec2(1,1))
                                 .origin(vec3(0,0,0))
                                 .normal(vec3(0,0,-1)), colorShader);
    
}

void LightFieldsClassics::update()
{
    
}

void LightFieldsClassics::draw()
{
    mFrameRate	= getAverageFps();
    
    mViewer.setPerspective(mViewerFOV, 1.0f, 0.1f, 100.0f);
    mViewer.setEyePoint(mViewerPosition);
    mViewer.setOrientation(glm::toQuat(orientate3(mViewerOrientation)));
    
    gl::clear(Color::gray(0.5f));
    
    gl::disable(GL_CULL_FACE);
    
    // render the plane UV onto a frame buffer, with the perspective baked at camera position
    {
        gl::ScopedFramebuffer fbScp(mUVFbo); //auto restore previous FBO
        gl::clear(Color::black());
        gl::ScopedViewport scpVp(ivec2(0), mUVFbo->getSize());
        
        gl::ScopedGlslProg p(mUVShader);
        gl::ScopedMatrices push;
        gl::setMatrices(mViewer);
        gl::pushModelMatrix();
        
        gl::translate(0, 0, mRotationPivot);
        gl::rotate(glm::toQuat(orientate3(mModelOrientation)));
        gl::translate(0, 0, -mRotationPivot);
        
        gl::scale(vec3(mSTPlaneScale));
        
        gl::draw(mVboQuad);
        gl::popModelMatrix();
    }
    
    {
        gl::ScopedFramebuffer fbScp(mFbo); //auto restore previous FBO
        gl::clear(Color::black());
        gl::ScopedViewport scpVp(ivec2(0), mFbo->getSize());
        
        gl::ScopedGlslProg p(mLightfieldShader);
        gl::ScopedMatrices push;
        gl::setMatrices(mViewer);
        gl::pushModelMatrix();
        
        
        
        gl::translate(0, 0, mRotationPivot);
        gl::rotate(glm::toQuat(orientate3(mModelOrientation)));
        gl::translate(0, 0, -mRotationPivot);
        
        gl::translate(0, 0, mUVPlaneDepth);
        
        gl::scale(vec3(mUVPlaneScale));
                  
        mLightfieldShader->uniform(mUUVScale, vec2(mLightfieldUVScale.x, mLightfieldUVScale.y));
        mLightfieldShader->uniform(mUScreenSize, vec2(getWindowHeight(), getWindowHeight()));
        mLightfieldShader->uniform(mUUVTex, 0);
        mLightfieldShader->uniform(mUTex, 1);
        
        mUVFbo->bindTexture(0);
        mLightfieldTex->bind(1);
        
        gl::draw(mVboQuad);
        gl::popModelMatrix();
    }

    
    {
        gl::ScopedViewport scpVp(0,0, getWindowHeight(), getWindowHeight());
        gl::ScopedMatrices push;
        gl::setMatricesWindow(toPixels(mFbo->getSize()));
        
        gl::color(Color::white());
        gl::draw(mFbo->getColorTexture(), Rectf(0, 0, mFbo->getWidth(), mFbo->getHeight()));
    }
    
    {
        gl::ScopedViewport scpVp(getWindowHeight(),0, getWindowWidth() - getWindowHeight(), getWindowHeight());
        
        gl::ScopedDepth d(true);
        
        gl::ScopedMatrices push;
        gl::setMatrices(mCamera);

        gl::color(Color::gray(0.75f));
        mWirePlane->draw();
        gl::color(1.0f, 0, 1.0f);
        mViewerViz.setPerspective(mViewerFOV, 1.0f, 0.01f, 0.5f);
        mViewerViz.setEyePoint(mViewerPosition);
        mViewerViz.setOrientation(glm::toQuat(orientate3(mViewerOrientation)));
        gl::drawFrustum(mViewerViz);
        
        gl::color(1.0f, 0, 0);
        gl::pushModelMatrix();
        
        gl::translate(0, 0, mRotationPivot);
        gl::rotate(glm::toQuat(orientate3(mModelOrientation)));
        gl::translate(0, 0, -mRotationPivot);
        
        gl::scale(vec3(mSTPlaneScale));
        mSTPlane->draw();
        gl::popModelMatrix();
        
        gl::color(0, 1.0f, 0);
        gl::pushModelMatrix();
        
        
        
        gl::translate(0, 0, mRotationPivot);
        gl::rotate(glm::toQuat(orientate3(mModelOrientation)));
        gl::translate(0, 0, -mRotationPivot);
        
        gl::translate(0, 0, mUVPlaneDepth);
        
        gl::scale(vec3(mUVPlaneScale));
        
        mUVPlane->draw();
        gl::popModelMatrix();
        
        gl::ScopedDepth d2(false);
        gl::pushModelMatrix();
        gl::translate(0, 0, mRotationPivot);
        gl::drawCoordinateFrame(0.5f, 0.1f, 0.025f);
        gl::popModelMatrix();
        mParams->draw();
    }
    
}

void LightFieldsClassics::keyDown(KeyEvent ke)
{
//    if(ke.getCode() == KeyEvent::KEY_LEFT)
//    {
//        mViewerCenter.x -= 0.1f;
//        mViewer.setEyePoint(mViewerCenter);
//        updateSceneParams();
//    }
//    if(ke.getCode() == KeyEvent::KEY_RIGHT)
//    {
//        mViewerCenter.x += 0.1f;
//        mViewer.setEyePoint(mViewerCenter);
//        updateSceneParams();
//    }
//    if(ke.getCode() == KeyEvent::KEY_DOWN)
//    {
//        mViewerCenter.y -= 0.1f;
//        mViewer.setEyePoint(mViewerCenter);
//        updateSceneParams();
//    }
//    if(ke.getCode() == KeyEvent::KEY_UP)
//    {
//        mViewerCenter.y += 0.1f;
//        mViewer.setEyePoint(mViewerCenter);
//        updateSceneParams();
//    }
//    if(ke.getCode() == KeyEvent::KEY_s)
//    {
//        mFocusPlaneOrigin.z -= 1;
//        //mViewerTarget = mFocusPlaneOrigin;
//        //mViewer.lookAt(mFocusPlaneOrigin);
//        updateSceneParams();
//    }
//    if(ke.getCode() == KeyEvent::KEY_w)
//    {
//        mFocusPlaneOrigin.z += 1;
//        //mViewerTarget = mFocusPlaneOrigin;
//        //mViewer.lookAt(mFocusPlaneOrigin);
//        updateSceneParams();
//    }
//    if(ke.getCode() == KeyEvent::KEY_e)
//    {
//        mViewerCenter.z -= 0.1f;
//        updateSceneParams();
//    }
//    if(ke.getCode() == KeyEvent::KEY_d)
//    {
//        mViewerCenter.z += 0.1f;
//        updateSceneParams();
//    }
//    if(ke.getCode() == KeyEvent::KEY_z)
//    {
//        mViewerTarget.x -= 0.1f;
//        mViewer.lookAt(mViewerTarget);
//        updateSceneParams();
//    }
//    if(ke.getCode() == KeyEvent::KEY_x)
//    {
//        mViewerTarget.x += 0.1f;
//        mViewer.lookAt(mViewerTarget);
//        updateSceneParams();
//    }
//    if(ke.getCode() == KeyEvent::KEY_b)
//    {
//        drawSamples = !drawSamples;
//    }
}

void LightFieldsClassics::mouseDown(MouseEvent event)
{
    mCamUi.mouseDown(event);
}

void LightFieldsClassics::mouseUp(MouseEvent event)
{
    mCamUi.mouseUp(event);
}

void LightFieldsClassics::mouseWheel(MouseEvent event)
{
    mCamUi.mouseWheel(event);
}

void LightFieldsClassics::mouseDrag(MouseEvent event)
{
    mCamUi.mouseDrag(event);
}

CINDER_APP( LightFieldsClassics, RendererGl( RendererGl::Options().msaa( 2 ) ), []( App::Settings* settings )
{
    settings->setWindowSize(1000,600);
})
