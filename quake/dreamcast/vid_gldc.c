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
// vid_gldc.c -- video driver for dreamcast using KOS and GLdc

#include "quakedef.h"

#include <dc/pvr.h>
#include <dc/sq.h>

viddef_t vid; // global video state

int BASEWIDTH = 640;
int BASEHEIGHT = 480;

unsigned int d_8to24table[256];
unsigned char d_15to8table[65536];
unsigned char *newPalette;
unsigned char backupPalette[256 * 3];
char st[80];

/* Cvars we set to give default acceptable performance */
extern cvar_t gl_smoothmodels;
extern cvar_t gl_affinemodels;
extern cvar_t show_fps;
extern cvar_t gl_keeptjunctions;
extern cvar_t gl_triplebuffer;
extern cvar_t gl_clear;
extern cvar_t snd_noextraupdate;
extern cvar_t r_wateralpha;

float gldepthmin, gldepthmax;

/* Custom Cvars we are Adding */
cvar_t gl_ztrick = {"gl_ztrick", "0"};
cvar_t gl_lighting = {"gl_lighting", "1"};

const GLubyte *gl_vendor;
const GLubyte *gl_renderer;
const GLubyte *gl_version;
const GLubyte *gl_extensions;

qboolean is8bit = true;
qboolean isPermedia = false;
qboolean gl_mtexable = false;


extern void VID_GenerateLighting(qboolean alpha);
void VID_GenerateLighting_f(void);
void VID_ChangeLightmapFilter_f(void);
void VID_Gamma_f(void);

void VID_SetPalette(unsigned char *palette)
{
  byte *pal;
  uint8_t r, g, b;
  unsigned v;
  int r1, g1, b1;
  int k;
  unsigned short i;
  unsigned *table;
  int dist, bestdist;

  if (!newPalette)
    newPalette = (unsigned char *)Z_Malloc(256 * sizeof(uint32_t));
  unsigned int *newPalettePack = (unsigned int *)newPalette;

  //
  // 8 8 8 encoding
  //
  pal = palette;
  table = d_8to24table;
  for (i = 0; i < 256; i++)
  {
    r = pal[0];
    g = pal[1];
    b = pal[2];
    pal += 3;

    newPalettePack[i] = 0xFF << 24 | b << 16 | g << 8 | r;

    v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
    *table++ = v;
  }
  d_8to24table[255] &= 0xFFFFFF; // 255 is transparent
  newPalettePack[255] = 0x00FFFFFF;

  // JACK: 3D distance calcs - k is last closest, l is the distance.
  for (i = 0; i < (1 << 15); i++)
  {
    /* Maps
		000000000000000
		000000000011111 = Red  = 0x1F
		000001111100000 = Blue = 0x03E0
		111110000000000 = Grn  = 0x7C00
		*/
    r = ((i & 0x1F) << 3) + 4;
    g = ((i & 0x03E0) >> 2) + 4;
    b = ((i & 0x7C00) >> 7) + 4;
    pal = (unsigned char *)d_8to24table;
    for (v = 0, k = 0, bestdist = 10000 * 10000; v < 256; v++, pal += 4)
    {
      r1 = (int)r - (int)pal[0];
      g1 = (int)g - (int)pal[1];
      b1 = (int)b - (int)pal[2];
      dist = (r1 * r1) + (g1 * g1) + (b1 * b1);
      if (dist < bestdist)
      {
        k = v;
        bestdist = dist;
      }
    }
    d_15to8table[i] = k;
  }
}

void VID_ShiftPalette(unsigned char *palette)
{
  (void)palette;
  //	VID_SetPalette(palette);
}

void VID_Init8bitPalette(void)
{
  Con_SafePrintf("... Using GL_EXT_shared_texture_palette\n");
  glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
  glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, GL_RGBA8, 256, GL_RGBA, GL_UNSIGNED_BYTE, (void *)newPalette);

  VID_GenerateLighting(true);
  is8bit = true;
}

void VID_Shutdown(void)
{
}

qboolean VID_Is8bit(void)
{
  return is8bit;
}

extern int gl_filter_min;
extern int gl_filter_max;

/*
===============
GL_Init
===============
*/
void GL_Init(void)
{
  gl_vendor = glGetString(GL_VENDOR);
  Con_Printf("GL_VENDOR: %s\n", gl_vendor);
  gl_renderer = glGetString(GL_RENDERER);
  Con_Printf("GL_RENDERER: %s\n", gl_renderer);

  gl_version = glGetString(GL_VERSION);
  Con_Printf("GL_VERSION: %s\n", gl_version);
  gl_extensions = glGetString(GL_EXTENSIONS);

  Con_Printf("\nThanks to everyone EXCEPT Saturn-Tan!\n\n");
  Con_Printf("\n%s\n\n", st);

  glClearColor(1, 1, 1, 1);
  glEnable(GL_TEXTURE_2D);

  //glAlphaFunc(GL_GREATER, 0.666f);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);

  //glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
  //glShadeModel(GL_FLAT);

  //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

void VID_Aspect_f(void) {
  if (Cmd_Argc() < 1)
    return;

  if (atoi(Cmd_Argv(1))) {
    /* Widescreen */
    vid.aspect = ((float)853.0 / (float)480.0) * (320.0 / 240.0);
  } else {
    /* 4:3 */
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
    //vid.aspect = 4.0f/3.0f;
  }
  extern void V_CalcRefdef(void);
  V_CalcRefdef();
}


/*
=================
GL_BeginRendering

=================
*/
void GL_BeginRendering(int *x, int *y, int *width, int *height)
{
  *x = 0;
  *y = 0;
  *width = BASEWIDTH;
  *height = BASEHEIGHT;
  glEnable(GL_NEARZ_CLIPPING_KOS);
}

extern void glKosSwapBuffers_NO_PT(void);
void GL_EndRendering(void)
{
  /* I believe this mirrors the Windows driver, otherwise status bar flicker */
  Sbar_Changed();
  //glKosSwapBuffers_NO_PT();
  glKosSwapBuffers();
}

void VID_Cvar_Update(void)
{
  //Cvar Changes
  gl_smoothmodels.value = 1;
  gl_affinemodels.value = 1;
  gl_keeptjunctions.value = 0;
  gl_triplebuffer.value = 0;
  snd_noextraupdate.value = 1;
  gl_clear.value = 0;
  //r_wateralpha.value = 0.5;
  show_fps.value = 1;
}

void VID_Init(unsigned char *palette)
{
  Cvar_RegisterVariable(&gl_ztrick);
  Cvar_RegisterVariable(&gl_lighting);

  Cmd_AddCommand("genlmp", VID_GenerateLighting_f);
  Cmd_AddCommand("lmpquality", VID_ChangeLightmapFilter_f);
  Cmd_AddCommand("vid_gamma", VID_Gamma_f);
  Cmd_AddCommand("aspect", VID_Aspect_f);

  vid.width = vid.conwidth = 320;
  vid.height = vid.conheight = 240;

#if 0 //Widescreen
  vid.aspect = ((float)853.0 / (float)480.0) * (320.0 / 240.0);
#else
  vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
//vid.aspect = 4.0f/3.0f;
#endif

  vid.numpages = 1;
  vid.colormap = host_colormap;
  vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));
  vid.rowbytes = vid.conrowbytes = BASEWIDTH / 2;

  //sprintf(st, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", 65, 76, 80, 72, 65, 32, 110, 117, 81, 117, 97, 107, 101, 32, 65, 76, 80, 72, 65);
  strcat(st, "\x6e\x6f\x74\x20\x66\x6f\x72\x20\x64\x69\x73\x74\x72\x69\x62\x75\x74\x69\x6f\x6e\x20\x74\x6f\n");
  strcat(st, "\x22\x44\x72\x65\x61\x6d\x63\x61\x73\x74\x20\x4f\x6e\x6c\x69\x6e\x65\x22\x20\x6f\x72\x20\x22\x53\x65\x67\x61\x4e\x65\x74\x22");
  GL_Init();

  memcpy(backupPalette, palette, 256 * 3);

  VID_SetPalette(palette);

  VID_Init8bitPalette();

  vid.recalc_refdef = true; // force a surface cache flush

  VID_Cvar_Update();
}
