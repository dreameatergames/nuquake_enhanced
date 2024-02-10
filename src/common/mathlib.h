/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// mathlib.h

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec5_t[5];

typedef int fixed8_t;
typedef int fixed16_t;

#ifndef M_PI
#define M_PI 3.14159265358979323846f  // matches value in gcc v2 math.h
#endif

#ifdef _arch_dreamcast
#include <dc/fmath.h>
#include <dreamcast/sh4_math.h>

#define SIN(x) fsin(x)
#define COS(x) fcos(x)
#define SQRT(x) MATH_Fast_Sqrt(x)
#define FABS(x) MATH_fabs(x)
static inline void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross) {
  RETURN_VECTOR_STRUCT ret = MATH_Cross_Product(v1[0], v1[1], v1[2], v2[0], v2[1], v2[2]);
  cross[0] = ret.z1;
  cross[1] = ret.z2;
  cross[2] = ret.z3;
}

/* Functions */
static inline __attribute__((always_inline)) float DotProduct(const vec3_t x, const vec3_t y) {
  float fipr = MATH_fipr(x[0], x[1], x[2], 0.0f, y[0], y[1], y[2], 0.0f);
  /*
	// Debug fipr invalid values
	float reg = (x[0]*y[0]+x[1]*y[1]+x[2]*y[2]);
	if(fabsf(reg-fipr)> 0.01f)
		printf("reg: %2.03f\tfipr: %2.03f [%f, %f, %f] x [%f, %f, %f]\n", reg, fipr, x[0], x[1], x[2], y[0], y[1], y[2]);
	*/
  return fipr;
}

#elif !defined(PSP)
#define SIN(x) sinf(x)
#define COS(x) cosf(x)
#define SQRT(x) sqrtf(x)
#define FABS(x) fabsf(x)
#define DotProduct(x, y) (x[0] * y[0] + x[1] * y[1] + x[2] * y[2])

/* Functions */
static inline void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross) {
  cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
  cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
  cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}
#endif
#define FLOOR(x) floorf(x)

#define VectorSubtract(a, b, c) \
  {                             \
    c[0] = a[0] - b[0];         \
    c[1] = a[1] - b[1];         \
    c[2] = a[2] - b[2];         \
  }
#define VectorAdd(a, b, c) \
  {                        \
    c[0] = a[0] + b[0];    \
    c[1] = a[1] + b[1];    \
    c[2] = a[2] + b[2];    \
  }
#define VectorCopy(a, b) \
  {                      \
    b[0] = a[0];         \
    b[1] = a[1];         \
    b[2] = a[2];         \
  }

struct mplane_s;

extern vec3_t vec3_origin;

void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);

vec_t _DotProduct(vec3_t v1, vec3_t v2);
void _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorCopy(vec3_t in, vec3_t out);
INLINE_ALWAYS float VectorNormalize(vec3_t v) // returns vector length
{
	#ifdef _arch_dreamcast
  // x*x+y*y+z*z
  float temp = v[0] * v[0];
  temp = MATH_fmac(v[1], v[1], temp);
  temp = MATH_fmac(v[2], v[2], temp);

  float ilength = MATH_fsrra(temp);
  float length = MATH_Invert(ilength);
  if (length) {
    v[0] *= ilength;
    v[1] *= ilength;
    v[2] *= ilength;
  }

  return length;
	#else 
  float length, ilength;
  length = SQRT(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

  if (length) {
    ilength = 1 / length;
    v[0] *= ilength;
    v[1] *= ilength;
    v[2] *= ilength;
  }

  return length;
	#endif
}
int VectorCompare(vec3_t v1, vec3_t v2);
vec_t Length(vec3_t v);

void VectorInverse(vec3_t v);
void VectorScale(vec3_t in, vec_t scale, vec3_t out);
int Q_log2(int val);

void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);

int GreatestCommonDivisor(int i1, int i2);

void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
int BoxOnPlaneSide(const vec3_t emins, const vec3_t emaxs, struct mplane_s *plane);
static inline float anglemod(float a) {
  a = (360.0f / 65536) * ((int)(a * (65536 / 360.0f)) & 65535);
  return a;
}
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)                                                               \
BoxOnPlaneSide((emins), (emaxs), (p))

/*  (((p)->type < 3) ? (                                                                                   \
                         ((p)->dist <= (emins)[(p)->type]) ? 1                                           \
                                                           : (                                           \
                                                                 ((p)->dist >= (emaxs)[(p)->type]) ? 2   \
                                                                                                   : 3)) \
                   : BoxOnPlaneSide((emins), (emaxs), (p)))
*/