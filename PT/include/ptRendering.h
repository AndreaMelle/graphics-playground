//
#ifndef ptRendering_h
#define ptRendering_h

#include "ptUtil.h"
#include "ptGeometry.h"
#include "ptRandom.h"
#include "ptMaterial.h"

#define USE_ITERATIVE

namespace pt
{
    void render_pixel(unsigned int x, unsigned int y, unsigned int samples,
                      unsigned int width, unsigned int height,
                      const Camera<float>& cam,
                      const PrimitiveList<float>& list,
                      const std::vector<fMaterialRef>& materials,
                      float* out_buffer,
                      UniformRNG<float>& rng,
                      Hash& hash)
    {
        glm::vec3 c(0,0,0);
        float weigth = 1.0f / (float)samples;
        float u, v;
        size_t idx;
        
        pt::Ray<float> ray;
        
        rng.seed(hash(x, y));
        
        for(int s = 0; s < samples; ++s)
        {
            u = ((float)x + rng()) / (float)width;
            v = ((float)y + rng()) / (float)height;
            
            ray = cam.getRay(u, v, rng);
            
#ifdef USE_ITERATIVE
            c += weigth * color_iterative(ray, list, materials, rng);
#else
            c += weigth * color(ray, list, materials, rng, 0);
#endif
            
        }
        
        idx = 3 * ((height - y - 1) * width + x);
        out_buffer[idx + 0] = c.r;
        out_buffer[idx + 1] = c.g;
        out_buffer[idx + 2] = c.b;
    }
    
    void render_frame(unsigned int width, unsigned int height, unsigned int samples,
                      const Camera<float>& cam,
                      const PrimitiveList<float>& list,
                      const std::vector<fMaterialRef>& materials,
                      float* out_buffer,
                      UniformRNG<float>& rng,
                      Hash& hash)
    {
        
        for(int j = height - 1; j >= 0; --j)
        {
            for(int i = 0; i < width; ++i)
            {
                render_pixel(i, j, samples, width, height, cam, list, materials, out_buffer, rng, hash);
            }
        }
    }
}




#endif /* ptRendering_h */
