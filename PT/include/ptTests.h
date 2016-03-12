//
//  ptTests.h
//  PT
//
//  Created by Andrea Melle on 11/03/2016.
//
//

#ifndef ptTests_h
#define ptTests_h

#include "ptUtil.h"
#include "ptGeometry.h"
#include "ptRandom.h"

//#define SAMPLE_HEMI
#include "ptMaterial.h"

#define MAX_RECURSION 5


namespace pt
{
    namespace test
    {
        glm::vec3 color(const Ray<float>& ray,
                        const PrimitiveList<float>& list,
                        const std::vector<std::shared_ptr<Material<float>>>& materials,
                        UniformRNG<float>& rng,
                        int recursion_depth,
                        const glm::vec3& bottom_sky_color = glm::vec3(1.0f, 1.0f, 1.0f),
                        const glm::vec3& top_sky_color = glm::vec3(0.5f, 0.7f, 1.0f))
        {
            float t;
            size_t idx;
            bool hit;
            //
            if ((hit = list.intersect_simple(ray, t, idx, 1e-4, MAXFLOAT)))
            {
                glm::vec3 p = ray.operator()(t); // where intersects
                glm::vec3 normal = list[idx]->normalAt(p); // normal at intersection
                Ray<float> ray_out;
                glm::vec3 attenuation;
                
                if(recursion_depth < MAX_RECURSION && materials[idx]->scatter(ray, p, normal, rng, attenuation, ray_out))
                    return attenuation * color(ray_out, list, materials, rng, recursion_depth + 1);
                else
                    return glm::vec3(0);
            }
            else
            {
                t = 0.5f * ray.dir.y + 0.5f;
                return ((1.0f - t) * bottom_sky_color + t * top_sky_color);
            }
        }
        
        void test_diffuse_metal_ppm(unsigned int width, unsigned int height, unsigned int samples = 8)
        {
            float* buffer = (float*)malloc(3 * width * height * sizeof(float));
            float u, v;
            size_t idx;
            pt::Ray<float> ray;
            
            Camera<float> cam(glm::vec3(0,0,0), glm::vec3(-2.0f,-1.5f,-1.0f), glm::vec3(4.0f, 0, 0), glm::vec3(0, 3.0f, 0));
            
            glm::vec3 dir, c, normal;
            
            PrimitiveList<float> list;
            list.push_back(std::shared_ptr<Sphere<float>>(
                                                          new Sphere<float>(glm::vec3(0,0,-1.0f), 0.5f)));
            
            list.push_back(std::shared_ptr<Sphere<float>>(
                                                          new Sphere<float>(glm::vec3(0,-100.5f, -1.0f), 100.0f)));
            
            list.push_back(std::shared_ptr<Sphere<float>>(
                                                          new Sphere<float>(glm::vec3(1,0,-1), 0.5f)));
            
            list.push_back(std::shared_ptr<Sphere<float>>(
                                                          new Sphere<float>(glm::vec3(-1,0,-1), 0.5f)));
            
            std::vector<std::shared_ptr<Material<float>>> materials;
            materials.push_back(std::shared_ptr<Material<float>>(new Lambertian<float>(glm::vec3(0.8f, 0.3f, 0.3f))));
            materials.push_back(std::shared_ptr<Material<float>>(new Lambertian<float>(glm::vec3(0.8f, 0.8f, 0.0f))));
            
            materials.push_back(std::shared_ptr<Material<float>>(new Metallic<float>(glm::vec3(0.8f, 0.6f, 0.2f), 0.3f)));
            materials.push_back(std::shared_ptr<Material<float>>(new Metallic<float>(glm::vec3(0.8f, 0.8f, 0.8f), 0.1f)));
            
            
            XORUniformRNG<float> rng;
            PcgHash hash;
            
            float weigth = 1.0f / (float)samples;
            
            for(int j = height - 1; j >= 0; --j)
            {
                for(int i = 0; i < width; ++i)
                {
                    c = glm::vec3(0,0,0);
                    
                    rng.seed(hash(i, j));
                    
                    for(int s = 0; s < samples; ++s)
                    {
                        u = ((float)i + rng()) / (float)width;
                        v = ((float)j + rng()) / (float)height;
                        
                        ray = cam.getRay(u, v);
                        
                        c += weigth * color(ray, list, materials, rng, 0);
                    }
                    
                    idx = 3 * ((height - j - 1) * width + i);
                    buffer[idx + 0] = c.r;
                    buffer[idx + 1] = c.g;
                    buffer[idx + 2] = c.b;
                }
            }
            
            pt::write_ppm<float>(buffer, width, height, 3, true, true, "diffuse_metal.ppm");
            
            free(buffer);
        }
        
        void test_diffuse_ppm(unsigned int width, unsigned int height, unsigned int samples = 8)
        {
            float* buffer = (float*)malloc(3 * width * height * sizeof(float));
            float u, v;
            size_t idx;
            pt::Ray<float> ray;
            
            Camera<float> cam(glm::vec3(0,0,0), glm::vec3(-2.0f,-1.5f,-1.0f), glm::vec3(4.0f, 0, 0), glm::vec3(0, 3.0f, 0));
            
            glm::vec3 dir, c, normal;
            
            PrimitiveList<float> list;
            list.push_back(std::shared_ptr<Sphere<float>>(
                                                          new Sphere<float>(glm::vec3(0,0,-1.0f), 0.5f)));
            
            list.push_back(std::shared_ptr<Sphere<float>>(
                                                          new Sphere<float>(glm::vec3(0,-100.5f, 1.0f), 100.0f)));
            
            std::vector<std::shared_ptr<Material<float>>> materials;
            materials.push_back(std::shared_ptr<Material<float>>(new Lambertian<float>(glm::vec3(0.5f))));
            materials.push_back(std::shared_ptr<Material<float>>(new Lambertian<float>(glm::vec3(0.5f))));
            
            
            XORUniformRNG<float> rng;
            PcgHash hash;
            
            float weigth = 1.0f / (float)samples;
            
            for(int j = height - 1; j >= 0; --j)
            {
                for(int i = 0; i < width; ++i)
                {
                    c = glm::vec3(0,0,0);
                    
                    rng.seed(hash(i, j));
                    
                    for(int s = 0; s < samples; ++s)
                    {
                        u = ((float)i + rng()) / (float)width;
                        v = ((float)j + rng()) / (float)height;
                        
                        ray = cam.getRay(u, v);
                        
                        c += weigth * color(ray, list, materials, rng, 0);
                    }
                    
                    idx = 3 * ((height - j - 1) * width + i);
                    buffer[idx + 0] = c.r;
                    buffer[idx + 1] = c.g;
                    buffer[idx + 2] = c.b;
                }
            }
            
            pt::write_ppm<float>(buffer, width, height, 3, true, true, "diffuse.ppm");
            
            free(buffer);
        }
        
        
        void test_aa_ppm(unsigned int width, unsigned int height, unsigned int samples = 8)
        {
            float* buffer = (float*)malloc(3 * width * height * sizeof(float));
            float u, v, t;
            size_t idx;
            pt::Ray<float> ray;
            
            Camera<float> cam(glm::vec3(0,0,0), glm::vec3(-2.0f,-1.5f,-1.0f), glm::vec3(4.0f, 0, 0), glm::vec3(0, 3.0f, 0));
            
            glm::vec3 dir, color, normal;
            
            PrimitiveList<float> list;
            list.push_back(std::shared_ptr<Sphere<float>>(
                                                          new Sphere<float>(glm::vec3(0,0,-1.0f), 0.5f)));
            
            list.push_back(std::shared_ptr<Sphere<float>>(
                                                          new Sphere<float>(glm::vec3(0,-100.5f, 1.0f), 100.0f)));
            
            XORUniformRNG<float> rng;
            PcgHash hash;
            
            float weigth = 1.0f / (float)samples;
            
            for(int j = height - 1; j >= 0; --j)
            {
                for(int i = 0; i < width; ++i)
                {
                    color = glm::vec3(0);
                    
                    rng.seed(hash(i, j));
                    
                    for(int s = 0; s < samples; ++s)
                    {
                        u = ((float)i + rng()) / (float)width;
                        v = ((float)j + rng()) / (float)height;
                        
                        ray = cam.getRay(u, v);
                        
                        bool hit = list.intersect_simple(ray, t, idx);
                        
                        if (hit)
                        {
                            normal = list[idx]->normalAt(ray.operator()(t));
                            color += weigth * (0.5f * glm::vec3(normal.x + 1, normal.y + 1, normal.z + 1));
                        }
                        else
                        {
                            t = 0.5f * ray.dir.y + 0.5f;
                            color += weigth * ((1.0f - t) * glm::vec3(1.0f, 1.0f, 1.0f) + t * glm::vec3(0.5f, 0.7f, 1.0f));
                        }
                    }
                    
                    idx = 3 * ((height - j - 1) * width + i);
                    buffer[idx + 0] = color.r;
                    buffer[idx + 1] = color.g;
                    buffer[idx + 2] = color.b;
                }
            }
            
            pt::write_ppm<float>(buffer, width, height, 3, true, false, "aa_normals.ppm");
            
            free(buffer);
        }
        
        void test_primitive_list_ppm(unsigned int width, unsigned int height)
        {
            float* buffer = (float*)malloc(3 * width * height * sizeof(float));
            float u, v, t;
            size_t idx;
            pt::Ray<float> ray;
            
            glm::vec3 origin(0,0,0);
            glm::vec3 dir, color, normal;
            glm::vec3 lower_left(-2.0f,-1.5f,-1.0f);
            glm::vec3 hor(4.0f, 0, 0);
            glm::vec3 ver(0, 3.0f, 0);
            
            PrimitiveList<float> list;
            list.push_back(std::shared_ptr<Sphere<float>>(
                new Sphere<float>(glm::vec3(0,0,-1.0f), 0.5f)));
            
            list.push_back(std::shared_ptr<Sphere<float>>(
                new Sphere<float>(glm::vec3(0,-100.5f, 1.0f), 100.0f)));
            
            for(int j = height - 1; j >= 0; --j)
            {
                for(int i = 0; i < width; ++i)
                {
                    u = (float)i / (float)width;
                    v = (float)j / (float)height;
                    dir = glm::normalize(lower_left + u * hor + v * ver);
                    ray = Ray<float>(origin, dir);
                    
                    bool hit = list.intersect_simple(ray, t, idx);
                    
                    if (hit)
                    {
                        normal = list[idx]->normalAt(ray.operator()(t));
                        color = 0.5f * glm::vec3(normal.x + 1, normal.y + 1, normal.z + 1);
                    }
                    else
                    {
                        t = 0.5f * ray.dir.y + 0.5f;
                        color = (1.0f - t) * glm::vec3(1.0f, 1.0f, 1.0f) + t * glm::vec3(0.5f, 0.7f, 1.0f);
                    }
                    
                    idx = 3 * ((height - j - 1) * width + i);
                    buffer[idx + 0] = color.r;
                    buffer[idx + 1] = color.g;
                    buffer[idx + 2] = color.b;
                }
            }
            
            pt::write_ppm<float>(buffer, width, height, 3, true, false, "normals.ppm");
            
            free(buffer);
        }
        
        void test_normals_ppm(unsigned int width, unsigned int height)
        {
            float* buffer = (float*)malloc(3 * width * height * sizeof(float));
            float u, v, t;
            unsigned int idx;
            pt::Ray<float> ray;
            
            glm::vec3 origin(0,0,0);
            glm::vec3 dir, color, normal;
            glm::vec3 lower_left(-2.0f,-1.5f,-1.0f);
            glm::vec3 hor(4.0f, 0, 0);
            glm::vec3 ver(0, 3.0f, 0);
            
            Sphere<float> sphere(glm::vec3(0,0,-1.0f), 0.5f);
            
            for(int j = height - 1; j >= 0; --j)
            {
                for(int i = 0; i < width; ++i)
                {
                    u = (float)i / (float)width;
                    v = (float)j / (float)height;
                    dir = glm::normalize(lower_left + u * hor + v * ver);
                    ray = Ray<float>(origin, dir);
                    
                    t = sphere.intersect_simple(ray);
                    
                    if (t > 0.0)
                    {
                        normal = sphere.normalAt(ray.operator()(t));
                        color = 0.5f * glm::vec3(normal.x + 1, normal.y + 1, normal.z + 1);
                    }
                    else
                    {
                        t = 0.5f * ray.dir.y + 0.5f;
                        color = (1.0f - t) * glm::vec3(1.0f, 1.0f, 1.0f) + t * glm::vec3(0.5f, 0.7f, 1.0f);
                    }
                    
                    idx = 3 * (j * width + i);
                    buffer[idx + 0] = color.r;
                    buffer[idx + 1] = color.g;
                    buffer[idx + 2] = color.b;
                }
            }
            
            pt::write_ppm<float>(buffer, width, height, 3, true, false, "normals.ppm");
            
            free(buffer);
        }
        
        void test_sphere_ppm(unsigned int width, unsigned int height)
        {
            float* buffer = (float*)malloc(3 * width * height * sizeof(float));
            float u, v, t;
            unsigned int idx;
            pt::Ray<float> ray;
            
            glm::vec3 origin(0,0,0);
            glm::vec3 dir, color;
            glm::vec3 lower_left(-2.0f,-1.5f,-1.0f);
            glm::vec3 hor(4.0f, 0, 0);
            glm::vec3 ver(0, 3.0f, 0);
            
            Sphere<float> sphere(glm::vec3(0,0,-1.0f), 0.5f);
            
            for(int j = height - 1; j >= 0; --j)
            {
                for(int i = 0; i < width; ++i)
                {
                    u = (float)i / (float)width;
                    v = (float)j / (float)height;
                    dir = glm::normalize(lower_left + u * hor + v * ver);
                    ray = Ray<float>(origin, dir);
                    
                    if (sphere.intersect_check(ray))
                        color = glm::vec3(1.0f, 0, 0);
                    else
                    {
                        t = 0.5f * ray.dir.y + 0.5f;
                        color = (1.0f - t) * glm::vec3(1.0f, 1.0f, 1.0f) + t * glm::vec3(0.5f, 0.7f, 1.0f);
                    }
                    
                    idx = 3 * (j * width + i);
                    buffer[idx + 0] = color.r;
                    buffer[idx + 1] = color.g;
                    buffer[idx + 2] = color.b;
                }
            }
            
            pt::write_ppm<float>(buffer, width, height, 3, true, false, "sphere.ppm");
            
            free(buffer);
        }
        
        void test_ray_ppm(unsigned int width, unsigned int height)
        {
            float* buffer = (float*)malloc(3 * width * height * sizeof(float));
            float u, v, t;
            unsigned int idx;
            pt::Ray<float> ray;
            
            glm::vec3 origin(0,0,0);
            glm::vec3 dir, color;
            glm::vec3 lower_left(-2.0f,-1.0f,-1.0f);
            glm::vec3 hor(4.0f, 0, 0);
            glm::vec3 ver(0, 2.0f, 0);
            
            for(int j = height - 1; j >= 0; --j)
            {
                for(int i = 0; i < width; ++i)
                {
                    u = (float)i / (float)width;
                    v = (float)j / (float)height;
                    dir = glm::normalize(lower_left + u * hor + v * ver);
                    ray = Ray<float>(origin, dir);
                    t = 0.5f * ray.dir.y + 0.5f;
                    color = (1.0f - t) * glm::vec3(1.0f, 1.0f, 1.0f) + t * glm::vec3(0.5f, 0.7f, 1.0f);
                    
                    idx = 3 * (j * width + i);
                    buffer[idx + 0] = color.r;
                    buffer[idx + 1] = color.g;
                    buffer[idx + 2] = color.b;
                }
            }
            
            pt::write_ppm<float>(buffer, width, height, 3, true, false, "ray.ppm");
            
            free(buffer);
        }
        
        void test_noise_ppm(unsigned int width, unsigned int height)
        {
            float* buffer = (float*)malloc(3 * width * height * sizeof(float));
            float r;

            unsigned int idx;
            
            XORUniformRNG<float> rng;
            PcgHash hash;
            
            for(int j = height - 1; j >= 0; --j)
            {
                for(int i = 0; i < width; ++i)
                {
                    rng.seed(hash(i, j));
                    r = rng();
                    idx = 3 * (j * width + i);
                    buffer[idx + 0] = r;
                    buffer[idx + 1] = r;
                    buffer[idx + 2] = r;
                }
            }
            
            pt::write_ppm<float>(buffer, width, height, 3, true, false, "noise.ppm");
            
            free(buffer);
        }
        
        void test_gradient_ppm(unsigned int width, unsigned int height)
        {
            float* buffer = (float*)malloc(3 * width * height * sizeof(float));
            float r, g, b;
            b = 0.2f;
            unsigned int idx;
            
            for(int j = height - 1; j >= 0; --j)
            {
                for(int i = 0; i < width; ++i)
                {
                    r = (float)i / (float)width;
                    g = (float)j / (float)height;
                    idx = 3 * (j * width + i);
                    buffer[idx + 0] = r;
                    buffer[idx + 1] = g;
                    buffer[idx + 2] = b;
                }
            }
            
            pt::write_ppm<float>(buffer, width, height, 3, true, false, "rg_gradient.ppm");
            
            free(buffer);
        }
    }
}

#endif /* ptTests_h */
