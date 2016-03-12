#include "random.clh"

__kernel
void random_image(write_only image2d_t dst_img)
{
  int2 coord = (int2)(get_global_id(0), get_global_id(1));
  uint seed = hash2(coord);
  uint val = (uint)((float)(UCHAR_MAX) * rng01(seed) + 0.5);

  uint4 pixel = (uint4)(val, 0, 0, 0);
  write_imageui(dst_img, coord, pixel);
}
