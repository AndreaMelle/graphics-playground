#include "random.clh"
#include "utils.clh"
#include "geometry.clh"
#include "rendering.clh"

__kernel
void sizecheck(__global uint* sizes)
{
  uint sizeof_float = sizeof(float);
  uint sizeof_float3 = sizeof(float3);
  uint sizeof_cam = sizeof(PinholeCamera);
  uint sizeof_sphere = sizeof(Sphere);

  sizes[0] = sizeof_float;
  sizes[1] = sizeof_float3;
  sizes[2] = sizeof_cam;
  sizes[3] = sizeof_sphere;

}
