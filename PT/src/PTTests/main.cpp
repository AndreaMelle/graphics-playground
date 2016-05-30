#define PT_TEST_PERF
#define PT_PRINT_KERNEL_INFO
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

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

#include "ptUtil.h"
#include "ptCL.h"
#include "ptTestUtils.h"
#include "CamRayKernelUnitTest.h"
#include "ScanKernelsUnitTest.h"

//#define PT_TEST_OPENGL_COMPATIBILITY

cl::Device device;
cl::Context context;
cl::CommandQueue cmd_queue;

//TEST_CASE( "Camera rays are computed", "[Camera rays kernel]" ) {
//    REQUIRE( pt::test::test_cam_rays_kernel(device, context, cmd_queue) == PT_TEST_PASS );
//    REQUIRE( pt::test::test_cam_rays_render(device, context, cmd_queue) == PT_TEST_PASS );
//}

TEST_CASE( "Inclusive scan kernel", "[Inclusive scan kernel]" ) {
    REQUIRE( pt::test::test_hillis_steele_inc_scan_single_block(device, context, cmd_queue) == PT_TEST_PASS );
}

TEST_CASE( "Exclusive scan kernel", "[Exclusive scan kernel]" ) {
    REQUIRE( pt::test::test_hillis_steele_exc_scan_single_block(device, context, cmd_queue) == PT_TEST_PASS );
}

int main(int argc, const char * argv[])
{
    /*
     * To implement OpenCL test cases we need to bring up a generic OpenCL environment
     * For the tests, we don't create an OpenGL compatible context!?
     * Then we need to allow tester to compile program, allocate memory, write data and access results
     */
    
    cl_int clStatus;
    
    /* Obtain a platform */
    std::vector<cl::Platform> platforms;
    clStatus = cl::Platform::get(&platforms);
    PTCL_ASSERT(clStatus, "Could not find an OpenCL platform.");
    
    /* Obtain a device and determinte max local size */
    std::vector<cl::Device> devices;
    clStatus = platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
    PTCL_ASSERT(clStatus, "Could not find a GPU device.");
    device = devices[0];
    
    cl_uint maxDeviceComputeUnits = device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
    
    /* Create an OpenCL context for the device */
#ifdef PT_TEST_OPENGL_COMPATIBILITY
    
    CGLContextObj glContext = CGLGetCurrentContext();
    CGLShareGroupObj shareGroup = CGLGetShareGroup(glContext);
    
    cl_context_properties properties[] = {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
        (cl_context_properties)shareGroup,
        0
    };
    
    context = cl::Context({device}, properties, NULL, NULL, &clStatus);
    
#else 
    
    context = cl::Context({device}, NULL, NULL, NULL, &clStatus);
    
#endif
    
    PTCL_ASSERT(clStatus, "Could not create a context for device.");
    
    /* Create command queue */
    cmd_queue = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &clStatus);
    PTCL_ASSERT(clStatus, "Could not create command queue");
    
    /*
     * Now the tests can execute, but each test needs to create a program first
     */
    
    int result = Catch::Session().run( argc, argv );
    
    return result;
}


