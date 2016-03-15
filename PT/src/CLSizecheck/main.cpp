#include <iostream>
#include <assert.h>
#include <fstream>
#include <memory>
#include <math.h>
#include "ptTestsCL.h"

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include "../include/cl.hpp"
#else
#include <CL/cl.h>
#include <CL/cl.hpp>
#endif

void assertFatal(const cl_int& status, const std::string& errorMsg)
{
    if (status != CL_SUCCESS)
    {
        std::cerr << errorMsg << "\n";
        exit(EXIT_FAILURE);
    }
}

int main(int argc, const char * argv[])
{
    cl_int clStatus;
    
    size_t img_width = 512;
    size_t img_height = 512;
    
    /* Obtain a platform */
    std::vector<cl::Platform> platforms;
    clStatus = cl::Platform::get(&platforms);
    assertFatal(clStatus, "Could not find an OpenCL platform.");
    
    /* Obtain a device and determinte max local size */
    std::vector<cl::Device> devices;
    clStatus = platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
    assertFatal(clStatus, "Could not find a GPU device.");
    cl::Device& device = devices[0];
    size_t local_size = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(); //TODO: throws
//    size_t global_size = img_width * img_height;
    
    /* Create an OpenCL context for the device */
    cl::Context context({device}, NULL, NULL, NULL, &clStatus); //TODO: add the error handling callback
    assertFatal(clStatus, "Could not create a context for device.");
    
    /* Load and build a program */
    cl::Program program;
    std::ifstream program_file("../../../assets/sizecheck.cl");
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
    
    cl::Buffer result_buffer(context, CL_MEM_WRITE_ONLY, 4 * sizeof(unsigned int), NULL, &clStatus);
    assertFatal(clStatus, "Could not create buffer");
    
    /* Create command queue */
    cl::CommandQueue cmd_queue(context, device, CL_QUEUE_PROFILING_ENABLE, &clStatus);
    assertFatal(clStatus, "Could not create command queue");
    
    /* create kernel and set the kernel arguments */
    cl::Kernel kernel(program, "sizecheck", &clStatus);
    assertFatal(clStatus, "Could not create kernel");
    
    clStatus = kernel.setArg(0, result_buffer);
    assertFatal(clStatus, "Could not set data argument");
    
    
    /* Enqueue kernel for execution */
    cl::Event profiling_evt;
    clStatus = cmd_queue.enqueueNDRangeKernel(kernel,
                                              cl::NDRange(0,0),
                                              cl::NDRange(1, 1),
                                              cl::NDRange(1,1),
                                              NULL,
                                              &profiling_evt);
    
    assertFatal(clStatus, "Could not enqueue the kernel");
    
    /* Wait for process to finish */
    cmd_queue.finish();
    cl_ulong time_start = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    cl_ulong time_end = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_END>();
    cl_ulong total_time = time_end - time_start;
    
    /* read the result */
    
    unsigned int* result = (unsigned int*)malloc(4 * sizeof(unsigned int));
    clStatus = cmd_queue.enqueueReadBuffer(result_buffer, CL_TRUE, 0, 4 * sizeof(unsigned int), result);
    assertFatal(clStatus, "Could not read the buffer");
    
    std::cout << "size of GPU float " << result[0] << "\n";
    std::cout << "size of GPU float3 " << result[1] << "\n";
    std::cout << "size of GPU PinholeCam " << result[2] << "\n";
    std::cout << "size of GPU Sphere " << result[3] << "\n";
    
    std::cout << "size of CPU float " << sizeof(float) << "\n";
    std::cout << "size of CPU float3 " << sizeof(cl_float3) << "\n";
    std::cout << "size of CPU PinholeCam " << sizeof(cl_pinhole_cam) << "\n";
    std::cout << "size of CPU Sphere " << sizeof(cl_sphere) << "\n";
    
    //std::cout << "Total time: " << total_time * 0.001 << " microsec \n";
    
    free(result);

    return 0;
}
