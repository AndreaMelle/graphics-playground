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
    std::ifstream program_file("../../../assets/random.cl");
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
    
//    cl_int num_groups = (cl_int)(img_width * img_height) / local_size;
    
    cl::Image2D img_buffer(context,
                           CL_MEM_WRITE_ONLY,
                           cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8),
                           img_width,
                           img_height,
                           0,
                           NULL,
                           &clStatus);
    
    assertFatal(clStatus, "Could not create buffer");
    
    /* Create command queue */
    cl::CommandQueue cmd_queue(context, device, CL_QUEUE_PROFILING_ENABLE, &clStatus);
    assertFatal(clStatus, "Could not create command queue");
    
    /* create kernel and set the kernel arguments */
    cl::Kernel kernel(program, "random_image", &clStatus);
    assertFatal(clStatus, "Could not create kernel");
    
    clStatus = kernel.setArg(0, img_buffer);
    assertFatal(clStatus, "Could not set data argument");
    
    // got to find a way to split local_size in a*b
    size_t local_width = (size_t)pow(2, ceilf(log2f((floorf(sqrtf(local_size))))));
    
    
    /* Enqueue kernel for execution */
    cl::Event profiling_evt;
    clStatus = cmd_queue.enqueueNDRangeKernel(kernel,
                                              cl::NDRange(0,0),
                                              cl::NDRange(img_height, img_width),
                                              cl::NDRange(local_width,local_size / local_width),
                                              NULL,
                                              &profiling_evt);
    
    assertFatal(clStatus, "Could not enqueue the kernel");
    
    /* Wait for process to finish */
    cmd_queue.finish();
    cl_ulong time_start = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    cl_ulong time_end = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_END>();
    cl_ulong total_time = time_end - time_start;
    
    /* read the result */
    
    unsigned char* img = (unsigned char*)malloc(4 * img_width * img_height * sizeof(unsigned char));
    cl::size_t<3> origin;
    origin.push_back(0); origin.push_back(0); origin.push_back(0);
    cl::size_t<3> region;
    region.push_back(img_width); region.push_back(img_height); region.push_back(1);
    clStatus = cmd_queue.enqueueReadImage(img_buffer, CL_TRUE, origin, region, 0, 0, (void*)img);
    
    assertFatal(clStatus, "Could not read the buffer");
    
    double avg = 0.0f;
    double weight = 1.0f / (double)(img_width * img_height);
    
    /* combine the result */
    FILE *f = fopen("noise.ppm", "w");         // Write image to PPM file.
    fprintf(f, "P3\n%d %d\n%d\n", (int)img_width, (int)img_height, 255);
    for (int i = 0; i < 4 * img_width * img_height; i += 4)
    {
//        printf("%d %d %d %d", img[i], img[i + 1], img[i + 2], img[i + 3]);
        fprintf(f,"%d %d %d ", img[i], img[i], img[i]);
        avg += ((double)img[i] * weight);
    }
    
    fclose(f);
    
    std::cout << "Average of noise: " << (unsigned int)avg << "\n";
    
    std::cout << "Total time: " << total_time * 0.001 << " microsec \n";
    
    free(img);

    return 0;
}
