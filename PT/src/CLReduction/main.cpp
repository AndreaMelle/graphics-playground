#include <iostream>
#include <assert.h>
#include <fstream>
#include <memory>
#include <math.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include "../include/cl.hpp"
#else
#include <CL/cl.h>
#include <CL/cl.hpp>
#endif

#define ARRAY_SIZE 1048576

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
    size_t global_size = ARRAY_SIZE;
    
    /* Create an OpenCL context for the device */
    cl::Context context({device}, NULL, NULL, NULL, &clStatus); //TODO: add the error handling callback
    assertFatal(clStatus, "Could not create a context for device.");
    
    /* Load and build a program */
    cl::Program program;
    std::ifstream program_file("../../../assets/reduce.cl");
    std::string program_str(std::istreambuf_iterator<char>(program_file), (std::istreambuf_iterator<char>()));
    cl::Program::Sources sources(1, std::make_pair(program_str.c_str(), program_str.length() + 1));
    program = cl::Program(context, sources);
    clStatus = program.build({device});
    
    if (clStatus != CL_SUCCESS)
    {
        std::string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        std::cerr << log << "\n";
        exit(EXIT_FAILURE);
    }
    
    /* Create data buffers */
    float data[ARRAY_SIZE];
    for(int i = 0; i < ARRAY_SIZE; ++i)
        data[i] = (float)i;
    
    // memory for input data
    cl::Buffer data_buffer(context,
                           CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                           ARRAY_SIZE * sizeof(float),
                           data,
                           &clStatus);
    
    assertFatal(clStatus, "Could not create buffer");
    
    cl_int num_groups = ARRAY_SIZE / local_size;
    float* scalar_sum = (float*)malloc(num_groups * sizeof(float));
    for(int i = 0; i < num_groups; ++i)
        scalar_sum[i] = 0.0f;
    
    // memory for each workgroup intermediate result
    cl::Buffer scalar_sum_buffer(context,
                                 CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                 num_groups * sizeof(float),
                                 scalar_sum,
                                 &clStatus);
    
    assertFatal(clStatus, "Could not create buffer");
    
    /* Create command queue */
    cl::CommandQueue cmd_queue(context, device, CL_QUEUE_PROFILING_ENABLE, &clStatus);
    assertFatal(clStatus, "Could not create buffer");
    
    /* create kernel and set the kernel arguments */
    cl::Kernel kernel(program, "reduce", &clStatus);
    assertFatal(clStatus, "Could not create kernel");
    
    clStatus = kernel.setArg(0, data_buffer);
    assertFatal(clStatus, "Could not set data argument");
    clStatus = kernel.setArg(1, cl::__local(local_size * sizeof(float)));
    assertFatal(clStatus, "Could not set local buffer argument");
    clStatus = kernel.setArg(2, scalar_sum_buffer);
    assertFatal(clStatus, "Could not set result argument");
    
    /* Enqueue kernel for execution */
    cl::Event profiling_evt;
    clStatus = cmd_queue.enqueueNDRangeKernel(kernel,
                                              cl::NDRange(0),
                                              cl::NDRange(global_size),
                                              cl::NDRange(local_size),
                                              NULL,
                                              &profiling_evt);
    
    assertFatal(clStatus, "Could not enqueue the kernel");
    
    /* Wait for process to finish */
    cmd_queue.finish();
    cl_ulong time_start = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    cl_ulong time_end = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_END>();
    cl_ulong total_time = time_end - time_start;
    
    /* read the result */
    clStatus = cmd_queue.enqueueReadBuffer(scalar_sum_buffer,
                                           CL_TRUE,
                                           0,
                                           num_groups * sizeof(float),
                                           scalar_sum);
    
    assertFatal(clStatus, "Could not read the buffer");
    
    /* combine the result */
    float sum = 0.0f;
    for(int i = 0; i < num_groups; ++i)
    {
        sum += scalar_sum[i];
    }
    
    float actual_sum = 1.0f * ARRAY_SIZE / 2.0f * (ARRAY_SIZE - 1.0f);
    if (fabs(sum - actual_sum) > 0.01 * fabs(sum))
    {
        std::cerr << "Sum check fail.\n";
    }
    else
    {
        std::cout << "Sum check pass.\n";
    }
    
    std::cout << "Total time: " << total_time << "ns \n";
    
    return 0;
}
