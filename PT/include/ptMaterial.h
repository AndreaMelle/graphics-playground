#ifndef ptMaterial_h
#define ptMaterial_h

#include "ptUtil.h"
#include "ptGeometry.h"

#define MAX_RECURSION 5
//#define SAMPLE_HEMI

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
    
    typedef std::shared_ptr<Material<float>>    fMaterialRef;
    
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
        
        ptvec<T> getAlbedo() const { return albedo; }
        
    private:
        ptvec<T> albedo;
        
    };
    
    typedef std::shared_ptr<Lambertian<float>>  fLambertianRef;
    
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
    
    template<typename T>
    class Dialectric : public Material<T>
    {
    public:
        Dialectric(T _refr_idx) : refr_idx(_refr_idx) { }
        
        bool scatter(const Ray<T>&      ray_in,
                     const ptvec<T>&    hit_point,
                     const ptvec<T>&    hit_normal,
                     UniformRNG<T>&     rng,
                     ptvec<T>&          attenuation_out,
                     Ray<T>&            ray_out) const
        {
            ptvec<T> reflected = reflect(ray_in.dir, hit_normal);
            ptvec<T> outward_normal, refracted;
            T ni_over_nt, cosine, reflect_prob;
            attenuation_out = ptvec<T>(1); // perfect dialectrics do not absorb stuff
            
            // entrance or exit?
            if(glm::dot(ray_in.dir, hit_normal) > 0)
            {
                outward_normal = -hit_normal;
                ni_over_nt = refr_idx;
                cosine = refr_idx * glm::dot(ray_in.dir, hit_normal);
            }
            else
            {
                outward_normal = hit_normal;
                ni_over_nt = 1 / refr_idx;
                cosine = -glm::dot(ray_in.dir, hit_normal);
            }
            
            if(refract(ray_in.dir, outward_normal, ni_over_nt, refracted))
            {
                reflect_prob = schlick(cosine, refr_idx);
                
                if(rng() < reflect_prob)
                    ray_out = Ray<T>(hit_point, reflected);
                else
                    ray_out = Ray<T>(hit_point, refracted);
            }
            else
            {
                ray_out = Ray<T>(hit_point, reflected);
            }
            
            return true;
        }
        
    private:
        T refr_idx;
        
    };
    
    template<typename T> T ray_min();
    template<typename T> T ray_max();
    
    template<> float ray_min<float>() { return (float)1e-4; }
    template<> float ray_max<float>() { return MAXFLOAT; }
    
    template<typename T>
    ptvec<T> color(const Ray<T>& ray,
                   const PrimitiveList<T>& list,
                   const std::vector<std::shared_ptr<Material<T>>>& materials,
                   UniformRNG<T>& rng,
                   int recursion_depth,
                   const ptvec<T>& bottom_sky_color = ptvec<T>(1.0, 1.0, 1.0),
                   const ptvec<T>& top_sky_color = ptvec<T>(0.5, 0.7, 1.0))
    {
        T t;
        size_t idx;
        bool hit;
        
        //
        if ((hit = list.intersect_simple(ray, t, idx, ray_min<T>(), ray_max<T>())))
        {
            ptvec<T> p = ray.operator()(t); // where intersects
            ptvec<T> normal = list[idx]->normalAt(p); // normal at intersection
            Ray<T> ray_out;
            ptvec<T> attenuation;
            
            if(recursion_depth < MAX_RECURSION && materials[idx]->scatter(ray, p, normal, rng, attenuation, ray_out))
                return attenuation * color(ray_out, list, materials, rng, recursion_depth + 1);
            else
                return ptvec<T>(0);
        }
        else
        {
            t = 0.5 * ray.dir.y + 0.5;
            return (((T)1.0 - t) * bottom_sky_color + t * top_sky_color);
        }
    }
    
    template<typename T>
    ptvec<T> color_iterative(const Ray<T>& ray,
                             const PrimitiveList<T>& list,
                             const std::vector<std::shared_ptr<Material<T>>>& materials,
                             UniformRNG<T>& rng,
                             const ptvec<T>& bottom_sky_color = ptvec<T>(1.0, 1.0, 1.0),
                             const ptvec<T>& top_sky_color = ptvec<T>(0.5, 0.7, 1.0))
    {
        T t;
        size_t idx;
        
        ptvec<T> col(1);
        pt::Ray<T> ray_in = ray;
        Ray<T> ray_out;
        ptvec<T> attenuation;
        ptvec<T> p;
        ptvec<T> normal;
        
        for(int i = 0; i < MAX_RECURSION; ++i)
        {
            if (list.intersect_simple(ray_in, t, idx, ray_min<T>(), ray_max<T>()))
            {
                p = ray_in.operator()(t); // where intersects
                normal = list[idx]->normalAt(p); // normal at intersection
                
                if(materials[idx]->scatter(ray_in, p, normal, rng, attenuation, ray_out))
                {
                    col *= attenuation;
                    ray_in = ray_out;
                }
                else
                {
                    col *= ptvec<T>(0);
                    break;
                }
            }
            else
            {
                t = 0.5 * ray_in.dir.y + 0.5;
                col *= (((T)1.0 - t) * bottom_sky_color + t * top_sky_color);
                break;
            }
            
            
        }
        
        return col;
        
    }
    
    
    
}

#endif /* ptMaterial_h */
