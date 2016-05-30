#ifndef __CL_RENDERING_H__
#define __CL_RENDERING_H__

#include "geometry.cl"
#include "material.cl"

#ifndef MAX_RECURSION
#define MAX_RECURSION 5
#endif

static float3 radiance_iterative_recursion_map(Ray* ray,
  __constant Sphere* primitive_list,
  __constant Material* material_list,
  uint primitive_list_size,
  uint* rng_state)
{
  float t;
  uint idx;

  Ray ray_in = *ray;
  Ray ray_out;

  float3 attenuation;
  float3 p;
  float3 normal;

  int i;

  for(i = 0; i < MAX_RECURSION; ++i)
  {
      if (sphere_list_intersect(primitive_list, primitive_list_size, &ray_in, &t, &idx))
      {
          p = ray_pointat(&ray_in, t); // where intersects
          normal = sphere_normal_at(&primitive_list[idx], p); // normal at intersection

          if(material_scatter(&material_list[idx], &ray_in, p, normal, rng_state, &attenuation, &ray_out))
          {
              //col *= attenuation;
              ray_in = ray_out;
          }
          else
          {
              //col *= (float3)(0,0,0);
              break;
          }
      }
      else
      {
          //t = 0.5f * ray_in.dir.y + 0.5f;
          //col *= ((1.0f - t) * sky->bottom + t * sky->top);
          break;
      }
  }

  if(i == 0)
    return (float3)(1.0f,0,0);
  else if(i == 1)
    return (float3)(0,0,1.0f);
  else if(i == 2)
    return (float3)(0,1.0f,0);
  else if(i == 3)
    return (float3)(1.0f,1.0f,0);
  else
    return (float3)(0,1.0f,1.0f);
}

static float3 radiance_iterative(Ray* ray,
  __constant Sphere* primitive_list,
  __constant Material* material_list,
  uint primitive_list_size,
  uint* rng_state,
  __constant SkyMaterial* sky)
{

  float t;
  uint idx;
  float3 col = (float3)(1,1,1);

  Ray ray_in = *ray;
  Ray ray_out;

  float3 attenuation;
  float3 p;
  float3 normal;

  for(int i = 0; i < MAX_RECURSION; ++i)
  {
      if (sphere_list_intersect(primitive_list, primitive_list_size, &ray_in, &t, &idx))
      {
          p = ray_pointat(&ray_in, t); // where intersects
          normal = sphere_normal_at(&primitive_list[idx], p); // normal at intersection

          if(material_scatter(&material_list[idx], &ray_in, p, normal, rng_state, &attenuation, &ray_out))
          {
              col *= attenuation;
              ray_in = ray_out;
          }
          else
          {
              col = (float3)(0,0,0);
              break;
          }
      }
      else
      {
          t = 0.5f * ray_in.dir.y + 0.5f;
          col *= ((1.0f - t) * sky->bottom + t * sky->top);
          break;
      }
  }

  return col;
}

#endif //__CL_RENDERING_H__
