#ifndef MAX_PRIMITIVES
#define MAX_PRIMITIVES 10
#endif

#ifndef MAX_RECURSION
#define MAX_RECURSION 5
#endif

#include "random.cl"
#include "utils.cl"
#include "geometry.cl"
#include "rendering.cl"

/*
 * This kernel generates n rays and write them into a buffer
 * Big conandrum: should I do this over a 1D or 2D buffer?
 * 1D buffer -> makes more sense to pipeline rays and stream of new rays
 * 2D buffer -> no need for expensive modulo operator when going from 1D to 2D (will happens twice)
 */

// TODO: support 2D buffer too with directive IF
//int2 ij = (int2)(get_global_id(0), get_global_id(1));

// We are launching threads in a 1D fashion, hence we need to compute xy from an index i
//float2 xy = convert_float2( in_index_to_coord[i] ); //TODO: get from precomputed table

//Expensive modulo operator
//int x = i % width; int y = i / width;

//Only if width is power of two
//int x = i & (width - 1); int y = i / width;

__kernel
void cam_rays_kernel(__global float3* out_ray_origin_buffer,
                     __global float3* out_ray_dir_buffer,
                     __constant struct PinholeCamera* cam,
                     uint width,
                     uint height,
                     uint samples)
{
  uint i = get_global_id(0);
  uint px_idx = i / samples;
  uint y = px_idx / width;
  uint x = px_idx - y * width;

  uint seed = hash2(x, y);

#ifdef INVERT
  float2 uv = (float2)(((float)x + sample_unit_1D(&seed)) / (float)width, 1.0f - ((float)y + sample_unit_1D(&seed)) / (float)height);
#else
  float2 uv = (float2)(((float)x + sample_unit_1D(&seed)) / (float)width, ((float)y + sample_unit_1D(&seed)) / (float)height);
#endif

  float3 dir = cam->lower_left + uv.x * cam->hor + uv.y * cam->ver - cam->origin;

  out_ray_origin_buffer[i] = cam->origin;
  out_ray_dir_buffer[i] = dir / sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

}

/*
 * This path tracing kernel is only used as a test against the camera ray generation implementation
 * Instead of creating rays from scratch, it uses the precomputed rays in the input buffer
 * In the future, the ray buffers will become read/write, when we break the problem in several passes and stream compaction
 */

 //NOTE: I cannot declare __constant a buffer which is created read/write !
__kernel
void path_tracing(__global float3* out_buffer,
  __global float3* in_ray_origin_buffer,
  __global float3* in_ray_dir_buffer,
  __constant struct Sphere* primitive_list,
  __constant struct Material* material_list,
  __constant struct SkyMaterial* sky,
  uint primitive_list_size,
  float weigth)
{
  int i = get_global_id(0);

  Ray ray;
  ray.origin = in_ray_origin_buffer[i];
  ray.dir = in_ray_dir_buffer[i];

  uint seed = hash(i);

  uint count = min(primitive_list_size, (uint)(MAX_PRIMITIVES));
  float3 out_color = fma(weigth, radiance_iterative(&ray, primitive_list, material_list, count, &seed, sky), out_color);

  /*
   * This is 1/weight-th of the final color so don't gamma correct this one
   */

  out_buffer[i] = out_color;
  // out_buffer[i] = (float3)(to32F_C1_gamma(out_color.r), to32F_C1_gamma(out_color.g), to32F_C1_gamma(out_color.b));
}

// /*
//  * Gather a width x height x samples buffer into a width x height image
//  */
// __kernel
// void kernel_gather_image(__global float4* in_pixelbuffer,
//                          write_only image2d_t out_img_buffer,
//                          uint width,
//                          uint height,
//                          uint samples)
// {
//   int i = get_global_id(0);
//
//   int y = i / width;
//   int x = i - y * height;
//
//   // float4 pixel;
//   // float weight = 1.0f / (float)(samples);
//   //
//   // for(int k = 0; k < samples; ++k)
//   // {
//   //   pixel += in_pixelbuffer[samples * i + k] * weight;
//   // }
//
//   float4 pixel = float4(1.0f, 0, 0, 1.0f);
//
//
//   pixel = (float4)(to32F_C1_gamma(pixel.r), to32F_C1_gamma(pixel.g), to32F_C1_gamma(pixel.b), 1.0f);
//   write_imagef(out_img_buffer, int2(x, y), pixel);
// }
