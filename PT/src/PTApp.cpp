#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "PathTracer.h"
#include <atomic>
#include "cinder/Utilities.h"

//TODO: what to we do on mac exactly?
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

using namespace ci;
using namespace ci::app;
using namespace std;

#define VECTOR_SIZE 1024

void _load(const char* filename, char*& result)
{
	FILE *fp;
	long lSize;

	fp = fopen(filename, "rb");
	if (!fp) perror(filename), exit(1);

	fseek(fp, 0L, SEEK_END);
	lSize = ftell(fp);
	rewind(fp);

	/* allocate memory for entire content */
	result = (char*)calloc(1, lSize + 1);
	if (!result) fclose(fp), fputs("memory alloc fails", stderr), exit(1);

	/* copy the file into the buffer */
	if (1 != fread(result, lSize, 1, fp))
		fclose(fp), free(result), fputs("entire read fails", stderr), exit(1);

	/* do your work here, buffer is a string contains the whole text */

	fclose(fp);
}

void _main()
{
	char* kernelSource;
	_load(getAssetPath("saxpy.kernel").string().c_str(), kernelSource);
	// = loadString(loadAsset()).c_str(); //TODO: remove cinder dep.

	// Allocate memory
	int i;
	float alpha = 2.0;
	float *A = (float*)malloc(sizeof(float)*VECTOR_SIZE);
	float *B = (float*)malloc(sizeof(float)*VECTOR_SIZE);
	float *C = (float*)malloc(sizeof(float)*VECTOR_SIZE);
	for (i = 0; i < VECTOR_SIZE; i++)
	{
		A[i] = i;
		B[i] = VECTOR_SIZE - i;
		C[i] = 0;
	}

	// get device and platform info
	cl_platform_id* platforms = NULL;
	cl_uint num_platforms;
	cl_int clStatus = clGetPlatformIDs(0, NULL, &num_platforms);
	platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
	clStatus = clGetPlatformIDs(num_platforms, platforms, NULL);

	// get device list and pick one
	cl_device_id* device_list = NULL;
	cl_uint num_devices;
	clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
	device_list = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices);
	clStatus = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, num_devices, device_list, NULL);

	// create OpenCL context with all devices for the platform
	cl_context context;
	context = clCreateContext(NULL, num_devices, device_list, NULL, NULL, &clStatus);

	//create a command queue
	cl_command_queue command_queue = clCreateCommandQueue(context, device_list[0], 0, &clStatus);

	//create the memory buffers on device
	cl_mem A_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, VECTOR_SIZE * sizeof(float), NULL, &clStatus);
	cl_mem B_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, VECTOR_SIZE * sizeof(float), NULL, &clStatus);
	cl_mem C_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, VECTOR_SIZE * sizeof(float), NULL, &clStatus);

	//copy to device memory
	clStatus = clEnqueueWriteBuffer(command_queue, A_clmem, CL_TRUE, 0, VECTOR_SIZE * sizeof(float), A, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, B_clmem, CL_TRUE, 0, VECTOR_SIZE * sizeof(float), B, 0, NULL, NULL);
	clStatus = clEnqueueWriteBuffer(command_queue, C_clmem, CL_TRUE, 0, VECTOR_SIZE * sizeof(float), C, 0, NULL, NULL);

	// create kernel program
	cl_program prog = clCreateProgramWithSource(context, 1, (const char **)&kernelSource, NULL, &clStatus);

	clStatus = clBuildProgram(prog, 1, device_list, NULL, NULL, NULL);

	cl_kernel kernel = clCreateKernel(prog, "saxpy_kernel", &clStatus);

	if (clStatus != 0) {
		size_t log_size;
		clGetProgramBuildInfo(prog, device_list[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

		// Allocate memory for the log
		char *log = (char *)malloc(log_size);

		// Get the log
		clGetProgramBuildInfo(prog, device_list[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

		console() << log << "\n";
	}

	// set kernl arguments
	clStatus = clSetKernelArg(kernel, 0, sizeof(float), (void*)&alpha);
	clStatus = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&A_clmem);
	clStatus = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&B_clmem);
	clStatus = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&C_clmem);

	// execute!
	size_t global_size = VECTOR_SIZE;
	size_t local_size = 64;
	clStatus = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);

	clStatus = clEnqueueReadBuffer(command_queue, C_clmem, CL_TRUE, 0, VECTOR_SIZE * sizeof(float), C, 0, NULL, NULL);

	// Clean up and wait for all the comands to complete.
	clStatus = clFlush(command_queue);
	clStatus = clFinish(command_queue);

	// Display the result to the screen
	for (i = 0; i < VECTOR_SIZE; i++)
		console() << C[i] << " ";

	// release resources
	clStatus = clReleaseKernel(kernel);
	clStatus = clReleaseProgram(prog);
	clStatus = clReleaseMemObject(A_clmem);
	clStatus = clReleaseMemObject(B_clmem);
	clStatus = clReleaseMemObject(C_clmem);
	clStatus = clReleaseCommandQueue(command_queue);
	clStatus = clReleaseContext(context);
	free(A);
	free(B);
	free(C);
	free(platforms);
	free(device_list);
}

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
	double			 renderTime;

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

	_main();

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
        
        std::shared_ptr<pt::PathTracer>  _pt = std::shared_ptr<pt::PathTracer>(new pt::PathTracer());

        _pt->Trace(ptScene,
                              frameBufferWidth,
                              frameBufferHeight,
                              8,
                              glm::dvec3(50, 52, 295.6),
                              glm::dvec3(0, -0.042612, -1),
                              .5135,
                              frameBuffer, &renderTime);

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
		console() << "Render time: " << renderTime << "s\n";
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

CINDER_APP(PTApp, RendererGl, [](App::Settings* settings) {
	//settings->setConsoleWindowEnabled(true);
})
