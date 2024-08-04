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
// mathlib.c -- math primitives

#include <math.h>
#include "quakedef.h"

void Sys_Error(char *error, ...);

vec3_t vec3_origin = {0, 0, 0};
int nanmask = 255 << 23;

/*-----------------------------------------------------------------*/

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal) {
#if defined(_arch_dreamcast) && defined(ENABLE_DC_MATH)
  float d;
  float inv_denom;

  inv_denom = MATH_Invert(DotProduct(normal, normal));

  d = DotProduct(normal, p) * inv_denom * inv_denom;

  dst[0] = MATH_fmac_Dec(normal[0], d, p[0]);
  dst[1] = MATH_fmac_Dec(normal[1], d, p[1]);
  dst[2] = MATH_fmac_Dec(normal[2], d, p[2]);
#else
  float d;
  vec3_t n;
  float inv_denom;

  inv_denom = 1.0F / DotProduct(normal, normal);

  d = DotProduct(normal, p) * inv_denom;

  n[0] = normal[0] * inv_denom;
  n[1] = normal[1] * inv_denom;
  n[2] = normal[2] * inv_denom;

  dst[0] = p[0] - d * n[0];
  dst[1] = p[1] - d * n[1];
  dst[2] = p[2] - d * n[2];
#endif
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector(vec3_t dst, const vec3_t src) {
  int pos;
  int i;
  float minelem = 1.0F;
  vec3_t tempvec;

  /*
	** find the smallest magnitude axially aligned vector
	*/
  for (pos = 0, i = 0; i < 3; i++) {
    if (FABS(src[i]) < minelem) {
      pos = i;
      minelem = FABS(src[i]);
    }
  }
  tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
  tempvec[pos] = 1.0F;

  /*
	** project the point onto the plane defined by src
	*/
  ProjectPointOnPlane(dst, tempvec, src);

  /*
	** normalize the result
	*/
  VectorNormalize(dst);
}

void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees) {
  float m[3][3];
  float im[3][3];
  float zrot[3][3];
  float tmpmat[3][3];
  float rot[3][3];
  int i;
  vec3_t vr, vup, vf;

  vf[0] = dir[0];
  vf[1] = dir[1];
  vf[2] = dir[2];

  PerpendicularVector(vr, dir);
  CrossProduct(vr, vf, vup);

  m[0][0] = vr[0];
  m[1][0] = vr[1];
  m[2][0] = vr[2];

  m[0][1] = vup[0];
  m[1][1] = vup[1];
  m[2][1] = vup[2];

  m[0][2] = vf[0];
  m[1][2] = vf[1];
  m[2][2] = vf[2];

  memcpy(im, m, sizeof(im));

  im[0][1] = m[1][0];
  im[0][2] = m[2][0];
  im[1][0] = m[0][1];
  im[1][2] = m[2][1];
  im[2][0] = m[0][2];
  im[2][1] = m[1][2];

  memset(zrot, 0, sizeof(zrot));
  /*zrot[0][0] = zrot[1][1] = */
  zrot[2][2] = 1.0F;
#if defined(_arch_dreamcast) && defined(ENABLE_DC_MATH)
  fsincos(degrees, &zrot[0][1], &zrot[0][0]);
  fsincos(degrees, &zrot[1][0], &zrot[1][1]);
  zrot[1][0] *= -1.0f;
#else
  zrot[0][0] = COS(DEG2RAD(degrees));
  zrot[0][1] = SIN(DEG2RAD(degrees));
  zrot[1][0] = -SIN(DEG2RAD(degrees));
  zrot[1][1] = COS(DEG2RAD(degrees));
#endif

  R_ConcatRotations(m, zrot, tmpmat);
  R_ConcatRotations(tmpmat, im, rot);

  for (i = 0; i < 3; i++) {
    dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
  }
}

/*-----------------------------------------------------------------*/
/*
float anglemod(float a) {
#if 0
	if (a >= 0)
		a -= 360*(int)(a/360);
	else
		a += 360*( 1 + (int)(-a/360) );
#endif
  a = (360.0f / 65536) * ((int)(a * (65536 / 360.0f)) & 65535);
  return a;
}
*/

/*
==================
BOPS_Error

Split out like this for ASM to call.
==================
*/
void BOPS_Error(void) {
  Sys_Error("BoxOnPlaneSide:  Bad signbits");
}

#if !id386

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide(const vec3_t emins, const vec3_t emaxs, mplane_t *p) {
#if 1
  float dist1 = 0, dist2 = 0;
  int sides;
  const int p_signbits = p->signbits;
  const int p_dist = p->dist;
  const int p_type = p->type;

  const float normal_x = p->normal[0];
  const float normal_y = p->normal[1];
  const float normal_z = p->normal[2];
  const float emin_x = emins[0];
  const float emin_y = emins[1];
  const float emin_z = emins[2];
  const float emax_x = emaxs[0];
  const float emax_y = emaxs[1];
  const float emax_z = emaxs[2];

// this is done by the BOX_ON_PLANE_SIDE macro before calling this function, but not when called directly
// fast axial cases
  if(p_type < 3){
    if(p_type == 0){
      if (p_dist <= emin_x)
        return 1;
      if (p_dist >= emax_x)
        return 2;
      return 3;
    }
    if(p_type == 1){
      if (p_dist <= emin_y)
        return 1;
      if (p_dist >= emax_y)
        return 2;
      return 3;
    }
    if(p_type == 2){
      if (p_dist <= emin_z)
        return 1;
      if (p_dist >= emax_z)
        return 2;
		  return 3;
    }
		/*
    if (p_dist <= emins[p->type])
			return 1;
		if (p_dist >= emaxs[p->type])
			return 2;
    */
  }

  // general case
  switch (p_signbits) {
    case 0:
      dist1 = normal_x * emax_x + normal_y * emax_y + normal_z * emax_z;
      dist2 = normal_x * emin_x + normal_y * emin_y + normal_z * emin_z;
      break;
    case 1:
      dist1 = normal_x * emin_x + normal_y * emax_y + normal_z * emax_z;
      dist2 = normal_x * emax_x + normal_y * emin_y + normal_z * emin_z;
      break;
    case 2:
      dist1 = normal_x * emax_x + normal_y * emin_y + normal_z * emax_z;
      dist2 = normal_x * emin_x + normal_y * emax_y + normal_z * emin_z;
      break;
    case 3:
      dist1 = normal_x * emin_x + normal_y * emin_y + normal_z * emax_z;
      dist2 = normal_x * emax_x + normal_y * emax_y + normal_z * emin_z;
      break;
    case 4:
      dist1 = normal_x * emax_x + normal_y * emax_y + normal_z * emin_z;
      dist2 = normal_x * emin_x + normal_y * emin_y + normal_z * emax_z;
      break;
    case 5:
      dist1 = normal_x * emin_x + normal_y * emax_y + normal_z * emin_z;
      dist2 = normal_x * emax_x + normal_y * emin_y + normal_z * emax_z;
      break;
    case 6:
      dist1 = normal_x * emax_x + normal_y * emin_y + normal_z * emin_z;
      dist2 = normal_x * emin_x + normal_y * emax_y + normal_z * emax_z;
      break;
    case 7:
      dist1 = normal_x * emin_x + normal_y * emin_y + normal_z * emin_z;
      dist2 = normal_x * emax_x + normal_y * emax_y + normal_z * emax_z;
      break;
    default:
      dist1 = dist2 = 0;  // shut up compiler
      BOPS_Error();
      break;
  }

#if 0
	int		i;
	vec3_t	corners[2];

	for (i=0 ; i<3 ; i++)
	{
		if (plane->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist = DotProduct (plane->normal, corners[0]) - plane->dist;
	dist2 = DotProduct (plane->normal, corners[1]) - plane->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

#endif

  sides = (dist1 >= p_dist);
  if (dist2 < p_dist)
    sides |= 2;

#ifdef PARANOID
  if (sides == 0)
    Sys_Error("BoxOnPlaneSide: sides==0");
#endif

  return sides;
#else
  float dist[2];
  int sides, b, i;
  // byte signbits = p->signbits;

  // fast axial cases
  if (p->type < 3) {
    if (p->dist <= emins[p->type])
      return 1;
    if (p->dist >= emaxs[p->type])
      return 2;
    return 3;
  }

  // general case
  dist[0] = dist[1] = 0;
  if (p->signbits < 8)  // >= 8: default case is original code (dist[0]=dist[1]=0)
  {
    for (i = 0; i < 3; i++) {
      b = (p->signbits >> i) & 1;
      dist[b] += p->normal[i] * emaxs[i];
      dist[!b] += p->normal[i] * emins[i];
    }
  }

  sides = 0;
  if (dist[0] >= p->dist)
    sides = 1;
  if (dist[1] < p->dist)
    sides |= 2;

  return sides;
#endif
}

#endif

void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
  float sr, sp, sy, cr, cp, cy;

#if defined(_arch_dreamcast) && defined(ENABLE_DC_MATH)
  fsincos(angles[YAW], &sy, &cy);
  fsincos(angles[PITCH], &sp, &cp);
  fsincos(angles[ROLL], &sr, &cr);
#else
  float angle;
  angle = angles[YAW] * (M_PI * 2 / 360);
  sy = SIN(angle);
  cy = COS(angle);
  angle = angles[PITCH] * (M_PI * 2 / 360);
  sp = SIN(angle);
  cp = COS(angle);
  angle = angles[ROLL] * (M_PI * 2 / 360);
  sr = SIN(angle);
  cr = COS(angle);
#endif

  forward[0] = cp * cy;
  forward[1] = cp * sy;
  forward[2] = -sp;
  right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
  right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
  right[2] = -1 * sr * cp;
  up[0] = (cr * sp * cy + -sr * -sy);
  up[1] = (cr * sp * sy + -sr * cy);
  up[2] = cr * cp;
}

int VectorCompare(vec3_t v1, vec3_t v2) {
  int i;

  for (i = 0; i < 3; i++)
    if (v1[i] != v2[i])
      return 0;

  return 1;
}

void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc) {
#if defined(_arch_dreamcast) && defined(ENABLE_DC_MATH)
  vecc[0] = MATH_fmac(vecb[0], scale, veca[0]);
  vecc[1] = MATH_fmac(vecb[1], scale, veca[1]);
  vecc[2] = MATH_fmac(vecb[2], scale, veca[2]);
#else
  vecc[0] = veca[0] + scale * vecb[0];
  vecc[1] = veca[1] + scale * vecb[1];
  vecc[2] = veca[2] + scale * vecb[2];
#endif
}


vec_t Length(vec3_t v) {
#if defined(_arch_dreamcast) && defined(ENABLE_DC_MATH)
  return MATH_Fast_Sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
#else
  int i;
  float length;

  length = 0;
  for (i = 0; i < 3; i++)
    length += v[i] * v[i];
  length = SQRT(length);  // FIXME

  return length;
#endif
}

void VectorInverse(vec3_t v) {
  v[0] = -v[0];
  v[1] = -v[1];
  v[2] = -v[2];
}

void VectorScale(vec3_t in, vec_t scale, vec3_t out) {
  out[0] = in[0] * scale;
  out[1] = in[1] * scale;
  out[2] = in[2] * scale;
}

int Q_log2(int val) {
  int answer = 0;
  while (val >>= 1)
    answer++;
  return answer;
}

/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]) {
  out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
              in1[0][2] * in2[2][0];
  out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
              in1[0][2] * in2[2][1];
  out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
              in1[0][2] * in2[2][2];
  out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
              in1[1][2] * in2[2][0];
  out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
              in1[1][2] * in2[2][1];
  out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
              in1[1][2] * in2[2][2];
  out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
              in1[2][2] * in2[2][0];
  out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
              in1[2][2] * in2[2][1];
  out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
              in1[2][2] * in2[2][2];
}

/*
================
R_ConcatTransforms
================
*/
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]) {
  out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
              in1[0][2] * in2[2][0];
  out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
              in1[0][2] * in2[2][1];
  out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
              in1[0][2] * in2[2][2];
  out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
              in1[0][2] * in2[2][3] + in1[0][3];
  out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
              in1[1][2] * in2[2][0];
  out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
              in1[1][2] * in2[2][1];
  out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
              in1[1][2] * in2[2][2];
  out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
              in1[1][2] * in2[2][3] + in1[1][3];
  out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
              in1[2][2] * in2[2][0];
  out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
              in1[2][2] * in2[2][1];
  out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
              in1[2][2] * in2[2][2];
  out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
              in1[2][2] * in2[2][3] + in1[2][3];
}


/*
===================
GreatestCommonDivisor
====================
*/
int GreatestCommonDivisor(int i1, int i2) {
  if (i1 > i2) {
    if (i2 == 0)
      return (i1);
    return GreatestCommonDivisor(i2, i1 % i2);
  } else {
    if (i1 == 0)
      return (i2);
    return GreatestCommonDivisor(i1, i2 % i1);
  }
}

