#ifndef __PATH_TRACER_H__
#define __PATH_TRACER_H__

#include "glm/glm.hpp"
#include <memory>
#include <vector>

#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <iostream>
#include "ptRandom.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

#ifndef M_1_PI
#define M_1_PI      0.318309886183790671537767526745028724  /* 1/pi           */
#endif

#define USE_MT

namespace pt
{
	const double EPSILON = 1e-4; // 1e-4 in the original implementation

	/*
	 * A parametric line of the form P(t) = origin + t * dir
	*/
	class Ray
	{
	public:
		Ray(const glm::dvec3& _origin, const glm::dvec3& _dir) : origin(_origin), dir(_dir) {}
		glm::dvec3 origin;
		glm::dvec3 dir;

		Ray() = delete;
		//Ray(const Ray& other) = delete;
		void operator=(const Ray& other) = delete;
	};

	class Primitive
	{
	public:
		Primitive(){}
		virtual double intersect(const Ray& ray) const = 0;
		virtual glm::dvec3 normalAt(const glm::dvec3& point) const = 0;
        virtual glm::dvec3 getCenter() const = 0;
        virtual double getRadius() const = 0;
		Primitive(const Primitive& other) = delete;
		void operator=(const Primitive& other) = delete;
	};

	/*
	 * The basic primitive supported by the path tracer
	 * (P - Center)^2 - radius^2 = 0
	*/
	class Sphere : public Primitive
	{
	public:
		Sphere(const glm::dvec3& _center, double _radius) : center(_center), radius(_radius) {}
		glm::dvec3	center;
		double		radius;

		/*
		 * Compute ray-sphere intersection
		*/
		double intersect(const Ray& ray) const
		{
			glm::dvec3 o_to_sphere = center - ray.origin;
			double t = EPSILON;
			double b = glm::dot(o_to_sphere, ray.dir); // 0.5 * b in quadratic equation of intersection
			double det = b * b - glm::dot(o_to_sphere, o_to_sphere) + radius * radius;
			if (det < 0) return 0; // ray misses sphere
			det = sqrt(det);
			return (t = b - det) > EPSILON ? t : ((t = b + det) > EPSILON ? t : 0); // smaller positive t, or 0 if sphere behind ray
		}

		glm::dvec3 normalAt(const glm::dvec3& point) const
		{
			return glm::normalize((point - center));
		}
        
        glm::dvec3 getCenter() const { return center; }
        double getRadius() const { return radius; }

		Sphere() = delete;
		Sphere(const Sphere& other) = delete;
		void operator=(const Sphere& other) = delete;
	};

	class Material
	{
	public:
		typedef enum Type
		{
			DIFFUSE
		} Type;

		Material(const glm::dvec3& _color, Type _type, const glm::dvec3& _emission = glm::dvec3(0))
			: color(_color)
			, emission(_emission)
			, type(_type) {}

		glm::dvec3 color; //rgb in unbounded domain
		glm::dvec3 emission; //rgb in unbounded domain
		Type type;

		Material() = delete;
		Material(const Material& other) = delete;
		void operator=(const Material& other) = delete;
	};


	class Renderable
	{
	public:
		Renderable(const std::shared_ptr<Primitive>& _primitive, const std::shared_ptr<Material>& _mat) : primitive(_primitive), mat(_mat) {}
		std::shared_ptr<Primitive>	primitive;
		std::shared_ptr<Material>	mat;

		Renderable() = delete;
		Renderable(const Renderable& other) = delete;
		void operator=(const Renderable& other) = delete;
	};

	class Scene
	{
	public:
		Scene(){}
		std::vector<std::shared_ptr<Renderable> >	renderables;

		//Scene(const Scene& other) = delete;
		void operator=(const Scene& other) = delete;
	};

	typedef std::shared_ptr<Renderable> RenderableRef;
	typedef std::shared_ptr<Material>	MaterialRef;
	typedef std::shared_ptr<Primitive>	PrimitiveRef;
	typedef std::shared_ptr<Sphere>		SphereRef;
	typedef std::shared_ptr<Ray>		RayRef;

	inline pt::RenderableRef CreateSphereRenderable(const glm::dvec3& _center, double _radius, const glm::dvec3& _color, Material::Type _type, const glm::dvec3& _emission = glm::dvec3(0))
	{
		return pt::RenderableRef(new pt::Renderable(
			pt::SphereRef(new pt::Sphere(_center, _radius)),
			pt::MaterialRef(new pt::Material(_color, _type, _emission))
		));
	}

	//clamp
	inline double clamp(double x) { return x < 0 ? 0 : x > 1 ? 1 : x; }
	//scale to rgb 255 values with clamp and gamma correction
	//inline char to255(double x) { return char(pow(clamp(x), 1./2.2) * 255. + 0.5); }
    inline int to255(double x) { return int(pow(clamp(x), 1./2.2) * 255. + 0.5); }

	/*
	 * Intersects ray with a scene and keep closest intersection found
	 * writes distance t from ray origin and index of object in scene
	 * this is where a space data structure could help speed up
	 */
	inline bool intersect(const Ray& ray, const Scene& scene, double& out_t, size_t &out_idx)
	{
		size_t n = scene.renderables.size();
		double d;
        out_t = 1e20;//std::numeric_limits<double>::max();

		for (size_t i = 0; i < n; ++i)
		{
			if ((d = scene.renderables[i]->primitive->intersect(ray)) && d < out_t)
			{
				out_t = d;
				out_idx = i;
			}
		}

        return out_t < 1e20;//std::numeric_limits<double>::max();
	}

	

	// E: whether we are considering emittance or not
	static glm::dvec3 radiance(const Ray& ray, const Scene& scene, int depth, URNG& rng, int E = 1)
	{
		double t; size_t idx = 0;
		if (!intersect(ray, scene, t, idx)) return glm::dvec3(0);

		std::shared_ptr<Primitive> obj = scene.renderables[idx]->primitive;
        std::shared_ptr<Material> objMat = scene.renderables[idx]->mat;

		glm::dvec3 x = ray.origin + t * ray.dir; //where we intersected
		glm::dvec3 n = obj->normalAt(x);
		glm::dvec3 orn = glm::dot(n, ray.dir) < 0 ? n : (n*-1.); //oriented surface normal
		glm::dvec3 color = objMat->color;

		// Russian rulette technique uses max component on r,g,b of surface color after depth 5 to cut recursion
		double p = color.x > color.y && color.x > color.z ? color.x : color.y > color.z ? color.y : color.z;
		if (++depth > 5 || !p)
		{
			if (rng() < p)
				color = color * (1. / p);
			else
				return objMat->emission * (double)E;
		}
		
        if(Material::DIFFUSE == scene.renderables[idx]->mat->type)
        {
            double r1 = 2 * M_PI * rng(); // pick a random angle around
            double r2 = rng();
            double r2s = sqrt(r2); //pick a random distance from center
            
            //w, u, v ortonormal coordinate frame oriented along object surface at point of intersection
            glm::dvec3 w = orn;
            glm::dvec3 u = glm::normalize(glm::cross((fabs(w.x) > .1) ? glm::dvec3(0,1,0) : glm::dvec3(1, 0, 0), w));
            glm::dvec3 v = glm::cross(w, u);
            
            // d is a random reflection ray (this is the unit hemisphere sampling formula
            glm::dvec3 d = glm::normalize(u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1 - r2));
            
            //loop through all explicit lights
            glm::dvec3 e;
            for(size_t i = 0; i < scene.renderables.size(); ++i)
            {
                std::shared_ptr<Material> mat = scene.renderables[i]->mat;
                std::shared_ptr<Primitive> prm = scene.renderables[i]->primitive;
                
                if(mat->emission.x <= 0 && mat->emission.y <= 0 && mat->emission.z <= 0) continue; //not a light
                
                //create random direction towards sphere
                glm::dvec3 sw = prm->getCenter() - x;
                glm::dvec3 su = glm::normalize(glm::cross(fabs(sw.x) > .1 ? glm::dvec3(0,1,0) : glm::dvec3(1,0,0), sw));
                
                glm::dvec3 sv = glm::cross(sw, su);
                
                double cos_a_max = sqrt(1. - prm->getRadius() * prm->getRadius() / glm::dot(x - prm->getCenter(), x - prm->getCenter()));
                
                double eps1 = rng(); double eps2 = rng();
                
                double cos_a = 1. - eps1 + eps1 * cos_a_max;
                double sin_a = sqrt(1 - cos_a * cos_a);
                double phi = 2. * M_PI * eps2;
                glm::dvec3 l = su * cos(phi) * sin_a + sv * sin(phi) * sin_a + sw * cos_a;
                l = glm::normalize(l);
                
                // shadow ray
                if(intersect(Ray(x, l), scene, t, idx) && idx == i)
                {
                    double omega = 2. * M_PI * (1 - cos_a_max);
                    glm::dvec3 temp = mat->emission * glm::dot(l, orn) * omega;
                    
                    //Compute 1/probability with respect to solid angle
                    e = e + glm::dvec3(color.x * temp.x, color.y * temp.y, color.z * temp.z) * M_1_PI;
                }
            }
            
            glm::dvec3 prev = radiance(Ray(x, d), scene, depth, rng, 0);
            
            return (objMat->emission * (double)E
                    + e
                    + glm::dvec3(prev.x * color.x, prev.y * color.y, prev.z * color.z));
            
        }
        else
        {
            throw "Other material types not yet implemented.";
        }
	}

	class PathTracer
	{
	public:
		std::atomic<int> tileCounter;

		void TraceTile(const Scene& scene,
			unsigned int from_x,
			unsigned int to_x,
			unsigned int from_y,
			unsigned int to_y,
			unsigned int width,
			unsigned int height,
			unsigned int samples,
			const Ray& cam,
			glm::dvec3 cx,
			glm::dvec3 cy,
			glm::dvec3* out_buffer)
		{
			glm::dvec3 r; //helper for accumulating colors
			URNG rng;
			PcgHash hash;

			for (unsigned int y = from_y; y < to_y; ++y)
			{
				for (unsigned int x = from_x; x < to_x; ++x)
				{
					// For each pixel we do 2x2 subpixels, and for each subpixel we draw samples samples
					for (unsigned int sy = 0, i = (height - y - 1) * width + x; sy < 2; ++sy)
					{
						/*
						 * In original implementation, rng gets seeded at each pixel
						 * therefore we can't use a global generator, because other threads would see him any time?
						 */

						rng.seed(hash(x, y));

						for (unsigned int sx = 0; sx < 2; ++sx, r = glm::dvec3(0))
						{
							for (int s = 0; s < samples; ++s)
							{
								// tent filter based sampling of the 2x2 area
								double r1 = 2.0 * rng();
								double r2 = 2.0 * rng();
								double dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
								double dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);

								glm::dvec3 d = cx * (((sx + .5 + dx) / 2 + x) / width - .5)
									+ cy * (((sy + .5 + dy) / 2 + y) / height - .5)
									+ cam.dir;

								// weighted by num samples
								r = r + radiance(Ray(cam.origin + d * 140.0, glm::normalize(d)), scene, 0, rng) * (1. / (double)samples);
							}

							// what's with the .25?
							out_buffer[i] = out_buffer[i] + glm::dvec3(clamp(r.x), clamp(r.y), clamp(r.z)) * 0.25;

						}
					}
				}
			}

			tileCounter++;
		}

		void Trace(const Scene& scene,
			unsigned int width,
			unsigned int height,
			unsigned int samples,
			const glm::dvec3& camPos,
			const glm::dvec3& camDir,
			double camFovRadians,
            char* out_buffer,
			double* rendertime)
		{
			samples /= 4; // dim of multi-sampled area
			samples = (samples > 0) ? samples : 1;

			Ray cam(camPos, glm::normalize(camDir));
			glm::dvec3 cx((double)width * camFovRadians / (double)height, 0, 0); // x dir increment
			glm::dvec3 cy = glm::normalize(glm::cross(cx, cam.dir)) * camFovRadians; //y dir increment

			glm::dvec3 r; //helper for accumulating colors
			glm::dvec3* buffer = new glm::dvec3[width * height]; //buffer for image rendering

			tileCounter = 0;

			unsigned int num_tiles_x = 4;
			unsigned int num_tiles_y = 2;

			assert((width % num_tiles_x) == 0);
			assert((height % num_tiles_y) == 0);

			unsigned int tile_width = width / num_tiles_x;
			unsigned int tile_height = height / num_tiles_y;

			std::chrono::time_point<std::chrono::high_resolution_clock> start, end;

			start = std::chrono::high_resolution_clock::now();

#ifdef USE_MT
			std::vector<std::shared_ptr<std::thread> > allThreads;
#endif

			for (int j = 0; j < num_tiles_y; ++j)
			{
				for (int i = 0; i < num_tiles_x; ++i)
				{
					unsigned int from_x = i * tile_width;
					unsigned int from_y = j * tile_height;
					unsigned int to_x = from_x + tile_width;
					unsigned int to_y = from_y + tile_height;

#ifdef USE_MT
                    allThreads.push_back(std::shared_ptr<std::thread>(new std::thread(&PathTracer::TraceTile, this,
#else
					TraceTile(
#endif
						scene,
						from_x, to_x, from_y, to_y,
						width, height, samples,
						cam, cx, cy,
						buffer)
#ifdef USE_MT
						))
#endif
						;
				}
			}

#ifdef USE_MT
			while (tileCounter < (num_tiles_x * num_tiles_y))
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			for (int i = 0; i < allThreads.size(); ++i)
				allThreads[i]->join();
#endif
            
			end = std::chrono::high_resolution_clock::now();

			std::chrono::duration<double> elapsed_seconds = end - start;
			*rendertime = elapsed_seconds.count();

            unsigned int j = 0;
            for(unsigned int i = 0; i < width * height; ++i)
            {
                out_buffer[j + 0] = to255(buffer[i].x);
                out_buffer[j + 1] = to255(buffer[i].y);
                out_buffer[j + 2] = to255(buffer[i].z);
                j += 3;
            }
            
            FILE *f = fopen("image.ppm", "w");         // Write image to PPM file.
            fprintf(f, "P3\n%d %d\n%d\n", width, height, 255);
            for (int i=0; i<width * height; i++)
                fprintf(f,"%d %d %d ", to255(buffer[i].x), to255(buffer[i].y), to255(buffer[i].z));

			delete[] buffer;
		}

        PathTracer() {}
		PathTracer(const PathTracer& other) = delete;
		void operator=(const PathTracer& other) = delete;
	};

}


#endif //__PATH_TRACER_H__