//
//  ScanKernelsUnitTest.h
//  PT
//
//  Created by Andrea Melle on 06/04/2016.
//
//

#ifndef ScanKernelsUnitTest_h
#define ScanKernelsUnitTest_h

#include "ptTestUtils.h"
#include "ptUtil.h"
#include "ptCL.h"

/*
 * TODO: massively refractor this into a class approach
 */
namespace pt
{
    namespace test
    {
        void allocate_scan_test_data(int*& test_data,
                                     int*& expected_result,
                                     size_t n,
                                     bool inclusive = true)
        {
            assert(n > 0);
            
            if(test_data != nullptr) free(test_data);
            if(expected_result != nullptr) free(expected_result);
            
            test_data = (int*)malloc(n * sizeof(int));
            expected_result = (int*)malloc(n * sizeof(int));
            
            // 1 2 3  4  5  6  7  8
            // 1 3 6 10 15 21 28 36 inclusive
            // 0 1 3  6 10 15 21 28 exclusive
            
            for(int i = 0; i < n; ++i)
            {
                test_data[i] = (i + 1);
            }
            
            expected_result[0] = inclusive ? test_data[0] : 0;
            
            int idx;
            
            for(int i = 1; i < n; ++i)
            {
                idx = inclusive ? i : (i - 1);
                expected_result[i] = test_data[idx] + expected_result[i - 1];
            }
        }
        
        pt_test_result test_hillis_steele_scan_single_block(cl::Device& device,
                                                            cl::Context& context,
                                                            cl::CommandQueue& cmd_queue,
                                                            bool inclusive_scan)
        {
            pt_test_result result = PT_TEST_PASS;
            
            cl_int clStatus;
            cl::Program program;
            
#ifdef PT_TEST_PERF
            cl::Event profiling_evt;
            cl_ulong time_start;
            cl_ulong time_end;
            cl_ulong total_time;
            double elapsed;
#endif
            
            std::string program_compile_opt = "-cl-denorms-are-zero";
            
            /* Load and build a program */
            if(inclusive_scan)
            {
                program_compile_opt += " -D SCAN_INCLUSIVE";
            }

            clStatus = pt::test::test_util_get_program(device, context, program, "../../../assets/hillis_steele_scan.cl", program_compile_opt.c_str());
            
            PTCL_ASSERT(clStatus, "Failed to compile program.");
            
            /* Create kernel */
            cl::Kernel kernel = cl::Kernel(program, "hillis_steele_scan", &clStatus);
            PTCL_ASSERT(clStatus, "Could not create kernel");
            
            /* Query kernel info */
            
            std::string kernel_print_name = kernel.getInfo<CL_KERNEL_FUNCTION_NAME>();
            
            if(inclusive_scan)
            {
                kernel_print_name += " inclusive";
            }
            else
            {
                kernel_print_name += " exclusive";
            }
            
            size_t maxDeviceWorkGroupSize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(); //TODO: throws
            size_t maxKernelWorkGroupSize = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);
            size_t preferredWorkGroupMult = kernel.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(device);
            cl_ulong localMemSize = kernel.getWorkGroupInfo<CL_KERNEL_LOCAL_MEM_SIZE>(device);
            cl_ulong privateMemSize = kernel.getWorkGroupInfo<CL_KERNEL_PRIVATE_MEM_SIZE>(device);
            
            /* Let's run one block only */
            size_t block_size = maxKernelWorkGroupSize;
            size_t data_num = block_size;
            size_t data_size = data_num * sizeof(int);
            
            /* Create test data and expected result */
            int* test_data = nullptr;
            int* expected_result = nullptr;
            
            allocate_scan_test_data(test_data, expected_result, data_num, inclusive_scan);
            
            /* Print kernel info */
#ifdef PT_PRINT_KERNEL_INFO
            print_kernel_info(kernel_print_name,
                              maxDeviceWorkGroupSize,
                              maxKernelWorkGroupSize,
                              preferredWorkGroupMult,
                              localMemSize,
                              privateMemSize);
#endif
            
            /* Allocate GPU buffers */
            
            cl::Buffer d_buff_rw_data = cl::Buffer(context, CL_MEM_READ_WRITE, data_size, NULL, &clStatus);
            PTCL_ASSERT(clStatus, "Could not create data buffer")
            
            PTCL_SAFE_ENQUEUE_WRITE_BUFFER("Write data buffer error.", cmd_queue, d_buff_rw_data, CL_TRUE, 0, data_size, test_data, NULL, NULL)
            
            cl_uint arg_start = 0;
            PTCL_SAFE_SET_ARG("Could not set global data buffer argument", kernel, arg_start++, d_buff_rw_data)
            
            /* Allocate shared memory */
            PTCL_SAFE_SET_ARG("Could not allocate shared memory", kernel, arg_start++, data_size, NULL);
            
            /* Launch kernel with only one block */
            
            clStatus = cmd_queue.enqueueNDRangeKernel(kernel,
                                                      cl::NDRange(0),
                                                      cl::NDRange(block_size),
                                                      cl::NDRange(block_size),
                                                      NULL,
#ifdef PT_TEST_PERF
                                                      &profiling_evt);
            profiling_evt.wait();
#else
            NULL);
#endif
            
            
            cmd_queue.finish();
            
            PTCL_ASSERT(clStatus, "Could not enqueue kernel")
            
#ifdef PT_TEST_PERF
            time_start = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
            time_end = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_END>();
            total_time = (time_end - time_start);
            elapsed = total_time  * 0.001 * 0.001;
            
            print_perf_results(kernel_print_name, block_size, block_size, elapsed);
#endif
            
            /* Read result */
            
            int* computed_result = (int*)malloc(data_size);
            
            PTCL_SAFE_OP("Could not read data buffer", enqueueReadBuffer, cmd_queue, d_buff_rw_data, CL_TRUE, 0, data_size, computed_result)
            
            /* Verify result */
            result = compare_array(expected_result, computed_result, data_num, kernel_print_name + " test failed");
            
            free(test_data);
            free(expected_result);
            free(computed_result);
            
            return result;
        }
        
        pt_test_result test_hillis_steele_inc_scan_single_block(cl::Device& device,
                                                                cl::Context& context,
                                                                cl::CommandQueue& cmd_queue)
        {
            return test_hillis_steele_scan_single_block(device, context, cmd_queue, true);
        }
        
        pt_test_result test_hillis_steele_exc_scan_single_block(cl::Device& device,
                                                                cl::Context& context,
                                                                cl::CommandQueue& cmd_queue)
        {
            return test_hillis_steele_scan_single_block(device, context, cmd_queue, false);
        }
        
    }
}


#endif /* ScanKernelsUnitTest_h */
