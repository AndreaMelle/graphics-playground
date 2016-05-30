#define MAX_PRIMITIVES 10
#define MAX_RECURSION 5
//#define INVERT

#include "random.cl"
#include "utils.cl"
#include "geometry.cl"
#include "rendering.cl"

__kernel
void path_tracing(__constant struct PinholeCamera* cam,
                  __constant struct Sphere* primitive_list,
                  __constant struct Material* material_list,
                  __constant struct SkyMaterial* sky,
                  uint primitive_list_size,
                  write_only image2d_t out_buffer,
                  uint samples)
{
  

  int2 size  = (int2)(get_global_size(0), get_global_size(1));
  int2 coord = (int2)(get_global_id(0), get_global_id(1));

  uint seed = hash2(coord.x, coord.y);
  float3 out_color = (float3)(0,0,0);
  float weigth = 1.0f / (float)samples;

  float2 uv;
  float2 xy = convert_float2(coord);
  float2 wh = convert_float2(size);

  uint count = min(primitive_list_size, (uint)(MAX_PRIMITIVES));

  Ray ray;

  for(uint s = 0; s < samples; ++s)
  {
      uv = (xy + sample_unit_2D(&seed)) / wh;
#ifdef INVERT
      uv.y = 1.0f - uv.y;
#endif
      ray = pinhole_cam_ray(cam, uv);
      out_color = fma(weigth, radiance_iterative(&ray, primitive_list, material_list, count, &seed, sky), out_color);
      // out_color += weigth * radiance_iterative(&ray, primitive_list, material_list, count, &seed, sky), out_color);
      // out_color += weigth * radiance_iterative_recursion_map(&ray, primitive_list, material_list, count, &seed);
  }

  // uint4 pixel = (uint4)(to8U_C1_gamma(out_color.r), to8U_C1_gamma(out_color.g), to8U_C1_gamma(out_color.b), 255);
  float4 pixel = (float4)(to32F_C1_gamma(out_color.r), to32F_C1_gamma(out_color.g), to32F_C1_gamma(out_color.b), 1.0f);
  //write_imageui(out_buffer, coord, pixel);
  write_imagef(out_buffer, coord, pixel);
}
