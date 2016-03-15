#ifndef ptCL_h
#define ptCL_h

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

#define MAX_PRIMITIVES 10

void pt_assert(const cl_int& status, const std::string& errorMsg)
{
    if (status != CL_SUCCESS)
    {
        std::cerr << errorMsg << "\n";
        exit(EXIT_FAILURE);
    }
}

//TODO: packing stuff is compiler dependent

typedef struct cl_pinhole_cam
{
    cl_float3 origin;
    cl_float3 lower_left;
    cl_float3 hor;
    cl_float3 ver;
} cl_pinhole_cam;

/*
 * sizeof(cl_sphere) declared with __attribute__((packed)) -> 20 (16 + 4)
 * sizeof(cl_sphere) -> 32 (16 + 4 + 12) 12 bytes padding...
 */
typedef struct cl_sphere
{
    cl_float3 center;
    cl_float radius;
} cl_sphere;

typedef struct cl_material
{
    cl_float3 albedo;
} cl_material;

typedef struct cl_sky_material
{
    cl_float3 bottom;
    cl_float3 top;
} cl_sky_material;

cl_int cl_set_skycolors(const glm::vec3& sky_bottom,
                        const glm::vec3& sky_top,
                        cl::Buffer& sky_buffer,
                        cl::CommandQueue& cmd_queue)
{
    cl_sky_material sky;
    memcpy(&sky.bottom, glm::value_ptr(sky_bottom), 3 * sizeof(float));
    memcpy(&sky.top, glm::value_ptr(sky_top), 3 * sizeof(float));
    return cmd_queue.enqueueWriteBuffer(sky_buffer, CL_TRUE, 0, sizeof(cl_sky_material), (void*)&sky, NULL, NULL);
}

cl_int cl_set_sphere_and_material_list(const std::vector<pt::fSphereRef>& primitives,
                                       const std::vector<pt::fLambertianRef>& materials,
                                       cl::Buffer& primitives_buffer,
                                       cl::Buffer& materials_buffer,
                                       const cl::CommandQueue& cmd_queue)
{
    cl_int clStatus;
    if(primitives.size() != materials.size()) return CL_INVALID_VALUE;
    
    unsigned int listCount = primitives.size();
    
    if(listCount > MAX_PRIMITIVES) return CL_INVALID_VALUE;
    
    cl_sphere* primitive_array = (cl_sphere*)malloc(listCount * sizeof(cl_sphere));
    cl_material* material_array = (cl_material*)malloc(listCount * sizeof(cl_material));
    
    for(int i = 0; i < listCount; ++i)
    {
        cl_sphere sp;
        memcpy(&sp.center, glm::value_ptr(primitives[i]->getCenter()), 3 * sizeof(float));
        sp.radius = primitives[i]->getRadius();
        memcpy(&primitive_array[i], &sp, sizeof(cl_sphere));
    }
    
    for(int i = 0; i < listCount; ++i)
    {
        cl_material mat;
        memcpy(&mat.albedo, glm::value_ptr(materials[i]->getAlbedo()), 3 * sizeof(float));
        memcpy(&material_array[i], &mat, sizeof(cl_material));
    }
    
    cmd_queue.enqueueWriteBuffer(primitives_buffer, CL_TRUE, 0, listCount * sizeof(cl_sphere), primitive_array, NULL, NULL);
    if (clStatus != CL_SUCCESS) return clStatus;
    
    cmd_queue.enqueueWriteBuffer(materials_buffer, CL_TRUE, 0, listCount * sizeof(cl_material), material_array, NULL, NULL);
    if (clStatus != CL_SUCCESS) return clStatus;
    
    free(primitive_array);
    free(material_array);
    
    return CL_SUCCESS;
}

cl_int cl_set_pinhole_cam_arg(const glm::vec3& origin,
                              const glm::vec3& lower_left,
                              const glm::vec3& hor,
                              const glm::vec3& ver,
                              cl::Buffer& cam_buffer,
                              const cl::CommandQueue& cmd_queue)
{
    cl_pinhole_cam cl_cam;
    
    memcpy(&cl_cam.origin, glm::value_ptr(origin), 3 * sizeof(float));
    memcpy(&cl_cam.lower_left, glm::value_ptr(lower_left), 3 * sizeof(float));
    memcpy(&cl_cam.hor, glm::value_ptr(hor), 3 * sizeof(float));
    memcpy(&cl_cam.ver, glm::value_ptr(ver), 3 * sizeof(float));
    
    return cmd_queue.enqueueWriteBuffer(cam_buffer, CL_TRUE, 0, sizeof(cl_pinhole_cam), &cl_cam, NULL, NULL);
}

cl_int cl_set_pinhole_cam_arg(const pt::PinholeCamera<float>& cam,
                              cl::Buffer& cam_buffer,
                              const cl::CommandQueue& cmd_queue)
{
    cl_pinhole_cam cl_cam;
    
    memcpy(&cl_cam.origin, glm::value_ptr(cam.getOrigin()), 3 * sizeof(float));
    memcpy(&cl_cam.lower_left, glm::value_ptr(cam.getLowerLeft()), 3 * sizeof(float));
    memcpy(&cl_cam.hor, glm::value_ptr(cam.getHor()), 3 * sizeof(float));
    memcpy(&cl_cam.ver, glm::value_ptr(cam.getVer()), 3 * sizeof(float));
    
    return cmd_queue.enqueueWriteBuffer(cam_buffer, CL_TRUE, 0, sizeof(cl_pinhole_cam), &cl_cam, NULL, NULL);
}

#endif /* ptCL_h */
