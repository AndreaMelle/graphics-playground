#include <stdio.h>
#include "ptGeometry.h"

using namespace pt;

/* Ray impl */

template <typename T>
Ray<T>::Ray(const ptvec<T>& _origin, const ptvec<T>& _dir)
    : origin(_origin), dir(_dir) {}

template <typename T>
Ray<T>::Ray(const Ray<T>& other)
{
    origin = other.origin;
    dir = other.dir;
}

template <typename T>
Ray<T>& Ray<T>::operator=(const Ray<T>& other)
{
    if (this == &other) return *this;
    this->origin = other.origin;
    this->dir = other.dir;
    return *this;
}

template <typename T>
ptvec<T> Ray<T>::operator()(const T& t) const { return origin + t * dir; }

template <typename T>
Ray<T>::Ray(){}

template class pt::Ray<double>;
template class pt::Ray<float>;

/* Ray impl */

template <typename T>
Camera<T>::Camera() {}

template <typename T>
Camera<T>::Camera(const ptvec<T>& _origin, const ptvec<T>& _lower_left, const ptvec<T>& _hor, const ptvec<T>& _ver)
    : origin(_origin), lower_left(_lower_left), hor(_hor), ver(_ver) { }

template <typename T>
Ray<T> Camera<T>::getRay(const T& u, const T& v)
{
    return Ray<T>(origin, glm::normalize(lower_left + u * hor + v * ver - origin));
}

template class pt::Camera<double>;
template class pt::Camera<float>;

/* Sphere impl */

template <typename T>
Sphere<T>::Sphere(const ptvec<T>& _center, T _radius) : center(_center), radius(_radius) {}

template <typename T>
Sphere<T>::Sphere() {};

template <typename T>
Sphere<T>::Sphere(const Sphere<T>& other)
{
    center = other.center;
    radius = other.radius;
}

template <typename T>
Sphere<T>& Sphere<T>::operator=(const Sphere<T>& other)
{
    if (this == &other) return *this;
    this->center = other.center;
    this->radius = other.radius;
    return *this;
}

template <typename T>
T Sphere<T>::intersect(const pt::Ray<T>& ray) const
{
    ptvec<T> o_to_sphere = center - ray.origin;
    T t = (T)PT_EPSILON;
    T b = glm::dot(o_to_sphere, ray.dir); // 0.5 * b in quadratic equation of intersection
    T det = b * b - glm::dot(o_to_sphere, o_to_sphere) + radius * radius;
    if (det < 0) return 0; // ray misses sphere
    det = sqrt(det);
    return (t = b - det) > (T)PT_EPSILON ? t : ((t = b + det) > (T)PT_EPSILON ? t : 0); // smaller positive t, or 0 if sphere behind ray
}

template <typename T>
bool Sphere<T>::intersect_check(const pt::Ray<T>& ray) const
{
    ptvec<T> sphere_to_o = ray.origin - center;
    T a = glm::dot(ray.dir, ray.dir);
    T b = 2.0 * glm::dot(sphere_to_o, ray.dir);
    T c = glm::dot(sphere_to_o, sphere_to_o) - radius * radius;
    T det = b * b - 4.0 * a * c;
    return (det > 0);
}

template <typename T>
T Sphere<T>::intersect_simple(const pt::Ray<T>& ray) const
{
    ptvec<T> sphere_to_o = ray.origin - center;
    T a = glm::dot(ray.dir, ray.dir);
    T b = 2.0 * glm::dot(sphere_to_o, ray.dir);
    T c = glm::dot(sphere_to_o, sphere_to_o) - radius * radius;
    T det = b * b - 4.0 * a * c;
    
    if(det < 0)
        return -1.0;
    else
        return (-b - sqrt(det)) / (2.0 * a);
}

template <typename T>
bool Sphere<T>::intersect_simple(const pt::Ray<T>& ray, T& t_out, const T& t_min, const T& t_max) const
{
    ptvec<T> sphere_to_o = ray.origin - center;
    T a = glm::dot(ray.dir, ray.dir);
    T b = glm::dot(sphere_to_o, ray.dir);
    T c = glm::dot(sphere_to_o, sphere_to_o) - radius * radius;
    T det = b * b - a * c;
    
    if(det > 0)
    {
        t_out = (-b - sqrt(det)) / a;
        if (t_out > t_min && t_out < t_max) return true;
        
        t_out = (-b + sqrt(det)) / a;
        if (t_out > t_min && t_out < t_max) return true;
    }
    
    return false;
}

template <typename T>
ptvec<T> Sphere<T>::normalAt(const ptvec<T>& point) const
{
    return glm::normalize((point - center));
}

template <typename T>
ptvec<T> Sphere<T>::getCenter() const { return center; }

template <typename T>
T Sphere<T>::getRadius() const { return radius; }

template class pt::Sphere<double>;
template class pt::Sphere<float>;

/* PrimitiveList impl */

template <typename T>
bool PrimitiveList<T>::intersect_simple(const pt::Ray<T>& ray, T& t_out, size_t& idx_t, const T& t_min, const T& t_max) const
{
    bool has_hit = false;
    T temp_t;
    t_out = std::numeric_limits<T>::infinity();
    
    for(int i = 0; i < this->size(); ++i)
    {
        if ((*this)[i]->intersect_simple(ray, temp_t, t_min, t_max))
        {
            if(temp_t < t_out)
            {
                has_hit = true;
                idx_t = i;
                t_out = temp_t;
            }
        }
    }
    
    return has_hit;
}

template class pt::PrimitiveList<double>;
template class pt::PrimitiveList<float>;


