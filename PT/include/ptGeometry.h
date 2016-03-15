//
//  ptGeometry.h
//  PT
//
//  Created by Andrea Melle on 11/03/2016.
//
//

#ifndef ptGeometry_h
#define ptGeometry_h

#include "glm/glm.hpp"
#include "ptUtil.h"
#include "ptRandom.h"

namespace pt
{
    
    
    /*
     * A parametric line of the form P(t) = origin + t * dir
     */
    template <typename T>
    class Ray
    {
    public:
        Ray(const ptvec<T>& _origin, const ptvec<T>& _dir);
        Ray(const Ray& other);
        Ray& operator=(const Ray& other);
        ptvec<T> operator()(const T& t) const;
        ptvec<T> origin;
        ptvec<T> dir;
        Ray();
    };
    
    template <typename T>
    class Camera
    {
    public:
        virtual Ray<T> getRay(const T& u, const T& v, UniformRNG<T>& rng) const = 0;
        
        virtual ptvec<T> getOrigin() const = 0;
        virtual ptvec<T> getLowerLeft() const = 0;
        virtual ptvec<T> getHor() const = 0;
        virtual ptvec<T> getVer() const = 0;
        
    };
    
    /*
     * Pinhole Camera
     */
    template <typename T>
    class PinholeCamera : public Camera<T>
    {
    public:
        PinholeCamera();
        
        PinholeCamera(const T& vfovdeg,
                      const T& aspect,
                      const ptvec<T>& eye,
                      const ptvec<T>& lookat,
                      const ptvec<T>& up);
        
        PinholeCamera(const ptvec<T>& _origin,
                      const ptvec<T>& _lower_left,
                      const ptvec<T>& _hor,
                      const ptvec<T>& _ver);
        
        Ray<T> getRay(const T& u, const T& v, UniformRNG<T>& rng) const;
        
        ptvec<T> getOrigin() const { return origin; }
        ptvec<T> getLowerLeft() const { return lower_left; }
        ptvec<T> getHor() const { return hor; }
        ptvec<T> getVer() const { return ver; }
        
    private:
        ptvec<T> origin;
        ptvec<T> lower_left;
        ptvec<T> hor;
        ptvec<T> ver;
    };
    
    /*
     * Lens Camera
     */
    template <typename T>
    class LensCamera : public Camera<T>
    {
    public:
        LensCamera();
        
        LensCamera(const T& vfovdeg,
                   const T& aspect,
                   const ptvec<T>& eye,
                   const ptvec<T>& lookat,
                   const ptvec<T>& up,
                   const T& aperture,
                   const T& focus_dist);
        
        Ray<T> getRay(const T& s, const T& t, UniformRNG<T>& rng) const;
        
        ptvec<T> getOrigin() const { return origin; }
        ptvec<T> getLowerLeft() const { return lower_left; }
        ptvec<T> getHor() const { return hor; }
        ptvec<T> getVer() const { return ver; }
        
    private:
        ptvec<T> origin;
        ptvec<T> lower_left;
        ptvec<T> hor;
        ptvec<T> ver;
        ptvec<T> u;
        ptvec<T> v;
        ptvec<T> w;
        T        lens_radius;
    };
    
    typedef Ray<float>  rayf;
    typedef Ray<double> rayd;
    
    template <typename T>
    class Primitive
    {
    public:
        Primitive(){}
        virtual T intersect(const pt::Ray<T>& ray) const = 0;
        virtual bool intersect_check(const pt::Ray<T>& ray) const = 0;
        virtual T intersect_simple(const pt::Ray<T>& ray) const = 0;
        virtual bool intersect_simple(const pt::Ray<T>& ray,
                                      T& t_out,
                                      const T& t_min = 0,
                                      const T& t_max = std::numeric_limits<T>::infinity()) const = 0;
        
        virtual ptvec<T> normalAt(const ptvec<T>& point) const = 0;
    };
    
    template <typename T>
    class PrimitiveList : public std::vector<std::shared_ptr<Primitive<T>>>
    {
    public:
        bool intersect_simple(const pt::Ray<T>& ray,
                              T& t_out,
                              size_t& idx_t,
                              const T& t_min = 0,
                              const T& t_max = std::numeric_limits<T>::infinity()) const;
    };
    
    /*
     * The basic primitive supported by the path tracer
     * (P - Center)^2 - radius^2 = 0
     */
    template <typename T>
    class Sphere : public Primitive<T>
    {
    public:
        Sphere(const ptvec<T>& _center, T _radius);
        
        /* Compute ray-sphere intersection */
        T intersect(const pt::Ray<T>& ray) const;
        
        /* simplified intersection test */
        bool intersect_check(const pt::Ray<T>& ray) const;
        
        /* simplified intersection test */
        T intersect_simple(const pt::Ray<T>& ray) const;
        
        bool intersect_simple(const pt::Ray<T>& ray,
                              T& t_out,
                              const T& t_min = 0,
                              const T& t_max = std::numeric_limits<T>::infinity()) const;
        
        ptvec<T> normalAt(const ptvec<T>& point) const;
        
        ptvec<T> getCenter() const;
        T getRadius() const;
        
        Sphere();
        Sphere(const Sphere& other);
        Sphere<T>& operator=(const Sphere& other);
        
    private:
        ptvec<T>	center;
        T           radius;
    };
    
    typedef std::shared_ptr<Sphere<float>>      fSphereRef;
    
    
    template<typename T>
    ptvec<T> reflect(const ptvec<T>& l, const ptvec<T>& n)
    {
        return glm::normalize( l - (T)2.0 * glm::dot(l, n) * n);
    }
    
    template<typename T>
    bool refract(const ptvec<T>& l, const ptvec<T>& n, const T& ni_over_nt, ptvec<T>& out_refracted)
    {
        T cosi = glm::dot(l, n);
        T cost2 = (T)1.0 - ni_over_nt * ni_over_nt * ((T)1.0 - cosi*cosi);
        if(cost2 > 0)
        {
            //out_refracted = ni_over_nt * l + ((ni_over_nt * cosi - (T)sqrt(std::abs(cost2))) * n);
            out_refracted = glm::normalize(ni_over_nt * (l - n*cosi) - n*(T)sqrt(cost2));
            return true;
        }
        
        return false;
    }
    
    template<typename T>
    T schlick(const T& cosine, const T& ref_idx)
    {
        T r0 = (1 - ref_idx) / (1 + ref_idx);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cosine), 5);
    }
    
    
}


#endif /* ptGeometry_h */
