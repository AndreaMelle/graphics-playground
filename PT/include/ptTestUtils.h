//
//  ptTestUtils.h
//  PT
//
//  Created by Andrea Melle on 06/04/2016.
//
//

#ifndef ptTestUtils_h
#define ptTestUtils_h

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

#define PT_TEST_PASS 0
#define PT_TEST_FAIL 1

namespace pt
{
    namespace test
    {
        
        typedef int pt_test_result;
        
        cl_int test_util_get_program(cl::Device& device,
                                     cl::Context& context,
                                     cl::Program& program,
                                     const char* program_file_str,
                                     const char* program_options = NULL)
        {
            cl_int clStatus;
            
            std::ifstream program_file(program_file_str);
            std::string program_str(std::istreambuf_iterator<char>(program_file), (std::istreambuf_iterator<char>()));
            cl::Program::Sources sources(1, std::make_pair(program_str.c_str(), program_str.length() + 1));
            program = cl::Program(context, sources);
            clStatus = program.build({device}, program_options);
            
            if (clStatus != CL_SUCCESS)
            {
                std::string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
                std::cerr << log << "\n";
            }
            
            return clStatus;
        }
        
        bool cl_float3_equals(const cl_float3& lhs, const cl_float3& rhs)
        {
            return (fabsf(lhs.x - rhs.x) <= FLT_EPSILON
                    && fabsf(lhs.y - rhs.y) <= FLT_EPSILON
                    && fabsf(lhs.z - rhs.z) <= FLT_EPSILON);
        }
        
        template<typename T>
        pt_test_result compare_array(T* expected, T* computed, size_t n, const std::string& err_msg)
        {
            pt_test_result result = PT_TEST_PASS;
            
            for(size_t i = 0; i < n; ++i)
            {
                if(computed[i] != expected[i])
                {
                    std::cout << err_msg << "\n"
                    << "at index : " << i << "\n"
                    << "expected : " << expected[i] << "\n"
                    << "computed : " << computed[i] << "\n\n";
                    result = PT_TEST_FAIL;
                    break;
                }
            }
            
            return result;
        }
        
        void print_perf_results(const std::string& name,
                                const size_t& global_size,
                                const size_t& local_size,
                                const double& milliseconds)
        {
            std::cout<< "=========== PERF ===========\n";
            std::cout << "Kernel : " << name << "\n"
            << "Global size : " << global_size << "\n"
            << "Local size : " << local_size << "\n"
            << "Time : " << milliseconds << " ms \n";
            std::cout<< "============================\n\n";
        }
        
        void print_kernel_info(const std::string& name,
                               const size_t& maxDeviceWorkGroupSize,
                               const size_t& maxKernelWorkGroupSize,
                               const size_t& preferredWorkGroupMult,
                               const cl_ulong& localMemSize,
                               const cl_ulong& privateMemSize)
        {
            std::cout<< "=========== INFO ===========\n";
            std::cout << "Kernel : " << name << "\n"
            << "Max device work group size : " << maxDeviceWorkGroupSize << "\n"
            << "Max kernel work group size : " << maxKernelWorkGroupSize << "\n"
            << "Preferred work group multiplier : " << preferredWorkGroupMult << "\n"
            << "Local memory size : " << localMemSize << "\n"
            << "Private memory size : " << privateMemSize << "\n";
            std::cout<< "============================\n\n";
        }
        
        
    }
}


#endif /* ptTestUtils_h */
