#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/TriMesh.h"
#include <glm/gtx/intersect.hpp>

using namespace ci;
using namespace ci::app;
using namespace std;

typedef struct Slab
{
    int x_samples;
    int y_samples;
    float scale;
    float z;
} Slab;

typedef struct Lfd
{
    Slab st;
    Slab uv;
    
    std::vector<gl::Texture2dRef> samples;
    std::vector<gl::VboMeshRef> sampleQuads;
    std::vector<bool> shouldRender;
    
    std::string dir;
    std::string basename;
    std::string ext;
    
} Lfd;

class LightFieldsVizApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void drawFbo();
    void updateSceneParams();
    
    void mouseMove( MouseEvent event ) override;
    void keyDown(KeyEvent ke) override;
    
private:
    mat4				mTransform;
    
    gl::BatchRef		mWirePlane;
    
    CameraPersp			mCamera;
    CameraUi			mCamUi;
    
    // Lightfield scene
    CameraPersp         mViewer; //viewer of light field
    vec3                mViewerCenter;
    vec3                mViewerTarget;
    gl::BatchRef        mViewerImgPlane;
    gl::BatchRef        mViewerImgPlaneTex;
    vec3                mViewerImgPlaneAxisX;
    vec3                mViewerImgPlaneAxisY;
    float               mViewerVertFovDeg;
    
    Lfd                 lfd;
    gl::BatchRef        mSTPlane;
    gl::BatchRef        mFocusPlane;
    vec3                mFocusPlaneOrigin;
    vec3                mFocusPlaneDirection;
    vec3                mFocusPlaneAxisX;
    vec3                mFocusPlaneAxisY;
    gl::BatchRef        mQuad;
    //gl::VboMeshRef      mVboQuad;
    float               mFilterDelta;
    
    CameraPersp         mSampleCam;
    float               mSampleCamFov; //vertical, in degrees
    float               mSampleCamAspect;
    gl::BatchRef        mSampleImgPlane;
    gl::BatchRef        mSampleImgPlaneTex;
    vec3                mSampleImgPlaneAxisX;
    vec3                mSampleImgPlaneAxisY;
    gl::Texture2dRef    mSampleTex;

    // Fbo
    gl::FboRef             mFbo;
    
    ivec2				mMousePos;
    
    gl::GlslProgRef colorShader;
    gl::GlslProgRef texShader;
    
    glm::vec3 mUp;
    glm::vec3 mForward;
    
    bool shouldUpdate;
    bool drawSamples;
};

void LightFieldsVizApp::setup()
{
    lfd.st.x_samples = 16; lfd.st.y_samples = 16;
    lfd.uv.x_samples = 820; lfd.uv.y_samples = 820;
    lfd.st.scale = 2.0f; lfd.st.z = 0.0f;
    lfd.uv.scale = 1; lfd.uv.z = 1;
    
    drawSamples = true;
    
    lfd.basename = "frame_s_";
    lfd.dir = "dragon-70/";
    lfd.ext = ".png";
    
    mUp = glm::vec3(0,1,0);
    mForward = vec3(0,0,-1);
    
    mViewerVertFovDeg = 50.0f;
    mFilterDelta = 0.3f;
    
    gl::Fbo::Format format;
    format.setSamples(2);
    mFbo = gl::Fbo::create(getWindowWidth(), getWindowHeight(), format.depthTexture());
    
    // Set up the UI camera.
    mCamera.lookAt( vec3( 2.0f, 3.0f, 1.0f ), vec3( 0 ) );
    mCamera.setPerspective( 40.0f, getWindowAspectRatio(), 0.01f, 100.0f );
    mCamUi = CameraUi( &mCamera, getWindow() );
    
    colorShader = gl::getStockShader( gl::ShaderDef().color() );
    texShader = gl::getStockShader(gl::ShaderDef().texture());
    
    mWirePlane = gl::Batch::create( geom::WirePlane().size( vec2( 10 ) ).subdivisions( ivec2( 10 ) ), colorShader );

    // Setup lightfield scene
    mViewerCenter = vec3(0, 0, 3.0f);
    mViewerTarget = vec3(0.0f,0.0f,0);
    mFocusPlaneOrigin = vec3(0, 0, -5);
    mFocusPlaneDirection = vec3(0, 0, -1);
    mSampleCamFov = 90.0f;
    
    int f = 1;
    for (int s = 0; s < lfd.st.x_samples; ++s) {
        for(int t = 0; t < lfd.st.y_samples; ++t) {
            std::stringstream ss;
            //ss << setfill('0') << setw(2) << f;
            ss << lfd.dir << lfd.basename << t << "_t_" << s << lfd.ext;
            std::string file = ss.str();
            auto img = loadImage(loadAsset(file));
            lfd.samples.push_back(gl::Texture2d::create(img));
            f++;
            lfd.shouldRender.push_back(false);
        }
    }
    
    shouldUpdate = true;
    
    updateSceneParams();
    
}

void LightFieldsVizApp::updateSceneParams()
{
    
    shouldUpdate = true;
}

void LightFieldsVizApp::update()
{
    if(!shouldUpdate) return;
    
    
    
    auto plane = geom::Plane().size(vec2(2 * mFilterDelta, 2 * mFilterDelta))
    .subdivisions(ivec2(1))
    .origin(vec3(0,0,0))
    .normal(mForward);
    
    lfd.sampleQuads.clear();
    
    
    for (int s = 0; s < lfd.st.x_samples; ++s) {
        for(int t = 0; t < lfd.st.y_samples; ++t) {
            
            vector<gl::VboMesh::Layout> bufferLayout = {
                gl::VboMesh::Layout().usage( GL_DYNAMIC_DRAW ).attrib( geom::Attrib::POSITION, 3 ),
                gl::VboMesh::Layout().usage( GL_DYNAMIC_DRAW ).attrib( geom::Attrib::TEX_COORD_0, 2 )
            };
            
            
            gl::VboMeshRef mVboQuad = gl::VboMesh::create(plane, bufferLayout);
            lfd.sampleQuads.push_back(mVboQuad);
            
            auto mappedPosAttrib = mVboQuad->mapAttrib3f( geom::Attrib::POSITION, false );
            mappedPosAttrib[0] = vec3(-mFilterDelta, mFilterDelta, 0);
            mappedPosAttrib[1] = vec3(mFilterDelta, mFilterDelta, 0);
            mappedPosAttrib[2] = vec3(-mFilterDelta, -mFilterDelta, 0);
            mappedPosAttrib[3] = vec3(mFilterDelta, -mFilterDelta, 0);
            mappedPosAttrib.unmap();
            
            auto mappedTexAttrib = mVboQuad->mapAttrib2f( geom::Attrib::TEX_COORD_0, false );
            mappedTexAttrib[0] = vec2(0, 1);
            mappedTexAttrib[1] = vec2(1, 1);
            mappedTexAttrib[2] = vec2(0, 0);
            mappedTexAttrib[3] = vec2(1, 0);
            mappedTexAttrib.unmap();
            
            
        }
    }
    
    mViewer.lookAt(mViewerCenter, mViewerTarget, mUp);
    
    mViewer.setPerspective(mViewerVertFovDeg, getWindowAspectRatio(), 0.01f, 100.0f);
    
    // view dir = target - eye center
    // focal length = 0.5 h / tan(0.5 * FOV)
    // h = 1, w = aspect_ration(i.e. w/h)
    
    mViewerImgPlaneAxisX = glm::cross(mViewer.getViewDirection(), mUp);
    mViewerImgPlaneAxisY = glm::cross(mViewerImgPlaneAxisX, mViewer.getViewDirection());
    mViewerImgPlane = gl::Batch::create(geom::WirePlane().size(vec2(getWindowAspectRatio(), 1.0f))
                                        .subdivisions(ivec2(2))
                                        .origin(mViewer.getEyePoint() + mViewer.getViewDirection() * mViewer.getFocalLength())
                                        .normal(mViewer.getViewDirection())
                                        .axes(mViewerImgPlaneAxisX, mViewerImgPlaneAxisY), colorShader);
    
    gl::Batch::AttributeMapping mapping( { { geom::Attrib::TEX_COORD_0, "ciTexCoord0" } } );
    mViewerImgPlaneTex = gl::Batch::create(geom::Plane().size(vec2(getWindowAspectRatio(), 1.0f))
                                           .subdivisions(ivec2(1))
                                           .origin(mViewer.getEyePoint() + mViewer.getViewDirection() * mViewer.getFocalLength())
                                           .normal(mViewer.getViewDirection())
                                           .axes(mViewerImgPlaneAxisX, mViewerImgPlaneAxisY), texShader, mapping);
    
    
    mSTPlane = gl::Batch::create(geom::WirePlane()
                                 .size(2.0f * vec2(lfd.st.scale, lfd.st.scale))
                                 .origin(vec3(0,0,lfd.st.z))
                                 .subdivisions(ivec2(lfd.st.x_samples - 1, lfd.st.y_samples - 1))
                                 .normal(mForward), colorShader);
    
    
    
    mQuad = gl::Batch::create(geom::Plane()
                              .size(vec2(2 * mFilterDelta, 2 * mFilterDelta))
                              .origin(vec3(0,0,0))
                              .normal(mForward), colorShader);
    
    mFocusPlaneAxisX = glm::cross(mFocusPlaneDirection, mUp);
    mFocusPlaneAxisY = glm::cross(mFocusPlaneAxisX, mFocusPlaneDirection);
    
    mFocusPlane = gl::Batch::create(geom::WirePlane()
                                    .size(vec2(8,8))
                                    .origin(mFocusPlaneOrigin)
                                    .subdivisions(ivec2(2,2))
                                    .normal(mFocusPlaneDirection)
                                    .axes(mFocusPlaneAxisX, mFocusPlaneAxisY), colorShader);
    
    
    mSampleCamAspect = (float)lfd.uv.x_samples / (float)lfd.uv.y_samples;
    mSampleCam.setPerspective(mSampleCamFov, mSampleCamAspect, 0.01f, 0.25f);
    mSampleCam.setFarClip(0.25f * mSampleCam.getFocalLength());
    mSampleCam.setViewDirection(mForward);
    
    mSampleImgPlaneAxisX = glm::cross(mSampleCam.getViewDirection(), mUp);
    mSampleImgPlaneAxisY = glm::cross(mSampleImgPlaneAxisX, mSampleCam.getViewDirection());
    mSampleImgPlane = gl::Batch::create(geom::WirePlane().size(0.25f * vec2(mSampleCamAspect, 1.0f))
                                        .subdivisions(ivec2(2))
                                        .origin(0.25f * mSampleCam.getViewDirection() * mSampleCam.getFocalLength())
                                        .normal(mSampleCam.getViewDirection())
                                        .axes(mSampleImgPlaneAxisX, mSampleImgPlaneAxisY), colorShader);
    
    mSampleImgPlaneTex = gl::Batch::create(geom::Plane().size(0.25f * vec2(mSampleCamAspect, 1.0f))
                                           .subdivisions(ivec2(1))
                                           .origin(0.25f * mSampleCam.getViewDirection() * mSampleCam.getFocalLength())
                                           .normal(mSampleCam.getViewDirection())
                                           .axes(mSampleImgPlaneAxisX, mSampleImgPlaneAxisY), texShader, mapping);
    
    gl::disableAlphaBlending();
    
}

void LightFieldsVizApp::drawFbo()
{
    gl::ScopedFramebuffer fbScp(mFbo); //auto restore previous FBO
    gl::clear(Color::black());
    gl::ScopedViewport scpVp( ivec2( 0 ), mFbo->getSize() );
    
    gl::ScopedDepth d1(true);
    {
    
        gl::setMatrices(mViewer);
        
        float ds = 2.0f * lfd.st.scale / (lfd.st.x_samples - 1);
        float dt = 2.0f * lfd.st.scale / (lfd.st.y_samples - 1);
        for (int s = 0; s < lfd.st.x_samples; ++s) {
            for(int t = 0; t < lfd.st.y_samples; ++t) {
                
//                if(lfd.shouldRender[s + t * lfd.st.x_samples])
//                {
                    glm::vec3 pt(s * ds - lfd.st.scale, t * dt - lfd.st.scale, lfd.st.z);
                    
                    gl::VboMeshRef mVboQuad = lfd.sampleQuads[s + t * lfd.st.x_samples];
                    
                    gl::pushModelMatrix();
                    gl::translate(pt);
                
                if(drawSamples)
                {
                    gl::color((float)s / (float)lfd.st.x_samples, (float)t / (float)lfd.st.y_samples,1);
                    gl::ScopedGlslProg glslScope(colorShader);
                    gl::draw(mVboQuad);
                }
                else
                {
                    gl::color(1,1,1);
                    gl::ScopedGlslProg glslScope(texShader);
                    lfd.samples[s + t * lfd.st.x_samples]->bind();
                    gl::draw(mVboQuad);
                    lfd.samples[s + t * lfd.st.x_samples]->unbind();
                }
                
                
                    

                    gl::popModelMatrix();
//                }
            }
        }
    }
}

void LightFieldsVizApp::draw()
{
    if(!shouldUpdate)
        this->drawFbo();
    shouldUpdate = false;
    
    gl::clear(Color::gray(0.5f));
    
    gl::ScopedMatrices push;
    gl::setMatrices(mCamera);
    
    gl::ScopedDepth d1(true);
    {
        float ds = 2.0f * lfd.st.scale / (lfd.st.x_samples - 1);
        float dt = 2.0f * lfd.st.scale / (lfd.st.y_samples - 1);
        
        //int s = 3; int t = 5;
        for (int s = 0; s < lfd.st.x_samples; ++s) {
            for(int t = 0; t < lfd.st.y_samples; ++t) {
                glm::vec3 pt(s * ds - lfd.st.scale, t * dt - lfd.st.scale, lfd.st.z);
                vec3 pp = mViewer.worldToNdc(pt); // both hor and ver coord are in [-1, 1]
                
//                if (fabsf(pp.x) <= 1.0f && fabsf(pp.y) <= 1.0f)
//                {
                    pp.z = 0;
                    
                    pp.x = max(-1.0f, min(1.0f, pp.x)) / 2.0f;
                    pp.y = max(-1.0f, min(1.0f, pp.y)) / 2.0f;
                    pp.x *= mViewer.getAspectRatio();
                    
                    pp = pp.x * mViewerImgPlaneAxisX + pp.y * mViewerImgPlaneAxisY;
                    
                    pp += (mViewer.getEyePoint() + mViewer.getViewDirection() * mViewer.getFocalLength());
                    
                    vec3 d = glm::normalize(pt - mViewer.getEyePoint());
                    vec3 focusPt;
                    float focusDist;
                    glm::intersectRayPlane(mViewer.getEyePoint(), d, mFocusPlaneOrigin, -mFocusPlaneDirection, focusDist);
                    focusPt = mViewer.getEyePoint() + focusDist * d;
                    
                    std::vector<vec3> quad;
                    quad.push_back(pt + vec3(mFilterDelta, mFilterDelta, 0));
                    quad.push_back(pt + vec3(-mFilterDelta, mFilterDelta, 0));
                    quad.push_back(pt + vec3(mFilterDelta, -mFilterDelta, 0));
                    quad.push_back(pt + vec3(-mFilterDelta, -mFilterDelta, 0));
                    
                    std::vector<vec3> quad_focusplane;
                    for(auto quad_point : quad)
                    {
                        vec3 d = glm::normalize(quad_point - mViewer.getEyePoint());
                        glm::intersectRayPlane(mViewer.getEyePoint(), d, mFocusPlaneOrigin, -mFocusPlaneDirection, focusDist);
                        quad_focusplane.push_back(mViewer.getEyePoint() + focusDist * d);
                    }
                    
                    /*
                     * pt -> point on the camera surface, world space
                     * also draw the quad around it, determined by the filter aperture
                     */
                    gl::color(1.0f, 0, 0);
                    gl::drawSphere(pt, 0.02f, 8);
                    
                    gl::color(1.0f, 0.25f, 0.25f);
                    for(auto quad_point : quad)
                    {
                        gl::drawSphere(quad_point, 0.02f, 8);
                    }

                    /*
                     * projection on viewer image plane
                     */
                    gl::ScopedColor c2(0, 1.0f, 0);
                    gl::drawSphere(pp, 0.02f, 8);
                    
                    gl::color(0.25f, 1.0f, 0.25f);
                    for(auto quad_point : quad_focusplane)
                    {
                        gl::drawSphere(quad_point, 0.02f, 8);
                    }
                    
                    gl::ScopedColor c3( Color::gray( 0.75f ) );
                    
                    //gl::drawLine(mViewer.getEyePoint(), mViewer.getEyePoint() + 10.0f * d);
                    
                    std::vector<vec2> texcoords;
                    for(vec3 quad_point : quad_focusplane)
                    {
                        //gl::drawLine(pt, quad_point);
                        
                        vec3 qpp = mSampleCam.worldToNdc(quad_point); // both hor and ver coord are in [-1, 1]
                        
                        vec2 uv(qpp.x * 0.5f + 0.5f, qpp.y * 0.5f + 0.5f);
                        texcoords.push_back(uv);
                    }
                    
                    gl::VboMeshRef mVboQuad = lfd.sampleQuads[s + t * lfd.st.x_samples];
                    
                    auto mappedTexAttrib = mVboQuad->mapAttrib2f( geom::Attrib::TEX_COORD_0, false );
                    mappedTexAttrib[0] = texcoords[1];
                    mappedTexAttrib[1] = texcoords[0];
                    mappedTexAttrib[2] = texcoords[3];
                    mappedTexAttrib[3] = texcoords[2];
//                    mappedTexAttrib[0] = vec2(0, 1);
//                    mappedTexAttrib[1] = vec2(1, 1);
//                    mappedTexAttrib[2] = vec2(0, 0);
//                    mappedTexAttrib[3] = vec2(1, 0);
                    mappedTexAttrib.unmap();
                    
                    mSampleCam.setEyePoint(pt);
                    gl::drawFrustum(mSampleCam);
                    
                    gl::ScopedColor c5(0, 1.0f, 0);
                    gl::drawSphere(focusPt, 0.02f, 8);
                    
                    gl::pushModelMatrix();
                    gl::translate(mSampleCam.getEyePoint());
                    
                    lfd.samples[s + t * lfd.st.x_samples]->bind();
                    mSampleImgPlaneTex->draw();
                    lfd.samples[s + t * lfd.st.x_samples]->unbind();
                    
                    lfd.shouldRender[s + t * lfd.st.x_samples] = true;
                    
                    mSampleImgPlane->draw();
                    gl::popModelMatrix();
                    
//                }
//                else
//                {
//                    lfd.shouldRender[s + t * lfd.st.x_samples] = false;
//                }
            }
        }
        
        
        
        
        gl::ScopedColor c3( Color::gray( 0.25f ) );
        mFocusPlane->draw();
        
        gl::ScopedColor c2( Color::gray( 0.75f ) );
        mWirePlane->draw();
        
        mFbo->bindTexture();
        mViewerImgPlaneTex->draw();
        mViewerImgPlane->draw();
        
        mSTPlane->draw();
        
        gl::drawFrustum(mViewer);
        
        
    }
    
    gl::ScopedDepth d2(false);
    {
        vec3 img_c = mViewer.getEyePoint() + mViewer.getViewDirection() * mViewer.getFocalLength();
        
        gl::drawCoordinateFrame(0.5f, 0.06f, 0.02f);
        
        gl::color(1, 0, 0);
        gl::drawVector(mFocusPlaneOrigin, mFocusPlaneOrigin + mFocusPlaneAxisX);
        gl::drawVector(img_c, img_c + 0.5f * mViewerImgPlaneAxisX, 0.1f, 0.025f);
        gl::color(0, 1, 0);
        gl::drawVector(mFocusPlaneOrigin, mFocusPlaneOrigin + mFocusPlaneAxisY);
        gl::drawVector(img_c, img_c + 0.5f * mViewerImgPlaneAxisY, 0.1f, 0.025f);
    }
    
    gl::setMatricesWindow( toPixels( getWindowSize() ) );
    gl::color(Color::white());
    gl::draw(mFbo->getColorTexture(), Rectf(0, 0, mFbo->getWidth() / 2.0f, mFbo->getHeight() / 2.0f));
    
}

void LightFieldsVizApp::keyDown(KeyEvent ke)
{
    if(ke.getCode() == KeyEvent::KEY_LEFT)
    {
        mViewerCenter.x -= 0.1f;
        mViewer.setEyePoint(mViewerCenter);
        updateSceneParams();
    }
    if(ke.getCode() == KeyEvent::KEY_RIGHT)
    {
        mViewerCenter.x += 0.1f;
        mViewer.setEyePoint(mViewerCenter);
        updateSceneParams();
    }
    if(ke.getCode() == KeyEvent::KEY_DOWN)
    {
        mViewerCenter.y -= 0.1f;
        mViewer.setEyePoint(mViewerCenter);
        updateSceneParams();
    }
    if(ke.getCode() == KeyEvent::KEY_UP)
    {
        mViewerCenter.y += 0.1f;
        mViewer.setEyePoint(mViewerCenter);
        updateSceneParams();
    }
    if(ke.getCode() == KeyEvent::KEY_s)
    {
        mFocusPlaneOrigin.z -= 1;
        //mViewerTarget = mFocusPlaneOrigin;
        //mViewer.lookAt(mFocusPlaneOrigin);
        updateSceneParams();
    }
    if(ke.getCode() == KeyEvent::KEY_w)
    {
        mFocusPlaneOrigin.z += 1;
        //mViewerTarget = mFocusPlaneOrigin;
        //mViewer.lookAt(mFocusPlaneOrigin);
        updateSceneParams();
    }
    if(ke.getCode() == KeyEvent::KEY_e)
    {
        mViewerCenter.z -= 0.1f;
        updateSceneParams();
    }
    if(ke.getCode() == KeyEvent::KEY_d)
    {
        mViewerCenter.z += 0.1f;
        updateSceneParams();
    }
    if(ke.getCode() == KeyEvent::KEY_z)
    {
        mViewerTarget.x -= 0.1f;
        mViewer.lookAt(mViewerTarget);
        updateSceneParams();
    }
    if(ke.getCode() == KeyEvent::KEY_x)
    {
        mViewerTarget.x += 0.1f;
        mViewer.lookAt(mViewerTarget);
        updateSceneParams();
    }
    if(ke.getCode() == KeyEvent::KEY_b)
    {
        drawSamples = !drawSamples;
    }
}

void LightFieldsVizApp::mouseMove( MouseEvent event )
{
    mMousePos = event.getPos();
}

CINDER_APP( LightFieldsVizApp, RendererGl( RendererGl::Options().msaa( 2 ) ), []( App::Settings* settings )
{
    settings->setWindowSize(800,800);
})
