#ifndef ptTestsCL_h
#define ptTestsCL_h

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include "../include/cl.hpp"
#else
#include <CL/cl.h>
#include <CL/cl.hpp>
#endif

#include "glm/gtc/type_ptr.hpp"
#include "ptUtil.h"
#include "ptGeometry.h"
#include "ptRandom.h"
#include "ptMaterial.h"
#include "ptRendering.h"
#include "ptCL.h"

namespace pt
{
    namespace test
    {
        
        
        bool setup_opencl(const char* program_file_str,
                          cl::Device& out_device,
                          cl::Context& out_context,
                          cl::Program& out_program,
                          CGLShareGroupObj* share_group = nullptr)
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
            out_device = devices[0];
            
            //    size_t global_size = img_width * img_height;
            
            /* Create an OpenCL context for the device */
            
            if (share_group != nullptr)
            {
                cl_context_properties properties[] = {
                  CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
                    (cl_context_properties)*share_group,
                    0
                };
                out_context = cl::Context({out_device}, properties, NULL, NULL, &clStatus);
            }
            else
            {
                out_context = cl::Context({out_device}, NULL, NULL, NULL, &clStatus); //TODO: add the error handling callback
            }
            
            assertFatal(clStatus, "Could not create a context for device.");
            
            /* Load and build a program */
            std::ifstream program_file(program_file_str);
            std::string program_str(std::istreambuf_iterator<char>(program_file), (std::istreambuf_iterator<char>()));
            cl::Program::Sources sources(1, std::make_pair(program_str.c_str(), program_str.length() + 1));
            out_program = cl::Program(out_context, sources);
            clStatus = out_program.build({out_device}, "-I ../../../assets/");
            
            if (clStatus != CL_SUCCESS)
            {
                std::string log = out_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(out_device);
                std::cerr << log << "\n";
                exit(EXIT_FAILURE);
            }
            
            return true;
        }
        
        void test_diffuse_ppm_cl(unsigned int width,
                                 unsigned int height,
                                 unsigned int samples = 8,
                                 CGLShareGroupObj* share_group = nullptr,
                                 GLuint* texture = nullptr)
        {
            cl_int clStatus;
            
            size_t img_width = (size_t)width;
            size_t img_height = (size_t)height;
            
            const char* program_file_str = "../../../assets/path_tracing.cl";
            
            cl::Device device;
            cl::Context context;
            cl::Program program;
            
            setup_opencl(program_file_str, device, context, program, share_group);
            
            // cl_int num_groups = (cl_int)(img_width * img_height) / local_size;
            
            cl::Image2DGL img_buffer;
            
            if(texture != nullptr)
            {
                glGenTextures(1, texture);
                glBindTexture(GL_TEXTURE_2D, *texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
                
                img_buffer = cl::Image2DGL(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, *texture, &clStatus);
            }
            else
            {
             //   img_buffer = cl::Image2D(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGBA, CL_UNSIGNED_INT8), img_width, img_height, 0, NULL, &clStatus);
            }
            
            
            assertFatal(clStatus, "Could not create buffer");
            
            /* Create command queue */
            cl::CommandQueue cmd_queue(context, device, CL_QUEUE_PROFILING_ENABLE, &clStatus);
            assertFatal(clStatus, "Could not create command queue");
            
            /* create kernel and set the kernel arguments */
            cl::Kernel kernel(program, "path_tracing", &clStatus);
            assertFatal(clStatus, "Could not create kernel");
            
            /* Camera */
            PinholeCamera<float> cam(glm::vec3(0,0,0), glm::vec3(-2.0f,-1.5f,-1.0f), glm::vec3(4.0f, 0, 0), glm::vec3(0, 3.0f, 0));
            
            cl::Buffer cam_buffer(context,
                                  CL_MEM_READ_ONLY,
                                  sizeof(cl_pinhole_cam),
                                  (void*)0,
                                  &clStatus);
            
            assertFatal(clStatus, "Could not create camera buffer");
            
            
            
            assertFatal(cl_set_pinhole_cam_arg(cam, cam_buffer, cmd_queue), "Could not fill camera buffer");
            
            /* Spheres */
            std::vector<fSphereRef> list;
            std::vector<fLambertianRef> materials;
            
            list.push_back(fSphereRef(new Sphere<float>(glm::vec3(0,0,-1.0f), 0.5f)));
            list.push_back(fSphereRef(new Sphere<float>(glm::vec3(0,-100.5f, 1.0f), 100.0f)));

            materials.push_back(fLambertianRef(new Lambertian<float>(glm::vec3(0.5f))));
            materials.push_back(fLambertianRef(new Lambertian<float>(glm::vec3(0.5f))));
            
            cl::Buffer primitive_buffer(context,
                                        CL_MEM_READ_ONLY,
                                        MAX_PRIMITIVES * sizeof(cl_sphere),
                                        (void*)0,
                                        &clStatus);
            
            assertFatal(clStatus, "Could not create primitive buffer");
            
            cl::Buffer material_buffer(context,
                                       CL_MEM_READ_ONLY,
                                       MAX_PRIMITIVES * sizeof(cl_material),
                                       (void*)0,
                                       &clStatus);
            
            assertFatal(clStatus, "Could not create primitive buffer");
            
            assertFatal(cl_set_sphere_and_material_list(list,
                                                        materials,
                                                        primitive_buffer,
                                                        material_buffer,
                                                        cmd_queue), "Could not fill primitives or materials buffers");
            
            /* Sky color */
            glm::vec3 bottom_sky_color(1.0, 1.0, 1.0);
            glm::vec3 top_sky_color(0.5, 0.7, 1.0);
            
            cl::Buffer sky_buffer(context,
                                  CL_MEM_READ_ONLY,
                                  sizeof(cl_sky_material),
                                  (void*)0,
                                  &clStatus);
            
            assertFatal(clStatus, "Could not create sky buffer");
            
            assertFatal(cl_set_skycolors(bottom_sky_color, top_sky_color, sky_buffer, cmd_queue), "Could not fill sky buffer");
        
            size_t local_size = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(); //TODO: throws
            size_t local_width = (size_t)pow(2, ceilf(log2f((floorf(sqrtf(local_size))))));
            
            
            clStatus = kernel.setArg(0, cam_buffer);
            assertFatal(clStatus, "Could not set camera buffer argument");
            
            clStatus = kernel.setArg(1, primitive_buffer);
            assertFatal(clStatus, "Could not set primitive buffer argument");
            
            clStatus = kernel.setArg(2, material_buffer);
            assertFatal(clStatus, "Could not set primitive buffer argument");
            
            clStatus = kernel.setArg(3, sky_buffer);
            assertFatal(clStatus, "Could not set primitive buffer argument");
            
            clStatus = kernel.setArg(4, list.size());
            assertFatal(clStatus, "Could not set primitive count argument");
            
            clStatus = kernel.setArg(5, img_buffer);
            assertFatal(clStatus, "Could not set img buffer argument");
            
            clStatus = kernel.setArg(6, samples);
            assertFatal(clStatus, "Could not set samples argument");
            
            
            /* Enqueue kernel for execution */
            cl::Event profiling_evt;
            std::vector<cl::Memory> m;
            
            if(texture != nullptr)
            {
                glFinish();
                glFlush();
                m.push_back(img_buffer);
                clStatus = cmd_queue.enqueueAcquireGLObjects(&m, NULL, NULL);
                assertFatal(clStatus, "Could not enqueue the kernel");
                cmd_queue.finish();
                cmd_queue.flush();
            }
            
            clStatus = cmd_queue.enqueueNDRangeKernel(kernel,
                                                      cl::NDRange(0,0),
                                                      cl::NDRange(img_width, img_height),
                                                      cl::NDRange(local_width,local_size / local_width),
                                                      NULL,
                                                      &profiling_evt);
            
            assertFatal(clStatus, "Could not enqueue the kernel");
            
            cmd_queue.finish();
            cmd_queue.flush();
            
            
            /* Wait for process to finish */
            
            cl_ulong time_start = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_START>();
            cl_ulong time_end = profiling_evt.getProfilingInfo<CL_PROFILING_COMMAND_END>();
            cl_ulong total_time = time_end - time_start;
            
            /* read the result */
            //if(texture == 0)
            //{
                unsigned char* img = (unsigned char*)malloc(4 * img_width * img_height * sizeof(unsigned char));
                cl::size_t<3> origin; origin.push_back(0); origin.push_back(0); origin.push_back(0);
                cl::size_t<3> region; region.push_back(img_width); region.push_back(img_height); region.push_back(1);
                clStatus = cmd_queue.enqueueReadImage(img_buffer, CL_TRUE, origin, region, 0, 0, (void*)img);
                assertFatal(clStatus, "Could not read the buffer");
                pt::write_ppm<unsigned char>(img, img_width, img_height, 4, false, false, "diffuse_cl.ppm");
                free(img);
            //}
            
            std::cout << "Total time: " << total_time * 0.001 * 0.001 << " ms \n";
            
            
            if(texture != nullptr)
            {
                clStatus = cmd_queue.enqueueReleaseGLObjects(&m, NULL, NULL);
                assertFatal(clStatus, "Could not enqueue the kernel");
                cmd_queue.finish();
                cmd_queue.flush();
            }
            
        }
    }
}

#endif /* ptTestsCL_h */
