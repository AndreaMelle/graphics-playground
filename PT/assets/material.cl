#ifndef __CL_MATERIAL_H__
#define __CL_MATERIAL_H__

#include "random.cl"
#include "geometry.cl"

#define MAT_LAMBERTIAN  1
#define MAT_DIALECTRIC  2
#define MAT_METALLIC    3

typedef struct Material
{
  float4 albedo;
  char   type;
} Material;

typedef struct SkyMaterial
{
  float3 bottom;
  float3 top;
} SkyMaterial;

static bool material_scatter_lambertian(__constant Material* mat,
  Ray* ray_in,
  float3 hit_point,
  float3 hit_normal,
  uint *seed,
  float3* attenuation_out,
  Ray* ray_out)
{
  ray_out->origin = hit_point;
  ray_out->dir = sample_unit_sphere_rejection(seed, hit_normal);
  *attenuation_out = mat->albedo.rgb;
  return true;
}

static bool material_scatter_metallic(__constant Material* mat,
  Ray* ray_in,
  float3 hit_point,
  float3 hit_normal,
  uint *seed,
  float3* attenuation_out,
  Ray* ray_out)
{
  ray_out->origin = hit_point;
  ray_out->dir = normalize(pt_reflect(ray_in->dir, hit_normal) + mat->albedo.s3 * sample_unit_sphere_rejection(seed, (float3)(0,0,0)));
  *attenuation_out = mat->albedo.rgb;
  return (dot(ray_out->dir, hit_normal) > 0);
}

static bool material_scatter(__constant Material* mat,
  Ray* ray_in,
  float3 hit_point,
  float3 hit_normal,
  uint *seed,
  float3* attenuation_out,
  Ray* ray_out)
{
  if(mat->type == MAT_LAMBERTIAN)
  {
    return material_scatter_lambertian(mat, ray_in, hit_point, hit_normal, seed, attenuation_out, ray_out);
  }
  else if(mat->type == MAT_METALLIC)
  {
    return material_scatter_metallic(mat, ray_in, hit_point, hit_normal, seed, attenuation_out, ray_out);
  }

  return false;
}




#endif //__CL_MATERIAL_H__
