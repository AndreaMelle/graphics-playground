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
    
    /*
     * A very primitive camera
     * TODO: change parametrization with the classic view projection model?
     */
    template <typename T>
    class Camera
    {
    public:
        Camera();
        Camera(const ptvec<T>& _origin,
               const ptvec<T>& _lower_left,
               const ptvec<T>& _hor,
               const ptvec<T>& _ver);
        
        Ray<T> getRay(const T& u, const T& v);
        
    private:
        ptvec<T> origin;
        ptvec<T> lower_left;
        ptvec<T> hor;
        ptvec<T> ver;
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
    
    template<typename T>
    ptvec<T> reflect(const ptvec<T>& l, const ptvec<T>& n) { return l - (T)2.0 * glm::dot(l, n) * n; }
    
    template<typename T>
    bool refract(const ptvec<T>& l, const ptvec<T>& n, T ni_over_nt, ptvec<T>& out_refracted)
    {
        float dt = glm::dot(l, n);
        float d = (T)1.0 - ni_over_nt * ni_over_nt * ((T)1.0 - dt*dt);
        if(d > 0)
        {
            out_refracted = ni_over_nt * (l - n*dt) - n*sqrt(d);
            return true;
        }
        
        return false;
    }
    
}


#endif /* ptGeometry_h */
