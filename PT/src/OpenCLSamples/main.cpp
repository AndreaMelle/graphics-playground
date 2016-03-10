//
#include <iostream>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <vector>

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

int main(int argc, const char * argv[])
{
    char* kernelSource;
    _load("../../../assets/saxpy.cl", kernelSource);
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
    cl_mem C_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, VECTOR_SIZE * sizeof(float), NULL, &clStatus);
    
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
        
        std::cout << log << "\n";
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
        std::cout << C[i] << " ";
    
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
    
    return 0;
}
