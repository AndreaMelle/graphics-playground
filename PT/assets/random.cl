#ifndef __CL_RANDOM_H__
#define __CL_RANDOM_H__

inline uint xor_shift_rng(uint *state)
{
  *state ^= (*state << 13);
  *state ^= (*state >> 17);
  *state ^= (*state << 5);
  return *state;
}

inline float sample_unit_1D (uint *state)
{
	return (
    (float)(xor_shift_rng(state)) / (float)(UINT_MAX)
  );
}

inline float2 sample_unit_2D (uint *state)
{
	return ((float2)(
    (float)(xor_shift_rng(state)) / (float)(UINT_MAX),
    (float)(xor_shift_rng(state)) / (float)(UINT_MAX)
  ));
}

static float3 sample_unit_3D (uint *state)
{
	return ((float3)(
    (float)(xor_shift_rng(state)) / (float)(UINT_MAX),
    (float)(xor_shift_rng(state)) / (float)(UINT_MAX),
    (float)(xor_shift_rng(state)) / (float)(UINT_MAX)
  ));
}

inline int rot(int x, int b) {
  return (x << b) ^ (x >> (32-b));
}

inline uint hash2(int i, int j) {
	int a = i;
	int b = j;
	for (int r = 0; r < 3; r++) {
		a = rot ((int) ((a^0xcafebabe) + (b^0xfaceb00c)), 23);
		b = rot ((int) ((a^0xdeadbeef) + (b^0x8badf00d)), 5);
	}
	return (uint)(a^b);
}

inline uint hash(int i) {
	int a = i;
	int b = 0;
	for (int r = 0; r < 3; r++) {
		a = rot ((int) ((a^0xcafebabe) + (b^0xfaceb00c)), 23);
		b = rot ((int) ((a^0xdeadbeef) + (b^0x8badf00d)), 5);
	}
	return (uint)(a^b);
}


/*
 * Sample unit sphere by sampling unit cube and reject
 * rng_state: current state used by xor random generator
 * normal: align sampled direction along this vector
 * a normalized random vector in unit sphere
 */
static float3 sample_unit_sphere_rejection(uint* rng_state, float3 normal)
{
    float3 p;
    do {
        p = 2.0f * sample_unit_3D(rng_state) - (float3)(1,1,1);
    } while (dot(p,p) >= 1.0f);

    return normalize(normal + p);
}

/*
 * Sample unit sphere by cosine weighted importance sampling
 * Favours ray directions closer to normal direction
 * Requires front facing normal?
 */
 static float3 sample_unit_hemisphere_cosweight(uint* rng_state, float3 normal)
 {
   float r1 = 2.0f * M_PI_F * sample_unit_1D(rng_state); //sample angle
   float r2 = sample_unit_1D(rng_state); //sample unit length
   float r2s = sqrt(r2);

   float3 w = normal;
   float3 up = fabs(w.x) > 0.1f ? (float3)(0, 1, 0) : (float3)(1, 0, 0);
   float3 u = normalize(cross(up, w));
   float3 v = cross(w,u);

   return normalize(u*cos(r1)*r2s + v*sin(r1)*r2s + w*sqrt(1.0f - r2));
 }

 #define SQRT_OF_ONE_THIRD     0.5773502691896257645091487805019574556476f

static float3 sample_unit_hemisphere_cosweight2(uint* rng_state, float3 normal)
{
  float r1 = sample_unit_1D(rng_state); //sample angle
  float r2 = sample_unit_1D(rng_state);

  float up = sqrt(r1); // cos(theta)
  float over = sqrt(1.0f - up * up); // sin(theta)
  float around = 2.0f * r2 * M_PI_F;

 	// Find any two perpendicular directions:
 	// Either all of the components of the normal are equal to the square root of one third, or at least one of the components of the normal is less than the square root of 1/3.
 	float3 directionNotNormal;
 	if (fabs(normal.x) < SQRT_OF_ONE_THIRD) {
 		directionNotNormal = (float3)(1, 0, 0);
 	} else if (fabs(normal.y) < SQRT_OF_ONE_THIRD) {
 		directionNotNormal = (float3)(0, 1, 0);
 	} else {
 		directionNotNormal = (float3)(0, 0, 1);
 	}
 	float3 perpendicular1 = normalize( cross(normal, directionNotNormal) );
 	float3 perpendicular2 =            cross(normal, perpendicular1); // Normalized by default.

     return ( up * normal ) + ( cos(around) * over * perpendicular1 ) + ( sin(around) * over * perpendicular2 );

 }

#endif //__CL_RANDOM_H__
