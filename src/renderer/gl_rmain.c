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
// r_main.c

#include "quakedef.h"

#include "glquake.h"
#include "gl_local.h"
#ifdef __SSE__
#include <xmmintrin.h>
#endif
#include <cglm/cglm.h>

entity_t r_worldentity;

qboolean r_cache_thrash;  // compatability

vec3_t modelorg, r_entorigin;
entity_t *currententity;

int r_visframecount;  // bumped when going to a new PVS
int r_framecount;     // used for dlight push checking

mplane_t frustum[4];

int c_brush_polys, c_alias_polys;

qboolean envmap;  // true during envmap command capture

unsigned int currenttexture = -1;  // to avoid unnecessary texture sets

unsigned int cnttextures[2] = {-1, -1};  // cached

unsigned int particletexture;  // little dot for particles
unsigned int playertextures[16];   // up to 16 color translated skins

unsigned int mirrortexturenum;  // quake texturenum, not gltexturenum
qboolean mirror;
mplane_t *mirror_plane;

//
// view origin
//
vec3_t vup;
vec3_t vpn;
vec3_t vright;
vec3_t r_origin;

float r_world_matrix[16];
float r_base_world_matrix[16];

//
// screen size info
//
refdef_t r_refdef;

mleaf_t *r_viewleaf, *r_oldviewleaf;

texture_t *r_notexture_mip;

int d_lightstylevalue[256];  // 8.8 fraction of base light value

void R_MarkLeaves(void);

cvar_t r_norefresh = {"r_norefresh", "0"};
cvar_t r_drawentities = {"r_drawentities", "1"};
cvar_t r_drawviewmodel = {"r_drawviewmodel", "1"};
cvar_t r_speeds = {"r_speeds", "0"};
cvar_t r_fullbright = {"r_fullbright", "0"};
cvar_t r_lightmap = {"r_lightmap", "0"};
cvar_t r_shadows = {"r_shadows", "0"};
cvar_t r_mirroralpha = {"r_mirroralpha", "1"};
cvar_t r_wateralpha = {"r_wateralpha", "1"};
cvar_t r_dynamic = {"r_dynamic", "1"}; //@Note: change back, WIN98
cvar_t r_novis = {"r_novis", "0"};
cvar_t r_interpolate_anim = {"r_interpolate_anim", "1", true};  // fenix@io.com: model interpolation
cvar_t r_interpolate_pos = {"r_interpolate_pos", "1", true};    // " "

cvar_t gl_finish = {"gl_finish", "0"};
cvar_t gl_clear = {"gl_clear", "0"};
cvar_t gl_cull = {"gl_cull", "1"};
cvar_t gl_smoothmodels = {"gl_smoothmodels", "1"};
cvar_t gl_affinemodels = {"gl_affinemodels", "1"};
cvar_t gl_polyblend = {"gl_polyblend", "1"};
cvar_t gl_flashblend = {"gl_flashblend", "1"};
cvar_t gl_playermip = {"gl_playermip", "0"};
cvar_t gl_nocolors = {"gl_nocolors", "0"};
cvar_t gl_keeptjunctions = {"gl_keeptjunctions", "0"};
cvar_t gl_reporttjunctions = {"gl_reporttjunctions", "0"};
cvar_t gl_doubleeyes = {"gl_doubleeys", "1"};
cvar_t r_sky = {"r_sky", "1"};

extern cvar_t gl_ztrick;

static int _backup_min_filter;
static int _backup_max_filter;

void R_DrawSkySphere(void);
void R_DrawParticles(void);
void R_RenderBrushPoly(msurface_t *fa);
static mat4 _model_matrix __attribute__((aligned(32))) =
    {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
};
/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox(const vec3_t mins, const vec3_t maxs) {
  int i;

  for (i = 0; i < 4; i++)
    if (BoxOnPlaneSide(mins, maxs, &frustum[i]) == 2)
      return true;
  return false;
}

void R_RotateForEntity(entity_t *e) {
  glGetFloatv(GL_MODELVIEW_MATRIX, (float *)_model_matrix);

  glm_translate(_model_matrix, (float *)e->origin);

  glm_rotate_z(_model_matrix, DEG2RAD(e->angles[1]), _model_matrix);
  glm_rotate_y(_model_matrix, DEG2RAD(-e->angles[0]), _model_matrix);
  glm_rotate_x(_model_matrix, DEG2RAD(e->angles[2]), _model_matrix);

  glLoadMatrixf((float *)_model_matrix);
}

/*
=============
R_BlendedRotateForEntity

fenix@io.com: model transform interpolation
=============
*/
void R_BlendedRotateForEntity(entity_t *e) {
  float timepassed;
  float blend;
  vec3_t d;
  int i;

  glGetFloatv(GL_MODELVIEW_MATRIX, (float *)_model_matrix);

  // positional interpolation

  timepassed = realtime - e->translate_start_time;

  if (e->translate_start_time == 0 || timepassed > 1) {
    e->translate_start_time = realtime;
    VectorCopy(e->origin, e->origin1);
    VectorCopy(e->origin, e->origin2);
  }

  if (!VectorCompare(e->origin, e->origin2)) {
    e->translate_start_time = realtime;
    VectorCopy(e->origin2, e->origin1);
    VectorCopy(e->origin, e->origin2);
    blend = 0;
  } else {
    blend = timepassed * 10.0f;

    if (cl.paused || blend > 1) blend = 1;
  }

  VectorSubtract(e->origin2, e->origin1, d);

  glm_translate(_model_matrix, (vec3){e->origin1[0] + (blend * d[0]), e->origin1[1] + (blend * d[1]), e->origin1[2] + (blend * d[2])});

  // orientation interpolation (Euler angles, yuck!)
  timepassed = realtime - e->rotate_start_time;

  if (e->rotate_start_time == 0 || timepassed > 1) {
    e->rotate_start_time = realtime;
    VectorCopy(e->angles, e->angles1);
    VectorCopy(e->angles, e->angles2);
  }

  if (!VectorCompare(e->angles, e->angles2)) {
    e->rotate_start_time = realtime;
    VectorCopy(e->angles2, e->angles1);
    VectorCopy(e->angles, e->angles2);
    blend = 0;
  } else {
    blend = timepassed * 10.0f;

    if (cl.paused || blend > 1) blend = 1;
  }

  VectorSubtract(e->angles2, e->angles1, d);

  // always interpolate along the shortest path
  for (i = 0; i < 3; i++) {
    if (d[i] > 180) {
      d[i] -= 360;
    } else if (d[i] < -180) {
      d[i] += 360;
    }
  }

  glm_rotate_z(_model_matrix, DEG2RAD(e->angles1[1] + (blend * d[1])), _model_matrix);
  glm_rotate_y(_model_matrix, DEG2RAD(-e->angles1[0] + (-blend * d[0])), _model_matrix);
  glm_rotate_x(_model_matrix, DEG2RAD(e->angles1[2] + (blend * d[2])), _model_matrix);

  glLoadMatrixf((float *)_model_matrix);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame(entity_t *currententity) {
  msprite_t *psprite;
  mspritegroup_t *pspritegroup;
  mspriteframe_t *pspriteframe;
  int i, numframes, frame;
  float *pintervals, fullinterval, targettime, time;

  psprite = currententity->model->cache.data;
  frame = currententity->frame;

  if ((frame >= psprite->numframes) || (frame < 0)) {
    Con_Printf("R_DrawSprite: no such frame %d\n", frame);
    frame = 0;
  }

  if (psprite->frames[frame].type == SPR_SINGLE) {
    pspriteframe = psprite->frames[frame].frameptr;
  } else {
    pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
    pintervals = pspritegroup->intervals;
    numframes = pspritegroup->numframes;
    fullinterval = pintervals[numframes - 1];

    time = cl.time + currententity->syncbase;

    // when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
    // are positive, so we don't have to worry about division by 0
    targettime = time - ((int)(time / fullinterval)) * fullinterval;

    for (i = 0; i < (numframes - 1); i++) {
      if (pintervals[i] > targettime)
        break;
    }

    pspriteframe = pspritegroup->frames[i];
  }

  return pspriteframe;
}

/*
=================
R_DrawSpriteModel

=================
*/
//Atomizer - New math function

#define VectorScalarMult(a, b, c) \
  {                               \
    c[0] = a[0] * b;              \
    c[1] = a[1] * b;              \
    c[2] = a[2] * b;              \
  }

void R_DrawSpriteModel(entity_t *e) {
  vec3_t point;
  mspriteframe_t *frame;
  float *up, *right;
  vec3_t v_forward, v_right, v_up;
  msprite_t *psprite;

  // don't even bother culling, because it's just a single
  // polygon without a surface cache
  frame = R_GetSpriteFrame(e);
  psprite = currententity->model->cache.data;

  if (psprite->type == SPR_ORIENTED) {  // bullet marks on walls
    AngleVectors(currententity->angles, v_forward, v_right, v_up);
    up = v_up;
    right = v_right;
  } else {  // normal sprite
    up = vup;
    right = vright;
  }

  GL_DisableMultitexture();

  GL_Bind(frame->gl_texturenum);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  //@Note: Check but i think this is better
  //glEnable (GL_ALPHA_TEST);
  glEnable(GL_BLEND);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  point[0] = (e->origin[0] + frame->down * up[0]) + frame->right * right[0];
  point[1] = (e->origin[1] + frame->down * up[1]) + frame->right * right[1];
  point[2] = (e->origin[2] + frame->down * up[2]) + frame->right * right[2];
  r_batchedtempverts[0] = (glvert_fast_t){.flags = VERTEX, .vert = {point[0], point[1], point[2]}, .texture = {1, 1}, VTX_COLOR_WHITE, .pad0 = {0}};

  point[0] = (e->origin[0] + frame->down * up[0]) + frame->left * right[0];
  point[1] = (e->origin[1] + frame->down * up[1]) + frame->left * right[1];
  point[2] = (e->origin[2] + frame->down * up[2]) + frame->left * right[2];
  r_batchedtempverts[1] = (glvert_fast_t){.flags = VERTEX, .vert = {point[0], point[1], point[2]}, .texture = {0, 1}, VTX_COLOR_WHITE, .pad0 = {0}};

  point[0] = (e->origin[0] + frame->up * up[0]) + frame->right * right[0];
  point[1] = (e->origin[1] + frame->up * up[1]) + frame->right * right[1];
  point[2] = (e->origin[2] + frame->up * up[2]) + frame->right * right[2];
  r_batchedtempverts[2] = (glvert_fast_t){.flags = VERTEX, .vert = {point[0], point[1], point[2]}, .texture = {1, 0}, VTX_COLOR_WHITE, .pad0 = {0}};

  point[0] = (e->origin[0] + frame->up * up[0]) + frame->left * right[0];
  point[1] = (e->origin[1] + frame->up * up[1]) + frame->left * right[1];
  point[2] = (e->origin[2] + frame->up * up[2]) + frame->left * right[2];
  r_batchedtempverts[3] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {point[0], point[1], point[2]}, .texture = {0, 0}, VTX_COLOR_WHITE, .pad0 = {0}};

  glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &r_batchedtempverts[0].vert);
  glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &r_batchedtempverts[0].texture);
#ifdef WIN98
  glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &r_batchedtempverts[0].color);
#else
  glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &r_batchedtempverts[0].color);
#endif

  glDepthMask(GL_FALSE);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDepthMask(GL_TRUE);
}

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

#define NUMVERTEXNORMALS 162

float r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

vec3_t shadevector;
float shadelight, ambientlight;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float r_avertexnormal_dots[16][256] =
{
{1.23,1.30,1.47,1.35,1.56,1.71,1.37,1.38,1.59,1.60,1.79,1.97,1.88,1.92,1.79,1.02,0.93,1.07,0.82,0.87,0.88,0.94,0.96,1.14,1.11,0.82,0.83,0.89,0.89,0.86,0.94,0.91,1.00,1.21,0.98,1.48,1.30,1.57,0.96,1.07,1.14,1.60,1.61,1.40,1.37,1.72,1.78,1.79,1.93,1.99,1.90,1.68,1.71,1.86,1.60,1.68,1.78,1.86,1.93,1.99,1.97,1.44,1.22,1.49,0.93,0.99,0.99,1.23,1.22,1.44,1.49,0.89,0.89,0.97,0.91,0.98,1.19,0.82,0.76,0.82,0.71,0.72,0.73,0.76,0.79,0.86,0.83,0.72,0.76,0.76,0.89,0.82,0.89,0.82,0.89,0.91,0.83,0.96,1.14,0.97,1.40,1.19,0.98,0.94,1.00,1.07,1.37,1.21,1.48,1.30,1.57,1.61,1.37,0.86,0.83,0.91,0.82,0.82,0.88,0.89,0.96,1.14,0.98,0.87,0.93,0.94,1.02,1.30,1.07,1.35,1.38,1.11,1.56,1.92,1.79,1.79,1.59,1.60,1.72,1.90,1.79,0.80,0.85,0.79,0.93,0.80,0.85,0.77,0.74,0.72,0.77,0.74,0.72,0.70,0.70,0.71,0.76,0.73,0.79,0.79,0.73,0.76,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.26,1.26,1.48,1.23,1.50,1.71,1.14,1.19,1.38,1.46,1.64,1.94,1.87,1.84,1.71,1.02,0.92,1.00,0.79,0.85,0.84,0.91,0.90,0.98,0.99,0.77,0.77,0.83,0.82,0.79,0.86,0.84,0.92,0.99,0.91,1.24,1.03,1.33,0.88,0.94,0.97,1.41,1.39,1.18,1.11,1.51,1.61,1.59,1.80,1.91,1.76,1.54,1.65,1.76,1.70,1.70,1.85,1.85,1.97,1.99,1.93,1.28,1.09,1.39,0.92,0.97,0.99,1.18,1.26,1.52,1.48,0.83,0.85,0.90,0.88,0.93,1.00,0.77,0.73,0.78,0.72,0.71,0.74,0.75,0.79,0.86,0.81,0.75,0.81,0.79,0.96,0.88,0.94,0.86,0.93,0.92,0.85,1.08,1.33,1.05,1.55,1.31,1.01,1.05,1.27,1.31,1.60,1.47,1.70,1.54,1.76,1.76,1.57,0.93,0.90,0.99,0.88,0.88,0.95,0.97,1.11,1.39,1.20,0.92,0.97,1.01,1.10,1.39,1.22,1.51,1.58,1.32,1.64,1.97,1.85,1.91,1.77,1.74,1.88,1.99,1.91,0.79,0.86,0.80,0.94,0.84,0.88,0.74,0.74,0.71,0.82,0.77,0.76,0.70,0.73,0.72,0.73,0.70,0.74,0.85,0.77,0.82,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.34,1.27,1.53,1.17,1.46,1.71,0.98,1.05,1.20,1.34,1.48,1.86,1.82,1.71,1.62,1.09,0.94,0.99,0.79,0.85,0.82,0.90,0.87,0.93,0.96,0.76,0.74,0.79,0.76,0.74,0.79,0.78,0.85,0.92,0.85,1.00,0.93,1.06,0.81,0.86,0.89,1.16,1.12,0.97,0.95,1.28,1.38,1.35,1.60,1.77,1.57,1.33,1.50,1.58,1.69,1.63,1.82,1.74,1.91,1.92,1.80,1.04,0.97,1.21,0.90,0.93,0.97,1.05,1.21,1.48,1.37,0.77,0.80,0.84,0.85,0.88,0.92,0.73,0.71,0.74,0.74,0.71,0.75,0.73,0.79,0.84,0.78,0.79,0.86,0.81,1.05,0.94,0.99,0.90,0.95,0.92,0.86,1.24,1.44,1.14,1.59,1.34,1.02,1.27,1.50,1.49,1.80,1.69,1.86,1.72,1.87,1.80,1.69,1.00,0.98,1.23,0.95,0.96,1.09,1.16,1.37,1.63,1.46,0.99,1.10,1.25,1.24,1.51,1.41,1.67,1.77,1.55,1.72,1.95,1.89,1.98,1.91,1.86,1.97,1.99,1.94,0.81,0.89,0.85,0.98,0.90,0.94,0.75,0.78,0.73,0.89,0.83,0.82,0.72,0.77,0.76,0.72,0.70,0.71,0.91,0.83,0.89,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.46,1.34,1.60,1.16,1.46,1.71,0.94,0.99,1.05,1.26,1.33,1.74,1.76,1.57,1.54,1.23,0.98,1.05,0.83,0.89,0.84,0.92,0.87,0.91,0.96,0.78,0.74,0.79,0.72,0.72,0.75,0.76,0.80,0.88,0.83,0.94,0.87,0.95,0.76,0.80,0.82,0.97,0.96,0.89,0.88,1.08,1.11,1.10,1.37,1.59,1.37,1.07,1.27,1.34,1.57,1.45,1.69,1.55,1.77,1.79,1.60,0.93,0.90,0.99,0.86,0.87,0.93,0.96,1.07,1.35,1.18,0.73,0.76,0.77,0.81,0.82,0.85,0.70,0.71,0.72,0.78,0.73,0.77,0.73,0.79,0.82,0.76,0.83,0.90,0.84,1.18,0.98,1.03,0.92,0.95,0.90,0.86,1.32,1.45,1.15,1.53,1.27,0.99,1.42,1.65,1.58,1.93,1.83,1.94,1.81,1.88,1.74,1.70,1.19,1.17,1.44,1.11,1.15,1.36,1.41,1.61,1.81,1.67,1.22,1.34,1.50,1.42,1.65,1.61,1.82,1.91,1.75,1.80,1.89,1.89,1.98,1.99,1.94,1.98,1.92,1.87,0.86,0.95,0.92,1.14,0.98,1.03,0.79,0.84,0.77,0.97,0.90,0.89,0.76,0.82,0.82,0.74,0.72,0.71,0.98,0.89,0.97,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.60,1.44,1.68,1.22,1.49,1.71,0.93,0.99,0.99,1.23,1.22,1.60,1.68,1.44,1.49,1.40,1.14,1.19,0.89,0.96,0.89,0.97,0.89,0.91,0.98,0.82,0.76,0.82,0.71,0.72,0.73,0.76,0.79,0.86,0.83,0.91,0.83,0.89,0.72,0.76,0.76,0.89,0.89,0.82,0.82,0.98,0.96,0.97,1.14,1.40,1.19,0.94,1.00,1.07,1.37,1.21,1.48,1.30,1.57,1.61,1.37,0.86,0.83,0.91,0.82,0.82,0.88,0.89,0.96,1.14,0.98,0.70,0.72,0.73,0.77,0.76,0.79,0.70,0.72,0.71,0.82,0.77,0.80,0.74,0.79,0.80,0.74,0.87,0.93,0.85,1.23,1.02,1.02,0.93,0.93,0.87,0.85,1.30,1.35,1.07,1.38,1.11,0.94,1.47,1.71,1.56,1.97,1.88,1.92,1.79,1.79,1.59,1.60,1.30,1.35,1.56,1.37,1.38,1.59,1.60,1.79,1.92,1.79,1.48,1.57,1.72,1.61,1.78,1.79,1.93,1.99,1.90,1.86,1.78,1.86,1.93,1.99,1.97,1.90,1.79,1.72,0.94,1.07,1.00,1.37,1.21,1.30,0.86,0.91,0.83,1.14,0.98,0.96,0.82,0.88,0.89,0.79,0.76,0.73,1.07,0.94,1.11,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.74,1.57,1.76,1.33,1.54,1.71,0.94,1.05,0.99,1.26,1.16,1.46,1.60,1.34,1.46,1.59,1.37,1.37,0.97,1.11,0.96,1.10,0.95,0.94,1.08,0.89,0.82,0.88,0.72,0.76,0.75,0.80,0.80,0.88,0.87,0.91,0.83,0.87,0.72,0.76,0.74,0.83,0.84,0.78,0.79,0.96,0.89,0.92,0.98,1.23,1.05,0.86,0.92,0.95,1.11,0.98,1.22,1.03,1.34,1.42,1.14,0.79,0.77,0.84,0.78,0.76,0.82,0.82,0.89,0.97,0.90,0.70,0.71,0.71,0.73,0.72,0.74,0.73,0.76,0.72,0.86,0.81,0.82,0.76,0.79,0.77,0.73,0.90,0.95,0.86,1.18,1.03,0.98,0.92,0.90,0.83,0.84,1.19,1.17,0.98,1.15,0.97,0.89,1.42,1.65,1.44,1.93,1.83,1.81,1.67,1.61,1.36,1.41,1.32,1.45,1.58,1.57,1.53,1.74,1.70,1.88,1.94,1.81,1.69,1.77,1.87,1.79,1.89,1.92,1.98,1.99,1.98,1.89,1.65,1.80,1.82,1.91,1.94,1.75,1.61,1.50,1.07,1.34,1.27,1.60,1.45,1.55,0.93,0.99,0.90,1.35,1.18,1.07,0.87,0.93,0.96,0.85,0.82,0.77,1.15,0.99,1.27,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.86,1.71,1.82,1.48,1.62,1.71,0.98,1.20,1.05,1.34,1.17,1.34,1.53,1.27,1.46,1.77,1.60,1.57,1.16,1.38,1.12,1.35,1.06,1.00,1.28,0.97,0.89,0.95,0.76,0.81,0.79,0.86,0.85,0.92,0.93,0.93,0.85,0.87,0.74,0.78,0.74,0.79,0.82,0.76,0.79,0.96,0.85,0.90,0.94,1.09,0.99,0.81,0.85,0.89,0.95,0.90,0.99,0.94,1.10,1.24,0.98,0.75,0.73,0.78,0.74,0.72,0.77,0.76,0.82,0.89,0.83,0.73,0.71,0.71,0.71,0.70,0.72,0.77,0.80,0.74,0.90,0.85,0.84,0.78,0.79,0.75,0.73,0.92,0.95,0.86,1.05,0.99,0.94,0.90,0.86,0.79,0.81,1.00,0.98,0.91,0.96,0.89,0.83,1.27,1.50,1.23,1.80,1.69,1.63,1.46,1.37,1.09,1.16,1.24,1.44,1.49,1.69,1.59,1.80,1.69,1.87,1.86,1.72,1.82,1.91,1.94,1.92,1.95,1.99,1.98,1.91,1.97,1.89,1.51,1.72,1.67,1.77,1.86,1.55,1.41,1.25,1.33,1.58,1.50,1.80,1.63,1.74,1.04,1.21,0.97,1.48,1.37,1.21,0.93,0.97,1.05,0.92,0.88,0.84,1.14,1.02,1.34,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.94,1.84,1.87,1.64,1.71,1.71,1.14,1.38,1.19,1.46,1.23,1.26,1.48,1.26,1.50,1.91,1.80,1.76,1.41,1.61,1.39,1.59,1.33,1.24,1.51,1.18,0.97,1.11,0.82,0.88,0.86,0.94,0.92,0.99,1.03,0.98,0.91,0.90,0.79,0.84,0.77,0.79,0.84,0.77,0.83,0.99,0.85,0.91,0.92,1.02,1.00,0.79,0.80,0.86,0.88,0.84,0.92,0.88,0.97,1.10,0.94,0.74,0.71,0.74,0.72,0.70,0.73,0.72,0.76,0.82,0.77,0.77,0.73,0.74,0.71,0.70,0.73,0.83,0.85,0.78,0.92,0.88,0.86,0.81,0.79,0.74,0.75,0.92,0.93,0.85,0.96,0.94,0.88,0.86,0.81,0.75,0.79,0.93,0.90,0.85,0.88,0.82,0.77,1.05,1.27,0.99,1.60,1.47,1.39,1.20,1.11,0.95,0.97,1.08,1.33,1.31,1.70,1.55,1.76,1.57,1.76,1.70,1.54,1.85,1.97,1.91,1.99,1.97,1.99,1.91,1.77,1.88,1.85,1.39,1.64,1.51,1.58,1.74,1.32,1.22,1.01,1.54,1.76,1.65,1.93,1.70,1.85,1.28,1.39,1.09,1.52,1.48,1.26,0.97,0.99,1.18,1.00,0.93,0.90,1.05,1.01,1.31,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.97,1.92,1.88,1.79,1.79,1.71,1.37,1.59,1.38,1.60,1.35,1.23,1.47,1.30,1.56,1.99,1.93,1.90,1.60,1.78,1.61,1.79,1.57,1.48,1.72,1.40,1.14,1.37,0.89,0.96,0.94,1.07,1.00,1.21,1.30,1.14,0.98,0.96,0.86,0.91,0.83,0.82,0.88,0.82,0.89,1.11,0.87,0.94,0.93,1.02,1.07,0.80,0.79,0.85,0.82,0.80,0.87,0.85,0.93,1.02,0.93,0.77,0.72,0.74,0.71,0.70,0.70,0.71,0.72,0.77,0.74,0.82,0.76,0.79,0.72,0.73,0.76,0.89,0.89,0.82,0.93,0.91,0.86,0.83,0.79,0.73,0.76,0.91,0.89,0.83,0.89,0.89,0.82,0.82,0.76,0.72,0.76,0.86,0.83,0.79,0.82,0.76,0.73,0.94,1.00,0.91,1.37,1.21,1.14,0.98,0.96,0.88,0.89,0.96,1.14,1.07,1.60,1.40,1.61,1.37,1.57,1.48,1.30,1.78,1.93,1.79,1.99,1.92,1.90,1.79,1.59,1.72,1.79,1.30,1.56,1.35,1.38,1.60,1.11,1.07,0.94,1.68,1.86,1.71,1.97,1.68,1.86,1.44,1.49,1.22,1.44,1.49,1.22,0.99,0.99,1.23,1.19,0.98,0.97,0.97,0.98,1.19,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.94,1.97,1.87,1.91,1.85,1.71,1.60,1.77,1.58,1.74,1.51,1.26,1.48,1.39,1.64,1.99,1.97,1.99,1.70,1.85,1.76,1.91,1.76,1.70,1.88,1.55,1.33,1.57,0.96,1.08,1.05,1.31,1.27,1.47,1.54,1.39,1.20,1.11,0.93,0.99,0.90,0.88,0.95,0.88,0.97,1.32,0.92,1.01,0.97,1.10,1.22,0.84,0.80,0.88,0.79,0.79,0.85,0.86,0.92,1.02,0.94,0.82,0.76,0.77,0.72,0.73,0.70,0.72,0.71,0.74,0.74,0.88,0.81,0.85,0.75,0.77,0.82,0.94,0.93,0.86,0.92,0.92,0.86,0.85,0.79,0.74,0.79,0.88,0.85,0.81,0.82,0.83,0.77,0.78,0.73,0.71,0.75,0.79,0.77,0.74,0.77,0.73,0.70,0.86,0.92,0.84,1.14,0.99,0.98,0.91,0.90,0.84,0.83,0.88,0.97,0.94,1.41,1.18,1.39,1.11,1.33,1.24,1.03,1.61,1.80,1.59,1.91,1.84,1.76,1.64,1.38,1.51,1.71,1.26,1.50,1.23,1.19,1.46,0.99,1.00,0.91,1.70,1.85,1.65,1.93,1.54,1.76,1.52,1.48,1.26,1.28,1.39,1.09,0.99,0.97,1.18,1.31,1.01,1.05,0.90,0.93,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.86,1.95,1.82,1.98,1.89,1.71,1.80,1.91,1.77,1.86,1.67,1.34,1.53,1.51,1.72,1.92,1.91,1.99,1.69,1.82,1.80,1.94,1.87,1.86,1.97,1.59,1.44,1.69,1.05,1.24,1.27,1.49,1.50,1.69,1.72,1.63,1.46,1.37,1.00,1.23,0.98,0.95,1.09,0.96,1.16,1.55,0.99,1.25,1.10,1.24,1.41,0.90,0.85,0.94,0.79,0.81,0.85,0.89,0.94,1.09,0.98,0.89,0.82,0.83,0.74,0.77,0.72,0.76,0.73,0.75,0.78,0.94,0.86,0.91,0.79,0.83,0.89,0.99,0.95,0.90,0.90,0.92,0.84,0.86,0.79,0.75,0.81,0.85,0.80,0.78,0.76,0.77,0.73,0.74,0.71,0.71,0.73,0.74,0.74,0.71,0.76,0.72,0.70,0.79,0.85,0.78,0.98,0.92,0.93,0.85,0.87,0.82,0.79,0.81,0.89,0.86,1.16,0.97,1.12,0.95,1.06,1.00,0.93,1.38,1.60,1.35,1.77,1.71,1.57,1.48,1.20,1.28,1.62,1.27,1.46,1.17,1.05,1.34,0.96,0.99,0.90,1.63,1.74,1.50,1.80,1.33,1.58,1.48,1.37,1.21,1.04,1.21,0.97,0.97,0.93,1.05,1.34,1.02,1.14,0.84,0.88,0.92,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.74,1.89,1.76,1.98,1.89,1.71,1.93,1.99,1.91,1.94,1.82,1.46,1.60,1.65,1.80,1.79,1.77,1.92,1.57,1.69,1.74,1.87,1.88,1.94,1.98,1.53,1.45,1.70,1.18,1.32,1.42,1.58,1.65,1.83,1.81,1.81,1.67,1.61,1.19,1.44,1.17,1.11,1.36,1.15,1.41,1.75,1.22,1.50,1.34,1.42,1.61,0.98,0.92,1.03,0.83,0.86,0.89,0.95,0.98,1.23,1.14,0.97,0.89,0.90,0.78,0.82,0.76,0.82,0.77,0.79,0.84,0.98,0.90,0.98,0.83,0.89,0.97,1.03,0.95,0.92,0.86,0.90,0.82,0.86,0.79,0.77,0.84,0.81,0.76,0.76,0.72,0.73,0.70,0.72,0.71,0.73,0.73,0.72,0.74,0.71,0.78,0.74,0.72,0.75,0.80,0.76,0.94,0.88,0.91,0.83,0.87,0.84,0.79,0.76,0.82,0.80,0.97,0.89,0.96,0.88,0.95,0.94,0.87,1.11,1.37,1.10,1.59,1.57,1.37,1.33,1.05,1.08,1.54,1.34,1.46,1.16,0.99,1.26,0.96,1.05,0.92,1.45,1.55,1.27,1.60,1.07,1.34,1.35,1.18,1.07,0.93,0.99,0.90,0.93,0.87,0.96,1.27,0.99,1.15,0.77,0.82,0.85,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.60,1.78,1.68,1.93,1.86,1.71,1.97,1.99,1.99,1.97,1.93,1.60,1.68,1.78,1.86,1.61,1.57,1.79,1.37,1.48,1.59,1.72,1.79,1.92,1.90,1.38,1.35,1.60,1.23,1.30,1.47,1.56,1.71,1.88,1.79,1.92,1.79,1.79,1.30,1.56,1.35,1.37,1.59,1.38,1.60,1.90,1.48,1.72,1.57,1.61,1.79,1.21,1.00,1.30,0.89,0.94,0.96,1.07,1.14,1.40,1.37,1.14,0.96,0.98,0.82,0.88,0.82,0.89,0.83,0.86,0.91,1.02,0.93,1.07,0.87,0.94,1.11,1.02,0.93,0.93,0.82,0.87,0.80,0.85,0.79,0.80,0.85,0.77,0.72,0.74,0.71,0.70,0.70,0.71,0.72,0.77,0.74,0.72,0.76,0.73,0.82,0.79,0.76,0.73,0.79,0.76,0.93,0.86,0.91,0.83,0.89,0.89,0.82,0.72,0.76,0.76,0.89,0.82,0.89,0.82,0.89,0.91,0.83,0.96,1.14,0.97,1.40,1.44,1.19,1.22,0.99,0.98,1.49,1.44,1.49,1.22,0.99,1.23,0.98,1.19,0.97,1.21,1.30,1.00,1.37,0.94,1.07,1.14,0.98,0.96,0.86,0.91,0.83,0.88,0.82,0.89,1.11,0.94,1.07,0.73,0.76,0.79,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.46,1.65,1.60,1.82,1.80,1.71,1.93,1.91,1.99,1.94,1.98,1.74,1.76,1.89,1.89,1.42,1.34,1.61,1.11,1.22,1.36,1.50,1.61,1.81,1.75,1.15,1.17,1.41,1.18,1.19,1.42,1.44,1.65,1.83,1.67,1.94,1.81,1.88,1.32,1.58,1.45,1.57,1.74,1.53,1.70,1.98,1.69,1.87,1.77,1.79,1.92,1.45,1.27,1.55,0.97,1.07,1.11,1.34,1.37,1.59,1.60,1.35,1.07,1.18,0.86,0.93,0.87,0.96,0.90,0.93,0.99,1.03,0.95,1.15,0.90,0.99,1.27,0.98,0.90,0.92,0.78,0.83,0.77,0.84,0.79,0.82,0.86,0.73,0.71,0.73,0.72,0.70,0.73,0.72,0.76,0.81,0.76,0.76,0.82,0.77,0.89,0.85,0.82,0.75,0.80,0.80,0.94,0.88,0.94,0.87,0.95,0.96,0.88,0.72,0.74,0.76,0.83,0.78,0.84,0.79,0.87,0.91,0.83,0.89,0.98,0.92,1.23,1.34,1.05,1.16,0.99,0.96,1.46,1.57,1.54,1.33,1.05,1.26,1.08,1.37,1.10,0.98,1.03,0.92,1.14,0.86,0.95,0.97,0.90,0.89,0.79,0.84,0.77,0.82,0.76,0.82,0.97,0.89,0.98,0.71,0.72,0.74,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.34,1.51,1.53,1.67,1.72,1.71,1.80,1.77,1.91,1.86,1.98,1.86,1.82,1.95,1.89,1.24,1.10,1.41,0.95,0.99,1.09,1.25,1.37,1.63,1.55,0.96,0.98,1.16,1.05,1.00,1.27,1.23,1.50,1.69,1.46,1.86,1.72,1.87,1.24,1.49,1.44,1.69,1.80,1.59,1.69,1.97,1.82,1.94,1.91,1.92,1.99,1.63,1.50,1.74,1.16,1.33,1.38,1.58,1.60,1.77,1.80,1.48,1.21,1.37,0.90,0.97,0.93,1.05,0.97,1.04,1.21,0.99,0.95,1.14,0.92,1.02,1.34,0.94,0.86,0.90,0.74,0.79,0.75,0.81,0.79,0.84,0.86,0.71,0.71,0.73,0.76,0.73,0.77,0.74,0.80,0.85,0.78,0.81,0.89,0.84,0.97,0.92,0.88,0.79,0.85,0.86,0.98,0.92,1.00,0.93,1.06,1.12,0.95,0.74,0.74,0.78,0.79,0.76,0.82,0.79,0.87,0.93,0.85,0.85,0.94,0.90,1.09,1.27,0.99,1.17,1.05,0.96,1.46,1.71,1.62,1.48,1.20,1.34,1.28,1.57,1.35,0.90,0.94,0.85,0.98,0.81,0.89,0.89,0.83,0.82,0.75,0.78,0.73,0.77,0.72,0.76,0.89,0.83,0.91,0.71,0.70,0.72,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00},
{1.26,1.39,1.48,1.51,1.64,1.71,1.60,1.58,1.77,1.74,1.91,1.94,1.87,1.97,1.85,1.10,0.97,1.22,0.88,0.92,0.95,1.01,1.11,1.39,1.32,0.88,0.90,0.97,0.96,0.93,1.05,0.99,1.27,1.47,1.20,1.70,1.54,1.76,1.08,1.31,1.33,1.70,1.76,1.55,1.57,1.88,1.85,1.91,1.97,1.99,1.99,1.70,1.65,1.85,1.41,1.54,1.61,1.76,1.80,1.91,1.93,1.52,1.26,1.48,0.92,0.99,0.97,1.18,1.09,1.28,1.39,0.94,0.93,1.05,0.92,1.01,1.31,0.88,0.81,0.86,0.72,0.75,0.74,0.79,0.79,0.86,0.85,0.71,0.73,0.75,0.82,0.77,0.83,0.78,0.85,0.88,0.81,0.88,0.97,0.90,1.18,1.00,0.93,0.86,0.92,0.94,1.14,0.99,1.24,1.03,1.33,1.39,1.11,0.79,0.77,0.84,0.79,0.77,0.84,0.83,0.90,0.98,0.91,0.85,0.92,0.91,1.02,1.26,1.00,1.23,1.19,0.99,1.50,1.84,1.71,1.64,1.38,1.46,1.51,1.76,1.59,0.84,0.88,0.80,0.94,0.79,0.86,0.82,0.77,0.76,0.74,0.74,0.71,0.73,0.70,0.72,0.82,0.77,0.85,0.74,0.70,0.73,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00}
};

float *shadedots = r_avertexnormal_dots[0];

int lastposenum0;
int lastposenum;

/*
=============
GL_DrawAliasFrame
=============
*/
void GL_DrawAliasFrame(aliashdr_t *paliashdr, int posenum) {
  unsigned char l;
  trivertx_t * __restrict verts;
  int * __restrict order;
  int count;

  glEnableClientState(GL_COLOR_ARRAY);

  lastposenum = posenum;
  verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
  verts += posenum * paliashdr->poseverts;
  order = (int *)((byte *)paliashdr + paliashdr->commands);

  while (1) {
    // get the vertex count and primitive type
    count = *order++;
    if (!count)
      break;  // done

    int c;
    int i = 0;

    glvert_fast_t *submission_pointer = &r_batchedtempverts[0];

#ifdef GL_EXT_dreamcast_direct_buffer
    glEnable(GL_DIRECT_BUFFER_KOS);
    glDirectBufferReserve_INTERNAL_KOS(count, (int *)&submission_pointer, GL_TRIANGLE_STRIP);
#endif

    glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->vert);
    glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->texture);
#ifdef WIN98
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#else
    glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#endif

    c = count;
    byte* restrict vert_x = &verts->v[0];
    byte* restrict vert_y = &verts->v[1];
    byte* restrict vert_z = &verts->v[2];
    byte* restrict vert_light = &verts->lightnormalindex;
    int vert_iter = 0;
    const int vertex_stride = 4;
    float* restrict texture_u = (float*)order;
    float* restrict texture_v = (float*)(order+1);
    int tex_iter = 0;
    const int texture_stride = 2;
    do {
      /* Load */
      const byte vx = vert_x[vert_iter];
      const byte vy = vert_y[vert_iter];
      const byte vz = vert_z[vert_iter];
      const byte vlight = vert_light[vert_iter];

      const float tu = texture_u[tex_iter];
      const float tv = texture_v[tex_iter];

      /* Update */
      const float vxf = (float)vx;
      const float vyf = (float)vy;
      const float vzf = (float)vz;

      /* Store */
      l = ambientlight < 0 ? 255 : (unsigned char)((shadedots[vlight] * shadelight) * 255);

      *submission_pointer++ = (glvert_fast_t){.flags = VERTEX,
                                              .vert = {vxf, vyf, vzf},
                                              .texture = {tu, tv},
                                              .color = {.packed = PACK_BGRA8888(l, l, l, 255)}, .pad0 = {0}};

      tex_iter += texture_stride;
      vert_iter += vertex_stride;
    } while (--c);
    (*(submission_pointer - 1)).flags = VERTEX_EOL;

    order += 2 * count;
    i += count;
    verts += count;

    glDrawArrays(GL_TRIANGLE_STRIP, 0, count);

#ifdef GL_EXT_dreamcast_direct_buffer
    glDisable(GL_DIRECT_BUFFER_KOS);
#endif
    #ifdef PARANOID
    if (count > 32)
      printf("%s:%d drew: %d\n", __FILE__, __LINE__, count);
    #endif
  }

  glDisableClientState(GL_COLOR_ARRAY);
}

/*
 =============
 GL_DrawAliasBlendedFrame

 fenix@io.com: model animation interpolation
 haydenkow: vertex arrays
 =============
 */
void GL_DrawAliasBlendedFrame(aliashdr_t *paliashdr, int pose1, int pose2, float blend) {
  unsigned char l;
  trivertx_t * __restrict verts1;
  trivertx_t * __restrict verts2;
  int * __restrict order;
  int count;

  lastposenum0 = pose1;
  lastposenum = pose2;

  verts1 = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
  verts2 = verts1;

  verts1 += pose1 * paliashdr->poseverts;
  verts2 += pose2 * paliashdr->poseverts;

  order = (int *)((byte *)paliashdr + paliashdr->commands);

  glEnableClientState(GL_COLOR_ARRAY);

  while (1) {
    // get the vertex count and primitive type
    count = *order++;
    if (!count)
      break;  // done

    int c;

    glvert_fast_t *submission_pointer = &r_batchedtempverts[0];

#ifdef GL_EXT_dreamcast_direct_buffer
    glEnable(GL_DIRECT_BUFFER_KOS);
    glDirectBufferReserve_INTERNAL_KOS(count, (int *)&submission_pointer, GL_TRIANGLE_STRIP);
#endif

    glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->vert);
    glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->texture);
#ifdef WIN98
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#else
    glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#endif

    c = count;

    /* Vert data */
    byte* restrict vert1_x = &verts1->v[0];
    byte* restrict vert1_y = &verts1->v[1];
    byte* restrict vert1_z = &verts1->v[2];
    byte* restrict vert1_light = &verts1->lightnormalindex;

    byte* restrict vert2_x = &verts2->v[0];
    byte* restrict vert2_y = &verts2->v[1];
    byte* restrict vert2_z = &verts2->v[2];
    byte* restrict vert2_light = &verts2->lightnormalindex;

    int vert_iter = 0;
    const int vertex_stride = 4;

    /* Texture */
    float* restrict texture_u = (float*)order;
    float* restrict texture_v = (float*)(order+1);

    int tex_iter = 0;
    const int texture_stride = 2;

    do {
      /* Load */
      const byte v1x = vert1_x[vert_iter];
      const byte v1y = vert1_y[vert_iter];
      const byte v1z = vert1_z[vert_iter];
      const byte v1light = vert1_light[vert_iter];

      const byte v2x = vert2_x[vert_iter];
      const byte v2y = vert2_y[vert_iter];
      const byte v2z = vert2_z[vert_iter];
      const byte v2light = vert2_light[vert_iter];

      const float tu = texture_u[tex_iter];
      const float tv = texture_v[tex_iter];

      /* Update */
      const float vxf = (float)v1x + (blend * (float)(v2x - v1x));
      const float vyf = (float)v1y + (blend * (float)(v2y - v1y));
      const float vzf = (float)v1z + (blend * (float)(v2z - v1z));

      /* Interpolate lighting */
      const float delta_light = shadedots[v2light] - shadedots[v1light];
      l = ambientlight < 0 ? 255 : (unsigned char)(((shadedots[v1light] + (blend * delta_light)) * shadelight) * 255);

      *submission_pointer++ = (glvert_fast_t){.flags = VERTEX,
                                              .vert = {vxf, vyf, vzf},
                                              .texture = {tu, tv},
                                              .color = {.packed = PACK_BGRA8888(l, l, l, 255)}, .pad0 = {0}};
      // New iters
      tex_iter += texture_stride;
      vert_iter += vertex_stride;
    } while (--c);
    (*(submission_pointer - 1)).flags = VERTEX_EOL;

    order += 2 * count;
    verts1 += count;
    verts2 += count;

    glDrawArrays(GL_TRIANGLE_STRIP, 0, count);

#ifdef GL_EXT_dreamcast_direct_buffer
    glDisable(GL_DIRECT_BUFFER_KOS);
#endif
#ifdef PARANOID
    if (count > 32)
      printf("%s:%d drew: %d\n", __FILE__, __LINE__, count);
#endif
  }

  glDisableClientState(GL_COLOR_ARRAY);
}

/*
=============
GL_DrawAliasShadow
=============
*/
extern vec3_t lightspot;

void GL_DrawAliasShadow(aliashdr_t *paliashdr, int posenum) {
  trivertx_t *verts;
  int *order;
  float height, lheight;
  int count;

  lheight = currententity->origin[2] - lightspot[2];

  height = 0;
  verts = (trivertx_t *)((byte *)paliashdr + paliashdr->posedata);
  verts += posenum * paliashdr->poseverts;
  order = (int *)((byte *)paliashdr + paliashdr->commands);

  height = -lheight + 1.0;
  glEnableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);

  int c;

  while (1) {
    count = *order++;
    if (!count)
      break;

    glvert_fast_t *submission_pointer = &r_batchedtempverts[0];

#ifdef GL_EXT_dreamcast_direct_buffer
    glEnable(GL_DIRECT_BUFFER_KOS);
    glDirectBufferReserve_INTERNAL_KOS(count, (int *)&submission_pointer, GL_TRIANGLE_STRIP);
#endif

    glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->vert);
    glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->texture);
#ifdef WIN98
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#else
    glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#endif

    for (c = 0; c < count; c++) {
      height -= 0.001f;
      const float x = verts->v[0] * paliashdr->scale[0] + paliashdr->scale_origin[0];
      const float y = verts->v[1] * paliashdr->scale[1] + paliashdr->scale_origin[1];
      const float z = verts->v[2] * paliashdr->scale[2] + paliashdr->scale_origin[2];
      *submission_pointer++ = (glvert_fast_t){.flags = VERTEX, .vert = {x - shadevector[0] * (z + lheight), y - shadevector[1] * (z + lheight), height}, .texture = {0, 0}, .color = {.packed = PACK_BGRA8888(0, 0, 0, 128)}, .pad0 = {0}};
      order += 2;
      verts++;
    }

    (*(submission_pointer - 1)).flags = VERTEX_EOL;
    glDrawArrays(GL_TRIANGLE_STRIP, 0, count);
  }

  #ifdef PARANOID
  if (count > 192)
    printf("%s:%d drew: %d\n", __FILE__, __LINE__, count);
  #endif

#ifdef GL_EXT_dreamcast_direct_buffer
  glDisable(GL_DIRECT_BUFFER_KOS);
#endif

  glEnable(GL_CULL_FACE);
  glEnable(GL_TEXTURE_2D);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
}

/*
=================
R_SetupAliasFrame

=================
*/
void R_SetupAliasFrame(int frame, aliashdr_t *paliashdr) {
  int pose, numposes;
  float interval;

  if ((frame >= paliashdr->numframes) || (frame < 0)) {
    Con_DPrintf("R_AliasSetupFrame: no such frame %d\n", frame);
    frame = 0;
  }

  pose = paliashdr->frames[frame].firstpose;
  numposes = paliashdr->frames[frame].numposes;

  if (numposes > 1) {
    interval = paliashdr->frames[frame].interval;
    pose += (int)(cl.time / interval) % numposes;
  }

  GL_DrawAliasFrame(paliashdr, pose);
}

/*
 =================
 R_SetupAliasBlendedFrame

 fenix@io.com: model animation interpolation
 =================
 */
void R_SetupAliasBlendedFrame(int frame, aliashdr_t *paliashdr, entity_t *e) {
  int pose;
  int numposes;
  float blend;

  if ((frame >= paliashdr->numframes) || (frame < 0)) {
    Con_DPrintf("R_AliasSetupFrame: no such frame %d\n", frame);
    frame = 0;
  }

  pose = paliashdr->frames[frame].firstpose;
  numposes = paliashdr->frames[frame].numposes;

  if (numposes > 1) {
    e->frame_interval = paliashdr->frames[frame].interval;
    pose += (int)(cl.time / e->frame_interval) % numposes;
  } else {
    /* One tenth of a second is a good for most Quake animations.
               If the nextthink is longer then the animation is usually meant to pause
               (e.g. check out the shambler magic animation in shambler.qc).  If its
               shorter then things will still be smoothed partly, and the jumps will be
               less noticable because of the shorter time.  So, this is probably a good
               assumption. */
    e->frame_interval = 0.1;
  }

  if (e->pose2 != pose) {
    e->frame_start_time = realtime;
    e->pose1 = e->pose2;
    e->pose2 = pose;
    blend = 0;
  } else {
    blend = (realtime - e->frame_start_time) / e->frame_interval;
  }

  // wierd things start happening if blend passes 1
  if (cl.paused || blend > 1) blend = 1;

  GL_DrawAliasBlendedFrame(paliashdr, e->pose1, e->pose2, blend);
}

/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel(entity_t *e) {
  int i;
  int lnum;
  vec3_t dist;
  float add;
  model_t *clmodel;
  vec3_t mins, maxs;
  aliashdr_t *paliashdr;
  int anim;

  clmodel = currententity->model;

  VectorAdd(currententity->origin, clmodel->mins, mins);
  VectorAdd(currententity->origin, clmodel->maxs, maxs);

  if (R_CullBox(mins, maxs))
    return;

  VectorCopy(currententity->origin, r_entorigin);
  VectorSubtract(r_origin, r_entorigin, modelorg);

  //
  // get lighting information
  //

  ambientlight = shadelight = R_LightPoint(currententity->origin);

  // allways give the gun some light
  if (e == &cl.viewent && ambientlight < 24)
    ambientlight = shadelight = 24;

  for (lnum = 0; lnum < MAX_DLIGHTS; lnum++) {
    if (cl_dlights[lnum].die >= cl.time) {
      VectorSubtract(currententity->origin,
                     cl_dlights[lnum].origin,
                     dist);
      add = cl_dlights[lnum].radius - Length(dist);

      if (add > 0) {
        ambientlight += add;
        //ZOID models should be affected by dlights as well
        shadelight += add;
      }
    }
  }

  // clamp lighting so it doesn't overbright as much
  if (ambientlight > 128)
    ambientlight = 128;
  if (ambientlight + shadelight > 192)
    shadelight = 192 - ambientlight;

  // ZOID: never allow players to go totally black
  i = currententity - cl_entities;
  if (i >= 1 && i <= cl.maxclients /* && !strcmp (currententity->model->name, "progs/player.mdl") */)
    if (ambientlight < 8)
      ambientlight = shadelight = 8;

  // HACK HACK HACK -- no fullbright colors, so make torches full light
  if (!strcmp(clmodel->name, "progs/flame2.mdl") || !strcmp(clmodel->name, "progs/flame.mdl"))
    ambientlight = shadelight = -1;

  shadedots = r_avertexnormal_dots[((int)(e->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
  shadelight = shadelight / 200.0;

#if defined(_arch_dreamcast) && defined(ENABLE_DC_MATH)
  fsincos(-e->angles[1], &shadevector[1], &shadevector[0]);
#else
  //float an = DEG2RAD(e->angles[1]);
  shadevector[0] = COS(-DEG2RAD(e->angles[1]));
  shadevector[1] = SIN(-DEG2RAD(e->angles[1]));
#endif
  shadevector[2] = 1;
  VectorNormalize(shadevector);

  //
  // locate the proper data
  //
  paliashdr = (aliashdr_t *)Mod_Extradata(currententity->model);

  c_alias_polys += paliashdr->numtris;

  //
  // draw all the triangles
  //

  GL_DisableMultitexture();

  glPushMatrix();
  /* Old */
  // fenix@io.com: model transform interpolation
  if (r_interpolate_pos.value) {
    R_BlendedRotateForEntity(e);
  } else {
    R_RotateForEntity(e);
  }

  if (__builtin_expect(!strcmp(clmodel->name, "progs/eyes.mdl") && gl_doubleeyes.value, 0)) {
    glTranslatef(paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2] - (22 + 8));
    // float size of eyes, since they are really hard to see in gl
    glScalef(paliashdr->scale[0] * 2, paliashdr->scale[1] * 2, paliashdr->scale[2] * 2);
  } else {
    glTranslatef(paliashdr->scale_origin[0], paliashdr->scale_origin[1], paliashdr->scale_origin[2]);
    glScalef(paliashdr->scale[0], paliashdr->scale[1], paliashdr->scale[2]);
  }

  anim = (int)(cl.time * 10) & 3;
  GL_Bind(paliashdr->gl_texturenum[currententity->skinnum][anim]);

  // we can't dynamically colormap textures, so they are cached
  // seperately for the players.  Heads are just uncolored.
  if (currententity->colormap != vid.colormap && !gl_nocolors.value) {
    i = currententity - cl_entities;
    if (i >= 1 && i <= cl.maxclients /* && !strcmp (currententity->model->name, "progs/player.mdl") */)
      GL_Bind(playertextures[i-1]);
  }

  /* Entities are filtered linear because mipamps arent guaranteed*/
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  if (gl_smoothmodels.value)
    glShadeModel(GL_SMOOTH);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  if (gl_affinemodels.value)
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  /* Old */
  // fenix@io.com: model animation interpolation
  if (r_interpolate_anim.value) {
    R_SetupAliasBlendedFrame(currententity->frame, paliashdr, currententity);
  } else {
    R_SetupAliasFrame(currententity->frame, paliashdr);
  }

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _backup_min_filter);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _backup_max_filter);

  glPopMatrix();

  if (r_shadows.value) {
    glPushMatrix();
    R_RotateForEntity(e);
    glDisable(GL_TEXTURE_2D);
    GL_DrawAliasShadow(paliashdr, lastposenum);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glPopMatrix();
  }
}

//==================================================================================

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList(void) {
  int i;

  if (!r_drawentities.value)
    return;

  extern int gl_filter_min;
  extern int gl_filter_max;

  _backup_min_filter = gl_filter_min;
  _backup_max_filter = gl_filter_max;

  // draw sprites seperately, because of alpha blending
  for (i = 0; i < cl_numvisedicts; i++) {
    currententity = cl_visedicts[i];

    switch (currententity->model->type) {
      case mod_alias:
        //@Note: Controls drawing of ALL Living entities
        R_DrawAliasModel(currententity);
        break;

      case mod_brush:
        //@Note: Controls drawing of level entities (doors, panels)
        R_DrawBrushModel(currententity);
        break;

      default:
        break;
    }
  }

  for (i = 0; i < cl_numvisedicts; i++) {
    currententity = cl_visedicts[i];

    if (currententity->model->type == mod_sprite) {
      /*switch (currententity->model->type)
		{
		case mod_sprite:*/
      R_DrawSpriteModel(currententity);
      //break;
    }
  }
}
/*
=============
R_DrawViewModel
=============
*/
void R_DrawViewModel (void)
{
	float		ambient[4], diffuse[4];
	int			j;
	int			lnum;
	vec3_t		dist;
	float		add;
	dlight_t	*dl;
	int			ambientlight, shadelight;

	if (!r_drawviewmodel.value || chase_active.value)
		return;

	if (!r_drawentities.value)
		return;

	if (cl.items & IT_INVISIBILITY)
		return;

	if (cl.stats[STAT_HEALTH] <= 0)
		return;

	currententity = &cl.viewent;
	if (!currententity->model)
		return;
  
	j = R_LightPoint (currententity->origin);

	if (j < 24)
		j = 24;		// allways give some light on gun
	ambientlight = j;
	shadelight = j;

// add dynamic lights		
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		dl = &cl_dlights[lnum];
		if (!dl->radius)
			continue;
		if (!dl->radius)
			continue;
		if (dl->die < cl.time)
			continue;

		VectorSubtract (currententity->origin, dl->origin, dist);
		add = dl->radius - Length(dist);
		if (add > 0)
			ambientlight += add;
	}

	ambient[0] = ambient[1] = ambient[2] = ambient[3] = (float)ambientlight / 128;
	diffuse[0] = diffuse[1] = diffuse[2] = diffuse[3] = (float)shadelight / 128;

// hack the depth range to prevent view model from poking into walls
  float save1 = r_interpolate_anim.value;
  float save2 = r_interpolate_pos.value;
  r_interpolate_anim.value = r_interpolate_pos.value = 0;

  glDepthRange(gldepthmin, gldepthmin + 0.1f*(gldepthmax-gldepthmin));
  R_DrawAliasModel(currententity);
  glDepthRange(gldepthmin, gldepthmax);

  r_interpolate_anim.value = save1;
  r_interpolate_pos.value = save2;
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend(void) {
  if (!gl_polyblend.value)
    return;
  if (!v_blend[3])
    return;

  glDisable(GL_TEXTURE_2D);
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glLoadIdentity();

  glRotatef(-90, 1, 0, 0);  // put Z going up
  glRotatef(90, 0, 0, 1);   // put Z going up
  glvert_fast_t quadvert[4];
  /* 1----2
	   |    |
		 |    |
		 4----3
		 Strip: 1423
		 2Tris: 124423
 	*/
  //Vertex 1
  //Quad vertex
  quadvert[0] = (glvert_fast_t){.flags = VERTEX,
                                .vert = {10, -100, 100},
                                .texture = {0, 1},
                                .color = {.array = {(uint8_t)(v_blend[2] * 255), (uint8_t)(v_blend[1] * 255), (uint8_t)(v_blend[0] * 255), (uint8_t)(v_blend[3] * 255)}},
                                .pad0 = {0}};

  //Vertex 4
  //Quad vertex
  quadvert[1] = (glvert_fast_t){.flags = VERTEX,
                                .vert = {10, -100, -100},
                                .texture = {0, 1},
                                .color = {.array = {(uint8_t)(v_blend[2] * 255), (uint8_t)(v_blend[1] * 255), (uint8_t)(v_blend[0] * 255), (uint8_t)(v_blend[3] * 255)}},
                                .pad0 = {0}};

  //Vertex 2
  //Quad vertex
  quadvert[2] = (glvert_fast_t){.flags = VERTEX,
                                .vert = {10, 100, 100},
                                .texture = {0, 1},
                                .color = {.array = {(uint8_t)(v_blend[2] * 255), (uint8_t)(v_blend[1] * 255), (uint8_t)(v_blend[0] * 255), (uint8_t)(v_blend[3] * 255)}},
                                .pad0 = {0}};

  //Vertex 3
  //Quad vertex
  quadvert[3] = (glvert_fast_t){.flags = VERTEX_EOL,
                                .vert = {10, 100, -100},
                                .texture = {0, 1},
                                .color = {.array = {(uint8_t)(v_blend[2] * 255), (uint8_t)(v_blend[1] * 255), (uint8_t)(v_blend[0] * 255), (uint8_t)(v_blend[3] * 255)}},
                                .pad0 = {0}};

  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);
  glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &quadvert[0].vert);
  glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &quadvert[0].texture);
#ifdef WIN98
  glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &quadvert[0].color);
#else
  glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &quadvert[0].color);
#endif
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDisableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glDepthMask(GL_TRUE);

  glEnable(GL_TEXTURE_2D);
}

int SignbitsForPlane(mplane_t *out) {
  int bits, j;

  // for fast box on planeside test

  bits = 0;
  for (j = 0; j < 3; j++) {
    if (out->normal[j] < 0)
      bits |= 1 << j;
  }
  return bits;
}

void R_SetFrustum(void) {
  int i;

  if (r_refdef.fov_x == 90) {
    // front side is visible

    VectorAdd(vpn, vright, frustum[0].normal);
    VectorSubtract(vpn, vright, frustum[1].normal);

    VectorAdd(vpn, vup, frustum[2].normal);
    VectorSubtract(vpn, vup, frustum[3].normal);
  } else {
    // rotate VPN right by FOV_X/2 degrees
    RotatePointAroundVector(frustum[0].normal, vup, vpn, -(90 - r_refdef.fov_x * 0.5f));
    // rotate VPN left by FOV_X/2 degrees
    RotatePointAroundVector(frustum[1].normal, vup, vpn, 90 - r_refdef.fov_x * 0.5f);
    // rotate VPN up by FOV_X/2 degrees
    RotatePointAroundVector(frustum[2].normal, vright, vpn, 90 - r_refdef.fov_y * 0.5f);
    // rotate VPN down by FOV_X/2 degrees
    RotatePointAroundVector(frustum[3].normal, vright, vpn, -(90 - r_refdef.fov_y * 0.5f));
  }

  for (i = 0; i < 4; i++) {
    frustum[i].type = PLANE_ANYZ;
    frustum[i].dist = DotProduct(r_origin, frustum[i].normal);
    frustum[i].signbits = SignbitsForPlane(&frustum[i]);
  }
}

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame(void) {
  // don't allow cheats in multiplayer
  if (cl.maxclients > 1)
    Cvar_Set("r_fullbright", "0");

  R_AnimateLight();

  r_framecount++;

  // build the transformation matrix for the given view angles
  VectorCopy(r_refdef.vieworg, r_origin);

  AngleVectors(r_refdef.viewangles, vpn, vright, vup);

  // current viewleaf
  r_oldviewleaf = r_viewleaf;
  r_viewleaf = Mod_PointInLeaf(r_origin, cl.worldmodel);

  V_SetContentsColor(r_viewleaf->contents);
  V_CalcBlend();

  r_cache_thrash = false;

  c_brush_polys = 0;
  c_alias_polys = 0;
}

void MYgluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar) {
  GLfloat xmin, xmax, ymin, ymax;

  ymax = zNear * tanf(fovy * M_PI / 360.0f);
  ymin = -ymax;

  xmin = ymin * aspect;
  xmax = ymax * aspect;

  glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL(void) {
  float screenaspect;
  extern int glwidth, glheight;
  int x, x2, y2, y, w, h;

  //
  // set up viewpoint
  //
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  x = r_refdef.vrect.x * glwidth / vid.width;
  x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * glwidth / vid.width;
  y = (vid.height - r_refdef.vrect.y) * glheight / vid.height;
  y2 = (vid.height - (r_refdef.vrect.y + r_refdef.vrect.height)) * glheight / vid.height;

  // fudge around because of frac screen scale
  if (x > 0)
    x--;
  if (x2 < glwidth)
    x2++;
  if (y2 < 0)
    y2--;
  if (y < glheight)
    y++;

  w = x2 - x;
  h = y - y2;

  if (envmap) {
    x = y2 = 0;
    w = h = 256;
  }

  glViewport(glx + x, gly + y2, w, h);
  screenaspect = (float)r_refdef.vrect.width / r_refdef.vrect.height;
  #if 0
  //float yfov = 2 * atanf((float)r_refdef.vrect.height / r_refdef.vrect.width) * 180 / M_PI;
  //gluPerspective(yfov, screenaspect, 4, 4096);
  //MYgluPerspective(r_refdef.fov_y, screenaspect, 4, 4096);
  #endif
  MYgluPerspective(r_refdef.fov_y, screenaspect,  4, 4096);

  //
  // set drawing parms
  //
  if (gl_cull.value)
    glEnable(GL_CULL_FACE);
  else
    glDisable(GL_CULL_FACE);
  glCullFace(GL_FRONT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  //return; //@Note: what is this here for?!?

  mat4 model_temp = GLM_MAT4_IDENTITY_INIT;

  glm_rotate_x(model_temp, DEG2RAD(-90.0f), model_temp);  // put Z going up
  glm_rotate_z(model_temp, DEG2RAD(90), model_temp);      // put Z going up

  glm_rotate_x(model_temp, DEG2RAD(-r_refdef.viewangles[2]), model_temp);
  glm_rotate_y(model_temp, DEG2RAD(-r_refdef.viewangles[0]), model_temp);
  glm_rotate_z(model_temp, DEG2RAD(-r_refdef.viewangles[1]), model_temp);

  glm_translate(model_temp, (vec3){-r_refdef.vieworg[0], -r_refdef.vieworg[1], -r_refdef.vieworg[2]});

  glLoadMatrixf((float *)model_temp);

  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene(void) {

  R_SetupFrame();

  R_SetFrustum();

  R_SetupGL();

  R_MarkLeaves();  // done here so we know if we're in water

  /*@Note: controls world geometry drawing */
  R_DrawWorld();  // adds static entities to the list

  S_ExtraUpdate();  // don't let sound get messed up if going slow

  R_DrawEntitiesOnList();

  GL_DisableMultitexture();

  R_RenderDlights();

  R_DrawParticles();
}

/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
  #if 0
	if (r_mirroralpha.value != 1.0)
	{
		if (gl_clear.value)
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			glClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 0.5;
		glDepthFunc (GL_LEQUAL);
	}
	else if (gl_ztrick.value)
	{
		static int trickframe;

		if (gl_clear.value)
			glClear (GL_COLOR_BUFFER_BIT);

		trickframe++;
		if (trickframe & 1)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			glDepthFunc (GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			glDepthFunc (GL_GEQUAL);
		}
	}
	else
	{
  #endif
		if (gl_clear.value)
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			glClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 1;
		glDepthFunc (GL_LEQUAL);
	//}

	glDepthRange (gldepthmin, gldepthmax);
}

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void R_RenderView(void) {
  float time1 = 0, time2 = 0;

  if (r_norefresh.value)
    return;

  if (!r_worldentity.model || !cl.worldmodel)
    Sys_Error("R_RenderView: NULL worldmodel");

  if (r_speeds.value) {
    time1 = Sys_FloatTime();
    c_brush_polys = 0;
    c_alias_polys = 0;
  }

  mirror = false;

  if (gl_finish.value)
    glFinish();

  R_Clear();

  // render normal view

  /***** Experimental silly looking fog ******
****** Use r_fullbright if you enable ******
	GLfloat colors[4] = {(GLfloat) 0.0, (GLfloat) 0.0, (GLfloat) 1, (GLfloat) 0.20};
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogfv(GL_FOG_COLOR, colors);
	glFogf(GL_FOG_END, 512.0);
	glEnable(GL_FOG);
********************************************/
  //return; //@Note: what is this here for?!?

  R_RenderScene();
  R_DrawViewModel();
  R_DrawWaterSurfaces();

  //  More fog right here :)
  //	glDisable(GL_FOG);
  //  End of all fog code...

  /* render mirror view
	Move check outside, save the call
	if(mirror)
		R_Mirror ();
	*/

  R_PolyBlend(); /*@NOTE: HACK! */

  if (r_speeds.value) {
    //		glFinish ();
    time2 = Sys_FloatTime();
    Con_Printf("%3i ms  %4i wpoly %4i epoly\n", (int)((time2 - time1) * 1000), c_brush_polys, c_alias_polys);
  }
}
