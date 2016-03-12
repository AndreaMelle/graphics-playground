#ifndef __PT_RANDOM_H__
#define __PT_RANDOM_H__

#include <random>
#include "ptUtil.h"

namespace pt
{
	class Hash
	{
	public:
		virtual unsigned int operator()(int i) = 0;
		virtual unsigned int operator()(int i, int j) = 0;
	};

	inline int rot(int x, int b) { return (x << b) ^ (x >> (32 - b)); }

	/*
	 * https://groups.google.com/d/msg/proceduralcontent/AuvxuA1xqmE/mnw4AubN4jkJ
	 */
	class PcgHash : public Hash
	{
	public:
        unsigned int operator()(int i) override;
        unsigned int operator()(int i, int j) override;
	};
    
    /* Abstract class to represent a rng spitting uniform distribution numbers out */
    template<typename T>
    class UniformRNG
    {
    public:
        UniformRNG() {}
        /*
         * Always returns a random number in [0, 1) interval
         * Therefore T must float or double
         */
        virtual T operator()() = 0;
        virtual void seed(unsigned int _seed) = 0;
    };
    
    template<typename T>
    class STDUniformRNG : public UniformRNG<T>
	{
	public:
        STDUniformRNG(unsigned int _seed = 0);

        T operator()() override;
        void seed(unsigned int _seed) override;
        
    private:
		std::default_random_engine gen;
		std::uniform_real_distribution<T> udist;
	};
    
    template<typename T>
    class XORUniformRNG : public UniformRNG<T>
    {
    public:
        XORUniformRNG(unsigned int _seed = 0);
        
        T operator()() override;
        void seed(unsigned int _seed) override;
        
    private:
        unsigned int state;
    };
    
    typedef std::function<ptvec<float>(UniformRNG<float>& rng, const ptvec<float>&)> unit_sphere_samplerf;
    typedef std::function<ptvec<double>(UniformRNG<double>& rng, const ptvec<double>&)> unit_sphere_samplerd;
    
    /* Sample unit sphere by sampling unit cube and reject */
    template<typename T>
    static ptvec<T> sample_unit_sphere_rejection(UniformRNG<T>& rng, const ptvec<T>& normal)
    {
        ptvec<T> p;
        do {
            p = (T)2.0 * ptvec<T>(rng(), rng(), rng()) - ptvec<T>(1,1,1);
        } while (glm::length2(p) >= 1.0);
        
        return glm::normalize(normal + p);
    }
    
    /* Sampling unit hemisphere oriented along a normal by sampling a unit disk and project out */
    template<typename T>
    static ptvec<T> sampling_unit_hemisphere(UniformRNG<T>& rng, const ptvec<T>& normal)
    {
        T r1 = 2 * M_PI * rng(); // pick a random angle
        T r2 = rng();
        T r2s = sqrt(r2); // random distance from center
        
        ptvec<T> w, u, v;
        
        w = normal; //w, u, v ortonormal coordinate frame oriented along object surface
        
        if(w.x < w.y && w.x < w.z)
            u = glm::normalize(glm::cross(ptvec<T>(1.0f, 0, 0), w));
        else if(w.y < w.z)
            u = glm::normalize(glm::cross(ptvec<T>(0, 1.0f, 0), w));
        else
            u = glm::normalize(glm::cross(ptvec<T>(0, 0, 1.0f), w));
        
        v = glm::cross(w, u);
        
        return glm::normalize(u * (T)cos(r1) * r2s + v * (T)sin(r1) * r2s + w * (T)sqrt(1 - r2));
    }
    
}

#endif //__PT_RANDOM_H__