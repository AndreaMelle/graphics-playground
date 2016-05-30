//
//  CamRayKernelUnitTest.h
//  PT
//
//  Created by Andrea Melle on 06/04/2016.
//
//

#ifndef CamRayKernelUnitTest_h
#define CamRayKernelUnitTest_h

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
        pt_test_result test_cam_rays_render(cl::Device& device, cl::Context& context, cl::CommandQueue& cmd_queue)
        {
            /*
             * Common stuff
             */
            cl_int clStatus;
            cl::Program program;
            cl::Event profiling_evt;
            cl_ulong time_start;
            cl_ulong time_end;
            cl_ulong total_time;
            
            cl_uint width = 512;
            cl_uint height = 512;
            cl_uint samples = 8;
            cl_uint num_rays = width * height * samples;
            
            size_t local_size = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(); //TODO: throws
            size_t ray_partial_size = num_rays * sizeof(cl_float3); //size of origin or dir array
            
            /*
             * Step 1 - Run camera ray creation kernel
             */
            
            /* Load and build a program */
            clStatus = pt::test::test_util_get_program(device, context, program, "../../../assets/cam_rays_kernel.cl", "-I ../../../assets/ -cl-denorms-are-zero -D INVERT -D MAX_PRIMITIVES=10 -D MAX_RECURSION=5");
            
            PTCL_ASSERT(clStatus, "Failed to compile program.");
            
            /* create kernel and set the kernel arguments */
            cl::Kernel kernel = cl::Kernel(program, "cam_rays_kernel", &clStatus);
            PTCL_ASSERT(clStatus, "Could not create kernel");
            
            cl::Kernel kernel_render = cl::Kernel(program, "path_tracing", &clStatus);
            PTCL_ASSERT(clStatus, "Could not create kernel");
            
            cl::Buffer d_buff_w_ray_origin = cl::Buffer(context, CL_MEM_READ_WRITE, ray_partial_size, NULL, &clStatus);
            PTCL_ASSERT(clStatus, "Could not create out ray buffer")
            
            cl::Buffer d_buff_w_ray_dir = cl::Buffer(context, CL_MEM_READ_WRITE, ray_partial_size, NULL, &clStatus);
            PTCL_ASSERT(clStatus, "Could not create out ray buffer")
            
            cl::Buffer device_buff_r_cam = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_pinhole_cam), NULL, &clStatus);
            PTCL_ASSERT(clStatus, "Could not create camera buffer")
            
            /* Upload the data and set the kernel arguments */
            cl_pinhole_cam camera = cl_make_pinhole_cam(pt::PinholeCamera<float>(45.0f, 1.0f, glm::vec3(-2,1,1), glm::vec3(0,0,-1), glm::vec3(0,1,0)));
            
            PTCL_SAFE_ENQUEUE_WRITE_BUFFER("Write camera buffer error.", cmd_queue, device_buff_r_cam, CL_TRUE, 0, sizeof(cl_pinhole_cam), &camera, NULL, NULL)
            
            cl_uint arg_start = 0;
            PTCL_SAFE_SET_ARG("Could not set ray buffer argument", kernel, arg_start++, d_buff_w_ray_origin)
            PTCL_SAFE_SET_ARG("Could not set ray buffer argument", kernel, arg_start++, d_buff_w_ray_dir)
            PTCL_SAFE_SET_ARG("Could not set cam buffer argument", kernel, arg_start++, device_buff_r_cam)
            PTCL_SAFE_SET_ARG("Could not set width argument", kernel, arg_start++, width)
            PTCL_SAFE_SET_ARG("Could not set height argument", kernel, arg_start++, height)
            PTCL_SAFE_SET_ARG("Could not set samples argument", kernel, arg_start++, samples)
            
            /* one thread per ray - launch width x height x samples threads */
            
            clStatus = cmd_queue.enqueueNDRangeKernel(kernel,
                                                      cl::NDRange(0),
                                                      cl::NDRange(width * height * samples),
                                                      cl::NDRange(local_size),
                                                      NULL,
                                                      NULL);
            
            cmd_queue.finish();
            
            PTCL_ASSERT(clStatus, "Could not enqueue kernel")
            
            /*
             * Step 2 - Run rendering kernel with precomputed rays
             */
            
            arg_start = 0;
            
            /* Write buffer for image output */
            cl::Buffer d_buff_w_frame = cl::Buffer(context, CL_MEM_WRITE_ONLY, samples * width * height * sizeof(cl_float3), NULL, &clStatus);
            PTCL_ASSERT(clStatus, "Could not create out frame buffer")
            PTCL_SAFE_SET_ARG("Could not set frame buffer argument", kernel_render, arg_start++, d_buff_w_frame)
            
            /* Set precomputed rays args, already in device memory */
            PTCL_SAFE_SET_ARG("Could not set ray buffer argument", kernel_render, arg_start++, d_buff_w_ray_origin)
            PTCL_SAFE_SET_ARG("Could not set ray buffer argument", kernel_render, arg_start++, d_buff_w_ray_dir)
            
            /* Create and upload the scene */
            cl::Buffer d_buff_r_prim = cl::Buffer (context, CL_MEM_READ_ONLY, MAX_PRIMITIVES * sizeof(cl_sphere), NULL, &clStatus);
            PTCL_ASSERT(clStatus, "Could not create primitive buffer")
            
            cl::Buffer d_buff_r_mat = cl::Buffer (context, CL_MEM_READ_ONLY, MAX_PRIMITIVES * sizeof(cl_material), NULL, &clStatus);
            PTCL_ASSERT(clStatus, "Could not create primitive buffer")
            
            cl::Buffer d_buff_r_sky = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_sky_material), NULL, &clStatus);
            PTCL_ASSERT(clStatus, "Could not create sky buffer")
            
            size_t sceneObjectCount = 5;
            cl_sphere* primitive_array = (cl_sphere*)malloc(sceneObjectCount * sizeof(cl_sphere));
            cl_material* material_array = (cl_material*)malloc(sceneObjectCount * sizeof(cl_material));
            
            primitive_array[0] = cl_make_sphere(glm::vec3(1, 0, -1), 0.5f);
            material_array[0] = cl_make_material(pt::ColorHex_to_RGBfloat<float>("0x730202"), 0, MAT_LAMBERTIAN);
            
            primitive_array[1] = cl_make_sphere(glm::vec3(-1, 0, -1), 0.5f);
            material_array[1] = cl_make_material(pt::ColorHex_to_RGBfloat<float>("0xF89000"), 0, MAT_LAMBERTIAN);
            
            primitive_array[2] = cl_make_sphere(glm::vec3(0, 0, 0), 0.5f);
            material_array[2] = cl_make_material(pt::ColorHex_to_RGBfloat<float>("0x97A663"), 0.1f, MAT_METALLIC);
            
            primitive_array[3] = cl_make_sphere(glm::vec3(0, 0, -2), 0.5f);
            material_array[3] = cl_make_material(glm::vec3(0.8f, 0.6f, 0.2f), 0.3f, MAT_METALLIC);
            
            primitive_array[4] = cl_make_sphere(glm::vec3(0,-100.5f, 1.0f), 100.0f);
            material_array[4] = cl_make_material(glm::vec3(0.5f), 0, MAT_LAMBERTIAN);
            
            clStatus = cmd_queue.enqueueWriteBuffer(d_buff_r_prim, CL_TRUE, 0, sceneObjectCount * sizeof(cl_sphere), primitive_array, NULL, NULL);
            PTCL_ASSERT(clStatus, "Could not fill primitive buffer");
            
            clStatus = cmd_queue.enqueueWriteBuffer(d_buff_r_mat, CL_TRUE, 0, sceneObjectCount * sizeof(cl_material), material_array, NULL, NULL);
            PTCL_ASSERT(clStatus, "Could not fill material buffer");
            
            glm::vec3 bottom_sky_color(1.0, 1.0, 1.0);
            glm::vec3 top_sky_color(0.5, 0.7, 1.0);
            PTCL_ASSERT(cl_set_skycolors(bottom_sky_color, top_sky_color, d_buff_r_sky, cmd_queue),
                        "Could not fill sky buffer");
            
            PTCL_SAFE_SET_ARG("Could not set ray buffer argument", kernel_render, arg_start++, d_buff_r_prim)
            PTCL_SAFE_SET_ARG("Could not set ray buffer argument", kernel_render, arg_start++, d_buff_r_mat)
            PTCL_SAFE_SET_ARG("Could not set ray buffer argument", kernel_render, arg_start++, d_buff_r_sky)
            PTCL_SAFE_SET_ARG("Could not set ray buffer argument", kernel_render, arg_start++, sceneObjectCount)
            
            /* Precompute weight given samples */
            float weight = 1.0f / (float)(samples);
            PTCL_SAFE_SET_ARG("Could not set weight argument", kernel_render, arg_start++, weight)
            
            /* Launch rendering kernel */
            clStatus = cmd_queue.enqueueNDRangeKernel(kernel_render,
                                                      cl::NDRange(0),
                                                      cl::NDRange(width * height * samples),
                                                      cl::NDRange(local_size),
                                                      NULL,
                                                      NULL);
            
            cmd_queue.finish();
            
            PTCL_ASSERT(clStatus, "Could not enqueue kernel")
            
            /* Read result */
            
            cl_float3* h_buff_w_frame = (cl_float3*)malloc(samples * width * height * sizeof(cl_float3));
            
            PTCL_SAFE_OP("Could not read frame buffer", enqueueReadBuffer, cmd_queue, d_buff_w_frame, CL_TRUE, 0, samples * width * height * sizeof(cl_float3), h_buff_w_frame)
            
            /* Transform direction result into image and sanity check origin */
            float* img = (float*)malloc(width * height * 3 * sizeof(float));
            float r, g, b;
            
            cl_float3 pixel;
            
            for(int i = 0; i < (width * height); ++i)
            {
                r = 0; g = 0; b = 0;
                
                for(int k = 0; k < samples; ++k)
                {
                    pixel = h_buff_w_frame[samples * i + k];
                    /*
                     * This is 1/weight-th of the final color so don't re-weight
                     */
                    r += pixel.x;// * weight;
                    g += pixel.y;// * weight;
                    b += pixel.z;// * weight;
                }
                
                img[3 * i + 0] = r;
                img[3 * i + 1] = g;
                img[3 * i + 2] = b;
            }
            
            /*
             * Final color must be gamma corrected here for now
             */
            pt::write_ppm<float>(img, width, height, 3, pt::BUFFER_TRANSFORM_255_GAMMA, "precomp_ray.ppm");
            
            free(img);
            free(material_array);
            free(primitive_array);
            
            return PT_TEST_PASS;
        }
        
        /*
         * As general convention, if we can't do things like compile programs, allocate memory, etc...
         * this is not a test fail per se, but we assert it instead
         */
        pt_test_result test_cam_rays_kernel(cl::Device& device, cl::Context& context, cl::CommandQueue& cmd_queue)
        {
            cl_int clStatus;
            cl::Program program;
            cl::Event profiling_evt;
            cl_ulong time_start;
            cl_ulong time_end;
            cl_ulong total_time;
            
            /* Load and build a program */
            clStatus = pt::test::test_util_get_program(device, context, program, "../../../assets/cam_rays_kernel.cl", "-I ../../../assets/ -cl-denorms-are-zero -D INVERT -D MAX_PRIMITIVES=10 -D MAX_RECURSION=5");
            
            PTCL_ASSERT(clStatus, "Failed to compile program.");
            
            /* create kernel and set the kernel arguments */
            cl::Kernel kernel = cl::Kernel(program, "cam_rays_kernel", &clStatus);
            PTCL_ASSERT(clStatus, "Could not create kernel");
            
            /*
             *__kernel void path_tracing(__global Ray* out_ray_buffer, __constant struct PinholeCamera* cam,
             uint width, uint height, uint samples)
             */
            
            cl_uint width = 512;
            cl_uint height = 512;
            cl_uint samples = 8;
            cl_uint num_rays = width * height * samples;
            
            size_t local_size = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(); //TODO: throws
            
            size_t ray_partial_size = num_rays * sizeof(cl_float3); //size of origin or dir array
            
            cl::Buffer d_buff_w_ray_origin = cl::Buffer(context, CL_MEM_WRITE_ONLY, ray_partial_size, NULL, &clStatus);
            PTCL_ASSERT(clStatus, "Could not create out ray buffer")
            
            cl::Buffer d_buff_w_ray_dir = cl::Buffer(context, CL_MEM_WRITE_ONLY, ray_partial_size, NULL, &clStatus);
            PTCL_ASSERT(clStatus, "Could not create out ray buffer")
            
            cl::Buffer device_buff_r_cam = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_pinhole_cam), NULL, &clStatus);
            PTCL_ASSERT(clStatus, "Could not create camera buffer")
            
            /* Upload the data and set the kernel arguments */
            cl_pinhole_cam camera = cl_make_pinhole_cam(pt::PinholeCamera<float>(45.0f, 1.0f, glm::vec3(-2,1,1), glm::vec3(0,0,-1), glm::vec3(0,1,0)));
            
            PTCL_SAFE_ENQUEUE_WRITE_BUFFER("Write camera buffer error.", cmd_queue, device_buff_r_cam, CL_TRUE, 0, sizeof(cl_pinhole_cam), &camera, NULL, NULL)
            
            cl_uint arg_start = 0;
            PTCL_SAFE_SET_ARG("Could not set ray buffer argument", kernel, arg_start++, d_buff_w_ray_origin)
            PTCL_SAFE_SET_ARG("Could not set ray buffer argument", kernel, arg_start++, d_buff_w_ray_dir)
            PTCL_SAFE_SET_ARG("Could not set cam buffer argument", kernel, arg_start++, device_buff_r_cam)
            PTCL_SAFE_SET_ARG("Could not set width argument", kernel, arg_start++, width)
            PTCL_SAFE_SET_ARG("Could not set height argument", kernel, arg_start++, height)
            PTCL_SAFE_SET_ARG("Could not set samples argument", kernel, arg_start++, samples)
            
            /* TEST one thread per ray - launch width x height x samples threads */
            
            //    PTCL_SAFE_SET_ARG("Could not set samples argument", kernel, 4, 1)
            
            int num_tests = 1;
            double elapsed = 0;
            
            for(int i = 0; i < num_tests; ++i)
            {
                
                clStatus = cmd_queue.enqueueNDRangeKernel(kernel,
                                                          cl::NDRange(0),
                                                          cl::NDRange(width * height * samples),
                                                          cl::NDRange(local_size),
                                                          NULL,
                                                          &profiling_evt);
                
                profiling_evt.wait();
                cmd_queue.finish();
                
                PTCL_ASSERT(clStatus, "Could not enqueue kernel")
                
                time_start = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
                time_end = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_END>();
                total_time = (time_end - time_start);
                
                elapsed += (total_time  * 0.001 * 0.001)/ (double)num_tests;
                
            }
            
            std::cout << "One thread per sample total time: " << elapsed << " ms \n";
            
            /* Read result */
            
            cl_float3* h_buff_w_ray_origin = (cl_float3*)malloc(ray_partial_size);
            cl_float3* h_buff_w_ray_dir = (cl_float3*)malloc(ray_partial_size);
            
            
            PTCL_SAFE_OP("Could not read origin buffer", enqueueReadBuffer, cmd_queue, d_buff_w_ray_origin, CL_TRUE, 0, ray_partial_size, h_buff_w_ray_origin)
            
            PTCL_SAFE_OP("Could not read origin buffer", enqueueReadBuffer, cmd_queue, d_buff_w_ray_dir, CL_TRUE, 0, ray_partial_size, h_buff_w_ray_dir)
            
            /* Transform direction result into image and sanity check origin */
            unsigned char* img = (unsigned char*)malloc(width * height * 3 * sizeof(unsigned char));
            float r, g, b;
            
            float weight = 1.0f / (float)(samples);
            cl_float3 pixel;
            
            for(int i = 0; i < width * height; ++i)
            {
                r = 0; g = 0; b = 0;
                
                for(int k = 0; k < samples; ++k)
                {
                    if(!pt::test::cl_float3_equals(h_buff_w_ray_origin[samples * i + k], camera.origin))
                    {
                        cl_float3 dir = h_buff_w_ray_origin[samples * i + k];
                        std::cerr << "Ray origin should be equal to camera origin.\n";
                        std::cerr << "Ray origin: " << dir.x  << " " << dir.y << " " << dir.z  << "\n";
                        std::cerr << "Cam origin: " << camera.origin.x  << " " << camera.origin.y << " " << camera.origin.z  << "\n";
                        return PT_TEST_FAIL;
                    }
                    
                    pixel = h_buff_w_ray_dir[samples * i + k];
                    r += pixel.x * weight;
                    g += pixel.y * weight;
                    b += pixel.z * weight;
                }
                
                img[3 * i + 0] = (unsigned char)(255.0f * (0.5f * r + 0.5f));
                img[3 * i + 1] = (unsigned char)(255.0f * (0.5f * g + 0.5f));
                img[3 * i + 2] = (unsigned char)(255.0f * (0.5f * b + 0.5f));
            }
            
            pt::write_ppm<unsigned char>(img, width, height, 3, false, true, "ray_img.ppm");
            free(img);
            free(h_buff_w_ray_origin);
            free(h_buff_w_ray_dir);
            
            return PT_TEST_PASS;
        }
    }
}


#endif /* CamRayKernelUnitTest_h */
