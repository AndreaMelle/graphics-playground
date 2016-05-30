#define MAX_PRIMITIVES 10
#define MAX_RECURSION 5
//#define INVERT

#include "random.cl"
#include "utils.cl"
#include "geometry.cl"
#include "rendering.cl"

/*
 * This path tracing kernel is only used as a test against the camera ray generation implementation
 * Instead of creating rays from scratch, it uses the precomputed rays in the input buffer
 * In the future, the ray buffers will become read/write, when we break the problem in several passes and stream compaction
 */
__kernel
void path_tracing(__global float3* out_buffer,
  __constant float3* in_ray_origin_buffer,
  __constant float3* in_ray_dir_buffer,
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

  // out_buffer[i] = (float3)(to32F_C1_gamma(out_color.r), to32F_C1_gamma(out_color.g), to32F_C1_gamma(out_color.b));
  out_buffer[i] = (float3)(1.0f, 0, 0);
}
