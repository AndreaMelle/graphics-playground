#ifndef __CL_UTILS_H__
#define __CL_UTILS_H__

/* scale scalar value to [0, 255] with clamp and gamma correction */
static uint to8U_C1_gamma(float x)
{
  return (uint)( pow(clamp(x, 0.0f, 1.0f), 1.0f/2.2f) * 255.0f + 0.5f );
}

/* clamp scalar value to [0, 1] gamma correction */
static float to32F_C1_gamma(float x)
{
  return ( pow(clamp(x, 0.0f, 1.0f), 1.0f/2.2f) );
}

/* clamp scalar value to [0, 1] gamma correction */
static float to32F_C1_linear(float x)
{
  return ( clamp(x, 0.0f, 1.0f) );
}

/* scale vector3 value to [0, 255] with clamp and gamma correction */
static uint3 to8U_C3_gamma(float3 x)
{
  return (uint3)( pow(clamp(x, 0.0f, 1.0f), 1.0f/2.2f) * 255.0f + (float3)(0.5f, 0.5f, 0.5f) );
}

#endif //__CL_UTILS_H__
