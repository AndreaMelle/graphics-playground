#ifndef __CL_GEOMETRY_H__
#define __CL_GEOMETRY_H__

#define t_min 1e-4f
#define t_max 0x1.fffffep+127f

/*
 * A parametric line of the form P(t) = origin + t * dir
 */
typedef struct Ray
{
  float3 origin;
  float3 dir;
} Ray;

/*
 * All we need to model a simple pinhole camera
 */
typedef struct PinholeCamera
{
  float3 origin;
  float3 lower_left;
  float3 hor;
  float3 ver;
} PinholeCamera;

typedef float4 Sphere; //center radius

// typedef struct Sphere
// {
//   float3 center;
//   float radius;
// } Sphere;

static float3 ray_pointat(Ray* ray, float t)
{
  return ray->origin + t * ray->dir;
}

inline Ray pinhole_cam_ray(__constant PinholeCamera* cam, float2 uv)
{
    Ray ray;
    ray.origin = cam->origin;
    ray.dir    = normalize(cam->lower_left + uv.x * cam->hor + uv.y * cam->ver - cam->origin);
    return ray;
}

static bool sphere_intersect(__constant Sphere* sphere, Ray* ray, float* t_out)
{
    float3 sphere_to_o = ray->origin - sphere->xyz;
    float a = dot(ray->dir, ray->dir);
    float b = dot(sphere_to_o, ray->dir);
    float c = dot(sphere_to_o, sphere_to_o) - sphere->w * sphere->w;
    float det = b * b - a * c;

    if(det > 0)
    {
        *t_out = (-b - sqrt(det)) / a;
        if (*t_out > t_min && *t_out < t_max) return true;

        *t_out = (-b + sqrt(det)) / a;
        if (*t_out > t_min && *t_out < t_max) return true;
    }

    return false;
}

static float3 sphere_normal_at(__constant Sphere* sphere, float3 point)
{
  return normalize((point - sphere->xyz) / sphere->w);
}

static bool sphere_list_intersect(__constant Sphere* primitive_list,
  uint primitive_list_size,
  Ray* ray,
  float* t_out,
  uint* idx_t)
{
    bool has_hit = false;
    float temp_t;
    *t_out = INFINITY;

    for(uint i = 0; i < primitive_list_size; ++i)
    {
        if (sphere_intersect(&primitive_list[i], ray, &temp_t))
        {
            if(temp_t < *t_out)
            {
                has_hit = true;
                *idx_t = i;
                *t_out = temp_t;
            }
        }
    }

    return has_hit;
}

static float3 pt_reflect(float3 l, float3 n)
{
  return normalize(l - 2.0f * dot(l, n) * n);
}

#endif //__CL_GEOMETRY_H__
