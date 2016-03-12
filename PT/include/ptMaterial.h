#ifndef ptMaterial_h
#define ptMaterial_h

#include "ptUtil.h"
#include "ptGeometry.h"

namespace pt
{
    template<typename T>
    class Material
    {
    public:
        virtual bool scatter(const Ray<T>&      ray_in,
                             const ptvec<T>&    hit_point,
                             const ptvec<T>&    hit_normal,
                             UniformRNG<T>&     rng,
                             ptvec<T>&          attenuation_out,
                             Ray<T>&            ray_out) const = 0;
    };
    
    template<typename T>
    class Lambertian : public Material<T>
    {
    public:
        Lambertian(const ptvec<T>& _albedo) : albedo(_albedo) { }
        
        bool scatter(const Ray<T>&      ray_in,
                     const ptvec<T>&    hit_point,
                     const ptvec<T>&    hit_normal,
                     UniformRNG<T>&     rng,
                     ptvec<T>&          attenuation_out,
                     Ray<T>&            ray_out) const
        {
#ifdef SAMPLE_HEMI
            ray_out = Ray<T>(hit_point, sampling_unit_hemisphere(rng, hit_normal));
#else
            ray_out = Ray<T>(hit_point, sample_unit_sphere_rejection(rng, hit_normal));
#endif
            attenuation_out = albedo;
            return true;
        }
        
    private:
        ptvec<T> albedo;
        
    };
    
    template<typename T>
    class Metallic : public Material<T>
    {
    public:
        Metallic(const ptvec<T>& _albedo, T _fuzz = 0) : albedo(_albedo)
        {
            fuzz = clamp(_fuzz);
        }
        
        bool scatter(const Ray<T>&      ray_in,
                     const ptvec<T>&    hit_point,
                     const ptvec<T>&    hit_normal,
                     UniformRNG<T>&     rng,
                     ptvec<T>&          attenuation_out,
                     Ray<T>&            ray_out) const
        {
            ray_out = Ray<T>(hit_point, reflect(ray_in.dir, hit_normal)
                             + fuzz * sample_unit_sphere_rejection(rng, ptvec<T>(0)));
            attenuation_out = albedo;
            return (glm::dot(ray_out.dir, hit_normal) > 0); //only use reflected 'outside' (i.e. not refracted?)
        }
        
    private:
        ptvec<T> albedo;
        T fuzz;
        
    };
}

#endif /* ptMaterial_h */
