#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "ptUtil.h"
#include "ptTests.h"
#include "ptCL.h"
#include "ptGeometry.h"
#include "cinder/CameraUi.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace pt;
using namespace pt::test;


class PTWeekend : public App {
public:
    void setup() override;
    void mouseDown( MouseEvent event ) override;
    void update() override;
    void draw() override;
    void cleanup() override;
    
private:
    
    CameraPersp camera;
    CameraUi cameraUI;
    
    cl_int clStatus;
    
    cl::Device device;
    cl::Context context;
    cl::Program program;
    cl::Kernel kernel;
    cl::CommandQueue cmd_queue;
    
    gl::Texture2dRef imgTex;
    std::vector<cl::Memory> img_buffer;
    cl::Buffer cam_buffer;
    cl::Buffer primitive_buffer;
    cl::Buffer material_buffer;
    cl::Buffer sky_buffer;
    
    size_t local_size;
    size_t local_width;
    size_t img_width;
    size_t img_height;
    
};

void PTWeekend::setup()
{
    /* Scene data */
    camera.lookAt(glm::vec3(-2,1,1), vec3(0, 0, -1.0f), vec3(0,1,0));
    camera.setPerspective( 45.0f, getWindowAspectRatio(), 0.01f, 100.0f );
    cameraUI = CameraUi(&camera, getWindow());
    
    std::vector<fSphereRef> list;
    std::vector<fLambertianRef> materials;
    
    list.push_back(fSphereRef(new Sphere<float>(glm::vec3(1.0,0,-1.0f), 0.5f)));
    list.push_back(fSphereRef(new Sphere<float>(glm::vec3(-1.0,0,-1.0f), 0.5f)));
    list.push_back(fSphereRef(new Sphere<float>(glm::vec3(0,-100.5f, 1.0f), 100.0f)));
    
    materials.push_back(fLambertianRef(new Lambertian<float>(glm::vec3(1.0f, 0, 0))));
    materials.push_back(fLambertianRef(new Lambertian<float>(glm::vec3(0, 1, 0))));
    materials.push_back(fLambertianRef(new Lambertian<float>(glm::vec3(0.5f))));
    
    glm::vec3 bottom_sky_color(1.0, 1.0, 1.0);
    glm::vec3 top_sky_color(0.5, 0.7, 1.0);
    
    CGLContextObj glContext = CGLGetCurrentContext();
    CGLShareGroupObj shareGroup = CGLGetShareGroup(glContext);
    
    GLuint imgTexName;
    
    const char* program_file_str = "../../../assets/path_tracing.cl";
    
    /* Obtain a platform */
    std::vector<cl::Platform> platforms;
    clStatus = cl::Platform::get(&platforms);
    pt_assert(clStatus, "Could not find an OpenCL platform.");
    
    /* Obtain a device and determinte max local size */
    std::vector<cl::Device> devices;
    clStatus = platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
    pt_assert(clStatus, "Could not find a GPU device.");
    device = devices[0];
    
    /* Create an OpenCL context for the device */
    cl_context_properties properties[] = {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
        (cl_context_properties)shareGroup,
        0
    };
    
    context = cl::Context({device}, properties, NULL, NULL, &clStatus);
    pt_assert(clStatus, "Could not create a context for device.");
    
    /* Load and build a program */
    std::ifstream program_file(program_file_str);
    std::string program_str(std::istreambuf_iterator<char>(program_file), (std::istreambuf_iterator<char>()));
    cl::Program::Sources sources(1, std::make_pair(program_str.c_str(), program_str.length() + 1));
    program = cl::Program(context, sources);
    clStatus = program.build({device}, "-I ../../../assets/");
    
    if (clStatus != CL_SUCCESS)
    {
        std::string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        std::cerr << log << "\n";
        exit(EXIT_FAILURE);
    }
    
    /* Create command queue */
    cmd_queue = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &clStatus);
    pt_assert(clStatus, "Could not create command queue");
    
    /* create kernel and set the kernel arguments */
    kernel = cl::Kernel(program, "path_tracing", &clStatus);
    pt_assert(clStatus, "Could not create kernel");
    
    img_width = getWindowWidth();
    img_height = getWindowHeight();
    local_size = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(); //TODO: throws
    local_width = (size_t)pow(2, ceilf(log2f((floorf(sqrtf(local_size))))));
    unsigned int samples = 8;
    
    /* Create GL texture and CL wrapper */
    glGenTextures(1, &imgTexName);
    glBindTexture(GL_TEXTURE_2D, imgTexName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    imgTex = gl::Texture2d::create(GL_TEXTURE_2D, imgTexName, img_width, img_height, true);
    glFinish();
    
    img_buffer.push_back(cl::Image2DGL(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, imgTexName, &clStatus));
    pt_assert(clStatus, "Could not create buffer");
    
    /* Create all buffers */
    cam_buffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_pinhole_cam), NULL, &clStatus);
    pt_assert(clStatus, "Could not create camera buffer");
    
    primitive_buffer = cl::Buffer (context, CL_MEM_READ_ONLY, MAX_PRIMITIVES * sizeof(cl_sphere), NULL, &clStatus);
    pt_assert(clStatus, "Could not create primitive buffer");
    
    material_buffer = cl::Buffer (context, CL_MEM_READ_ONLY, MAX_PRIMITIVES * sizeof(cl_material), NULL, &clStatus);
    pt_assert(clStatus, "Could not create primitive buffer");
    
    sky_buffer = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_sky_material), NULL, &clStatus);
    pt_assert(clStatus, "Could not create sky buffer");
    
    /* Upload scene (static) */
    
    pt_assert(cl_set_sphere_and_material_list(list, materials, primitive_buffer, material_buffer, cmd_queue),
              "Could not fill primitives or materials buffers");
    pt_assert(cl_set_skycolors(bottom_sky_color, top_sky_color, sky_buffer, cmd_queue),
              "Could not fill sky buffer");
    
    clStatus = kernel.setArg(1, primitive_buffer);
    pt_assert(clStatus, "Could not set primitive buffer argument");
    
    clStatus = kernel.setArg(2, material_buffer);
    pt_assert(clStatus, "Could not set primitive buffer argument");
    
    clStatus = kernel.setArg(3, sky_buffer);
    pt_assert(clStatus, "Could not set primitive buffer argument");
    
    clStatus = kernel.setArg(4, list.size());
    pt_assert(clStatus, "Could not set primitive count argument");
    
    clStatus = kernel.setArg(5, img_buffer[0]);
    pt_assert(clStatus, "Could not set img buffer argument");
    
    
    clStatus = kernel.setArg(6, samples);
    pt_assert(clStatus, "Could not set samples argument");
    
    clStatus = kernel.setArg(0, cam_buffer);
    pt_assert(clStatus, "Could not set camera buffer argument");
}

void PTWeekend::mouseDown( MouseEvent event ) { }

void PTWeekend::update() { }

void PTWeekend::draw()
{
    /*
     * BEGIN - each frame part
     */
    
    /* Enqueue kernel for execution */
    
    glm::vec3 origin,lower_left, hor, ver;
    
    float theta = camera.getFov() * M_PI / 180.0f;
    float half_height = tan(theta / 2.0f);
    float half_width = camera.getAspectRatio() * half_height;
    
    origin = camera.getEyePoint();
    glm::vec3 u, v, w;
    
    w = -glm::normalize(camera.getViewDirection()); //odd...
    u = glm::normalize(glm::cross(glm::vec3(0,1,0), w));
    v = glm::cross(w, u);
    
    lower_left = origin - half_width * u - half_height * v - w;
    hor = 2.0f * half_width * u;
    ver = 2.0f * half_height * v;
    
    pt_assert(cl_set_pinhole_cam_arg(origin, lower_left, hor, ver, cam_buffer, cmd_queue), "Could not fill camera buffer");
    
    clStatus = cmd_queue.enqueueAcquireGLObjects(&img_buffer, NULL, NULL);
    pt_assert(clStatus, "Could not acquire gl objects");
    
    cl::Event profiling_evt;
    
    clStatus = cmd_queue.enqueueNDRangeKernel(kernel,
                                              cl::NDRange(0,0),
                                              cl::NDRange(img_width, img_height),
                                              cl::NDRange(local_width,local_size / local_width),
                                              NULL,
                                              &profiling_evt);
    profiling_evt.wait();
    
    pt_assert(clStatus, "Could not enqueue the kernel");
    clStatus = cmd_queue.enqueueReleaseGLObjects(&img_buffer, NULL, NULL);
    pt_assert(clStatus, "Could not release gl objects");
    cmd_queue.finish();
    
    cl_ulong time_start = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    cl_ulong time_end = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_END>();
    cl_ulong total_time = time_end - time_start;
    std::cout << "Total time: " << total_time * 0.001 * 0.001 << " ms \n";
    
    /*
     * END - each frame part
     */
    
    gl::draw(imgTex, Rectf(0, 0, getWindowWidth(), getWindowHeight()));
}

void PTWeekend::cleanup()
{
    GLuint textureName = imgTex->getId();
    glDeleteTextures(1, &textureName);
}

CINDER_APP(PTWeekend, RendererGl, [](App::Settings* settings) {
//    settings->setConsoleWindowEnabled(true);
})
