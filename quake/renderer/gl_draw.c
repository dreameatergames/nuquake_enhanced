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

// draw.c -- this is the only file outside the refresh that touches the
// vid buffer

#include "quakedef.h"
typedef unsigned char uint8_t;

void DrawQuad(int x, int y, int w, int h, float u, float v, float uw, float vh);
void DrawQuad_NoTex(float x, float y, float w, float h, int c);

#define GL_COLOR_INDEX8_EXT 0x80E5

extern unsigned char d_15to8table[65536];

cvar_t gl_nobind = {"gl_nobind", "0"};
cvar_t gl_max_size = {"gl_max_size", "256"};
cvar_t gl_picmip = {"gl_picmip", "0"};
extern cvar_t scr_safety;

byte *draw_chars; // 8*8 graphic characters
qpic_t *draw_disc;
qpic_t *draw_backtile;

unsigned int translate_texture = 0;
unsigned int char_texture = 0;

int vid_halfwidth = 0;
int vid_halfheight = 0;

typedef struct {
  int texnum;
  float sl, tl, sh, th;
} glpic_t;

byte conback_buffer[sizeof(qpic_t) + sizeof(glpic_t)];
qpic_t *conback = (qpic_t *)&conback_buffer;

int gl_lightmap_format = GL_RGBA;
int gl_solid_format = GL_RGB;
int gl_alpha_format = GL_RGBA;

int gl_filter_min = GL_LINEAR;
int gl_filter_max = GL_LINEAR;

typedef struct {
  GLuint texnum;
  int width, height;
  int lhcsum;
  char identifier[64];
  uint8_t flags;
} gltexture_t;

static int numgltextures;
#define MAX_GLTEXTURES 512
gltexture_t gltextures[MAX_GLTEXTURES];

/*
 * @Note: define static texture memory
 */
static unsigned char __attribute__((aligned(32))) buffer_aligned[512 * 512 * 4];
#ifndef _arch_dreamcast
static unsigned int scaled[1024 * 512]; // [512*256];
#else
static unsigned char *scaled_raw = NULL;
static unsigned char *scaled = NULL;
#endif

void GL_FreeTextures(void) {
  int i, j;

  // Con_DPrintf("GL_FreeTextures: Entry.\n");
  /*
          if (gl_free_world_textures.value == 0)
          {
                  Con_DPrintf("GL_FreeTextures: Not Clearing old Map
     Textures.\n"); return;
          }
  */
  Con_DPrintf("GL_FreeTextures: Freeing textures (numgltextures = %i) \n",
              numgltextures);

  for (i = j = 0; i < numgltextures; ++i, ++j) {
    if (gltextures[i].flags &
        TEX_WORLD) // Only clear out world textures... for now.
    {
      // Con_DPrintf("GL_FreeTextures: Clearing texture %s\n",
      // gltextures[i].identifier);
      glDeleteTextures(1, (GLuint *)(&gltextures[i].texnum));
      memset(&gltextures[i], 0, sizeof(gltexture_t));
      --j;
    } else if (j < i) {
      // Con_DDPrintf("GL_FreeTextures: NOT Clearing texture %s\n",
      // gltextures[i].identifier);
      gltextures[j] = gltextures[i];
    }
  }

  numgltextures = j;
#ifdef GL_EXT_dreamcast_yalloc
  Con_DPrintf("GL_FreeTextures: Completed (numgltextures = %i) \n",
              numgltextures);
  Con_Printf("GL Mem Before:%u\n", (unsigned int)glGetFreeVRAM_INTERNAL_KOS());
  Con_Printf("GL Mem Block:%u\n",
             (unsigned int)glGetContinuousVRAM_INTERNAL_KOS());
  // glDumpVRAM_INTERNAL_KOS();
  glDefragmentVRAM_INTERNAL_KOS();
  Con_Printf("GL Mem After:%u\n", (unsigned int)glGetFreeVRAM_INTERNAL_KOS());
  Con_Printf("GL Mem Block:%u\n",
             (unsigned int)glGetContinuousVRAM_INTERNAL_KOS());
  // glDumpVRAM_INTERNAL_KOS();
#endif
}

void GL_Bind(unsigned int texnum) {
  if (gl_nobind.value)
    texnum = char_texture;
  if (currenttexture == texnum)
    return;
  currenttexture = texnum;
#if defined(_WIN32)
  bindTexFunc(GL_TEXTURE_2D, texnum);
#else
  glBindTexture(GL_TEXTURE_2D, texnum);
#endif
}

void GL_OverscanAdjust(int *x, int *y) {
  if (scr_safety.value > 4.0f) {
    if (*x < scr_safety.value) {
      *x += scr_safety.value;
    }
    if (*x > vid.width - scr_safety.value) {
      *x -= scr_safety.value;
    }
    if (*y < scr_safety.value) {
      *y += scr_safety.value;
    }
    if (*y > vid.height - scr_safety.value) {
      *y -= scr_safety.value;
    }

    /* Old */
    /*if( *x < vid_halfwidth){
                        *x += scr_safety.value;
                } else {
                        *x -= scr_safety.value;
                }
                if( *y < vid_halfheight){
                        *y += scr_safety.value;
                } else {
                        *y -= scr_safety.value;
                }*/
  }
}

/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up stupid hardware / drivers

=============================================================================
*/

#define MAX_SCRAPS 4
#define BLOCK_WIDTH 256
#define BLOCK_HEIGHT 256
#define TEXTURE_PADDING 1 // Increased padding to ensure no bleeding

int scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte __attribute__((
    aligned(32))) scrap_texels[MAX_SCRAPS][BLOCK_WIDTH * BLOCK_HEIGHT];
qboolean scrap_dirty;
int scrap_texnum;

// returns a texture number and the position inside it
int Scrap_AllocBlock(int w, int h, int *x, int *y) {
  int i, j;
  int best, best2;
  int texnum;

  for (texnum = 0; texnum < MAX_SCRAPS; texnum++) {
    best = BLOCK_HEIGHT;

    for (i = 0; i < BLOCK_WIDTH - w; i++) {
      best2 = 0;

      for (j = 0; j < w; j++) {
        if (scrap_allocated[texnum][i + j] >= best)
          break;
        if (scrap_allocated[texnum][i + j] > best2)
          best2 = scrap_allocated[texnum][i + j];
      }
      if (j == w) { // this is a valid spot
        *x = i;
        *y = best = best2;
      }
    }

    if (best + h > BLOCK_HEIGHT)
      continue;

    for (i = 0; i < w; i++)
      scrap_allocated[texnum][*x + i] = best + h;

    return texnum;
  }
  // return -1; /* Die Gracefully */
  return 0; /* Die Gracefully */
  // Sys_Error("Scrap_AllocBlock: full");
  // return 0; /* We Died anyways */
}

int scrap_uploads;

void Scrap_Upload(void) {
  int texnum;

  scrap_uploads++;

  for (texnum = 0; texnum < MAX_SCRAPS; texnum++) {
    GL_Bind(scrap_texnum + texnum);
    GL_Upload8(scrap_texels[texnum], BLOCK_WIDTH, BLOCK_HEIGHT, TEX_ALPHA);
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  scrap_dirty = false;
}

//=============================================================================
/* Support Routines */

/*
================
GL_LoadPicTexture
================
*/
int GL_LoadPicTexture(qpic_t *pic) {
  const int width = pic->width;
  const int height = pic->height;
  byte *data = pic->data;
  const int ret = GL_LoadTexture("", width, height, data, TEX_ALPHA);
  return ret;
}

typedef struct cachepic_s {
  char name[MAX_QPATH];
  qpic_t pic;
  byte padding[32]; // for appended glpic
} cachepic_t;

#define MAX_CACHED_PICS 128
cachepic_t menu_cachepics[MAX_CACHED_PICS];
int menu_numcachepics;

byte menuplyr_pixels[4096];

int pic_texels;
int pic_count;

qpic_t *Draw_PicFromWad(char *name) {
  qpic_t *p;
  glpic_t *gl;

  p = W_GetLumpName(name);
  gl = (glpic_t *)p->data;

  // load little ones into the scrap
  if (p->width < 64 && p->height < 64) {
    int x, y;
    int i, j, k;
    int texnum;
    x = y = 0;

    texnum = Scrap_AllocBlock(p->width, p->height, &x, &y);
    if (texnum == -1) {
      gl->texnum = GL_LoadPicTexture(p);
      gl->sl = 0;
      gl->sh = 1;
      gl->tl = 0;
      gl->th = 1;
      return p;
    }
    scrap_dirty = true;
    k = 0;
    for (i = 0; i < p->height; i++)
      for (j = 0; j < p->width; j++, k++)
        scrap_texels[texnum][(y + i) * BLOCK_WIDTH + x + j] = p->data[k];
    texnum += scrap_texnum;
    gl->texnum = texnum;
    gl->sl = (x + 0.01) / (float)BLOCK_WIDTH;
    gl->sh = (x + p->width - 0.01) / (float)BLOCK_WIDTH;
    gl->tl = (y + 0.01) / (float)BLOCK_WIDTH;
    gl->th = (y + p->height - 0.01) / (float)BLOCK_WIDTH;

    pic_count++;
    pic_texels += p->width * p->height;
  } else {
    gl->texnum = GL_LoadPicTexture(p);
    gl->sl = 0;
    gl->sh = 1;
    gl->tl = 0;
    gl->th = 1;
  }
  return p;
}

qpic_t pic_null = {1, 1, {0}};

/*
================
Draw_CachePic
================
*/
qpic_t *Draw_CachePic(char *path) {
  cachepic_t *pic;
  int i;
  qpic_t *dat;
  glpic_t *gl;

  for (pic = menu_cachepics, i = 0; i < menu_numcachepics; pic++, i++)
    if (!strcmp(path, pic->name))
      return &pic->pic;

  if (menu_numcachepics == MAX_CACHED_PICS)
    Sys_Error("menu_numcachepics == MAX_CACHED_PICS");
  menu_numcachepics++;
  strcpy(pic->name, path);

  //
  // load the pic from disk
  //
  dat = (qpic_t *)COM_LoadTempFile(path);
  if (!dat) {
    // Sys_Error("Draw_CachePic: failed to load %s", path);
    Con_Printf("Draw_CachePic: failed to load %s\n", path);
    /* shareware 0.91 doesnt have this */
    /*@Note: SW Hack */
    extern qboolean sw91;
    sw91 = true;
    dat = &pic_null;
  }
  SwapPic(dat);

  // HACK HACK HACK --- we need to keep the bytes for
  // the translatable player picture just for the menu
  // configuration dialog
  if (!strcmp(path, "gfx/menuplyr.lmp"))
    memcpy(menuplyr_pixels, dat->data, dat->width * dat->height);

  pic->pic.width = dat->width;
  pic->pic.height = dat->height;

  gl = (glpic_t *)pic->pic.data;
  gl->texnum = GL_LoadPicTexture(dat);
  gl->sl = 0;
  gl->sh = 1;
  gl->tl = 0;
  gl->th = 1;

  return &pic->pic;
}

void Draw_CharToConback(int num, byte *dest) {
  int row, col;
  byte *source;
  int drawline;
  int x;

  row = num >> 4;
  col = num & 15;
  source = draw_chars + (row << 10) + (col << 3);

  drawline = 8;

  while (drawline--) {
    for (x = 0; x < 8; x++)
      if (source[x] != 255)
        dest[x] = 0x60 + source[x];
    source += 128;
    dest += 320;
  }
}

typedef struct {
  char *name;
  int minimize, maximize;
} glmode_t;

glmode_t modes[] = {
    {"GL_NEAREST", GL_NEAREST, GL_NEAREST},
    {"GL_LINEAR", GL_LINEAR, GL_LINEAR},
    {"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
    //{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
    {"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
    //{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};
#define MAX_MODES 4

int current_texmode = 1;
extern unsigned int lightmap_textures[MAX_LIGHTMAPS];
/*
===============
Draw_TextureMode_f
===============
*/
void Draw_TextureMode_f(void) {
  int i;
  gltexture_t *glt;

  if (Cmd_Argc() == 1) {
    for (i = 0; i < MAX_MODES; i++)
      if (gl_filter_min == modes[i].minimize) {
        Con_Printf("%s\n", modes[i].name);
        return;
      }
    Con_Printf("current filter is unknown???\n");
    return;
  }

  int temp = atoi(Cmd_Argv(1));
  if (temp > 0) {
    i = temp - 1;
  } else {
    for (i = 0; i < MAX_MODES; i++) {
      if (!strcasecmp(modes[i].name, Cmd_Argv(1)))
        break;
    }
  }
  if (i >= MAX_MODES) {
    Con_Printf("bad filter name\n");
    return;
  }

  gl_filter_min = modes[i].minimize;
  gl_filter_max = modes[i].maximize;

  current_texmode = i;

  // change all the existing mipmap texture objects
  for (i = 0, glt = gltextures; i < numgltextures; i++, glt++) {
    GL_Bind(glt->texnum);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    ((glt->flags & TEX_MIP) == TEX_MIP) ? gl_filter_min
                                                        : gl_filter_max);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
  }
  for (i = 0; i < MAX_LIGHTMAPS; i++) {
    GL_Bind(lightmap_textures[i]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
}

void Draw_Set_TextureMode(int i) {
  if (i >= 0 && i < MAX_MODES) {
    current_texmode = i;
    gl_filter_min = modes[i].minimize;
    gl_filter_max = modes[i].maximize;

    gltexture_t *glt;
    int i;

    // change all the existing mipmap texture objects
    for (i = 0, glt = gltextures; i < numgltextures; i++, glt++) {
      GL_Bind(glt->texnum);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      ((glt->flags & TEX_MIP) == TEX_MIP) ? gl_filter_min
                                                          : gl_filter_max);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
    }
    for (i = 0; i < MAX_LIGHTMAPS; i++) {
      GL_Bind(lightmap_textures[i]);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
  }
}

/*
===============
Draw_Init
===============
*/
void Draw_Init(void) {
  vid_halfwidth = vid.width >> 1;
  vid_halfheight = vid.height >> 1;

  int i;
  qpic_t *cb;
/* Used for drawing on conback */
#if 0
  unsigned int x, y;
  byte *dest;
  char ver[40];
#endif
  glpic_t *gl;
  int start;
  byte *ncdata;

  Cvar_RegisterVariable(&gl_nobind);
  Cvar_RegisterVariable(&gl_max_size);
  Cvar_RegisterVariable(&gl_picmip);

  // 3dfx can only handle 256 wide textures
  if (!strncasecmp((char *)gl_renderer, "3dfx", 4) ||
      strstr((char *)gl_renderer, "Glide"))
    Cvar_Set("gl_max_size", "256");

  Cmd_AddCommand("gl_texturemode", &Draw_TextureMode_f);

  // load the console background and the charset
  // by hand, because we need to write the version
  // string into the background before turning
  // it into a texture
  draw_chars = W_GetLumpName("conchars");
  for (i = 0; i < 256 * 64; i++)
    if (draw_chars[i] == 0)
      draw_chars[i] = 255; // proper transparent color

  // now turn them into textures
  char_texture = GL_LoadTexture("charset", 128, 128, draw_chars, TEX_ALPHA);

  start = Hunk_LowMark();

  cb = (qpic_t *)COM_LoadTempFile("gfx/conback.lmp");
  if (!cb)
    Sys_Error("Couldn't load gfx/conback.lmp");
  SwapPic(cb);

/* No need to plaster version numbers */
/*@Issue: 21 */
#if 0
  char gl_ver[5], game_ver[5];
  ftoa(GLQUAKE_VERSION, gl_ver, 4, 2);
  ftoa(VERSION, game_ver, 4, 2);

  // hack the version number directly into the pic
#if defined(__linux__)
  char lin_ver[5];
  ftoa(LINUX_VERSION, lin_ver, 4,2);
  sprintf(ver, "(Linux %s, gl %s) %s", lin_ver, gl_ver, game_ver);
  //sprintf(ver, "(Linux %2.2f, gl %4.2f) %4.2f", LINUX_VERSION, GLQUAKE_VERSION, VERSION);
#else
  sprintf(ver, "(gl %s) %s", gl_ver, game_ver);
  //sprintf(ver, "(gl %4.2f) %4.2f", GLQUAKE_VERSION, VERSION);
#endif
  dest = cb->data + 320 * 186 + 320 - 11 - 8 * strlen(ver);
  y = strlen(ver);
  for (x = 0; x < y; x++)
    Draw_CharToConback(ver[x], dest + (x << 3));
#endif

#ifdef _arch_dreamcast
  unsigned int x, y;
  int f, fstep;
  byte *src;
  byte *dest;
  conback->width = vid.conwidth;
  conback->height = vid.conheight;

  // scale console to vid size
  dest = ncdata = Hunk_AllocName(vid.conwidth * vid.conheight, "conback");

  for (y = 0; y < vid.conheight; y++, dest += vid.conwidth) {
    src = cb->data + cb->width * (y * cb->height / vid.conheight);
    if (vid.conwidth == (unsigned)cb->width)
      memcpy(dest, src, vid.conwidth);
    else {
      f = 0;
      fstep = cb->width * 0x10000 / vid.conwidth;
      for (x = 0; x < vid.conwidth; x += 4) {
        dest[x] = src[f >> 16];
        f += fstep;
        dest[x + 1] = src[f >> 16];
        f += fstep;
        dest[x + 2] = src[f >> 16];
        f += fstep;
        dest[x + 3] = src[f >> 16];
        f += fstep;
      }
    }
  }
#else
  conback->width = cb->width;
  conback->height = cb->height;
  ncdata = cb->data;
#endif
  gl = (glpic_t *)conback->data;
  gl->texnum = GL_LoadTexture("conback", conback->width, conback->height,
                              ncdata, TEX_NONE);
  gl->sl = 0;
  gl->sh = 1;
  gl->tl = 0;
  gl->th = 1;
  conback->width = vid.width;
  conback->height = vid.height;

  // free loaded console
  Hunk_FreeToLowMark(start);

  // save a texture slot for translated picture
  glGenTextures(1, (GLuint *)&translate_texture);

  // save slots for scraps
  GLuint temp[MAX_SCRAPS - 1];
  glGenTextures(1, (GLuint *)&scrap_texnum);
  glGenTextures(MAX_SCRAPS - 1, temp);

  //
  // get the other pics we need
  //
  draw_disc = Draw_PicFromWad("disc");
  draw_backtile = Draw_PicFromWad("backtile");
}

/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character(int x, int y, int num) {
  GL_OverscanAdjust(&x, &y);
  int row, col;
  float v, u, size;

  if (num == 32)
    return; // space

  num &= 255;

  if (y <= -8)
    return; // totally off screen

  row = num >> 4;
  col = num & 15;

  // Add a small offset to prevent texture bleeding
  // and ensure proper alignment
  v = row * 0.0625f + 0.001f;
  u = col * 0.0625f + 0.001f;
  size = 0.0625f - 0.002f; // Slightly smaller to account for the offset

  // Ensure x and y are properly aligned for the hardware
  x = (x + 4) & ~7; // Align to 8-pixel boundary
  y = (y + 4) & ~7;

  GL_Bind(char_texture);

  R_BeginBatchingSurfacesQuad();
  R_BatchSurfaceQuadText(x, y, v, u, size);
  R_EndBatchingSurfacesQuads();
}

/*
================
Draw_String
================
*/
void Draw_String(int x, int y, char *str) {
  GL_OverscanAdjust(&x, &y);
  GL_Bind(char_texture);
  R_BeginBatchingSurfacesQuad();
  while (*str) {
    int row, col;
    float v, u, size;
    unsigned char num;

    num = (unsigned char)*str & 255;
    if (num == 32) {
      str++;
      x += 8;
      continue;
    }

    row = num >> 4;
    col = num & 15;
    v = row * 0.0625f;
    u = col * 0.0625f;
    size = 0.0625f;

    R_BatchSurfaceQuadText(x, y, v, u, size);
    str++;
    x += 8;
  }
  R_EndBatchingSurfacesQuads();
}

/*
================
Draw_DebugChar

Draws a single character directly to the upper right corner of the screen.
This is for debugging lockups by drawing different chars in different parts
of the code.
================
*/
void Draw_DebugChar(char num) { (void)num; }

/*
=============
Draw_AlphaPic
=============
*/
static uint8_t quad_alpha = 255;
void Draw_AlphaPic(int x, int y, qpic_t *pic, float alpha) {
  GL_OverscanAdjust(&x, &y);
  glpic_t *gl;

  if (scrap_dirty)
    Scrap_Upload();

  gl = (glpic_t *)pic->data;

  // Setup blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Disable culling for consistent rendering
  glDisable(GL_CULL_FACE);

  // Set alpha
  quad_alpha = (uint8_t)(alpha * 255.0f);

  // Bind texture and ensure proper filtering
  GL_Bind(gl->texnum);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Ensure coordinates are hardware aligned
  x = (x + 4) & ~7;
  y = (y + 4) & ~7;

  // Draw the quad
  DrawQuad(x, y, pic->width, pic->height, gl->sl, gl->tl, gl->sh - gl->sl,
           gl->th - gl->tl);

  // Restore state
  quad_alpha = 255;
  glEnable(GL_CULL_FACE);
  glDisable(GL_BLEND);
}
/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int x, int y, qpic_t *pic) {
  GL_OverscanAdjust(&x, &y);
  glpic_t *gl;

  if (scrap_dirty)
    Scrap_Upload();
  gl = (glpic_t *)pic->data;
  GL_Bind(gl->texnum);
  DrawQuad(x, y, pic->width, pic->height, gl->sl, gl->tl, gl->sh - gl->sl,
           gl->th - gl->tl);
}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic(int x, int y, qpic_t *pic) {
  Draw_AlphaPic(x, y, pic, 1.0f);
}

/*
=============
Draw_TransPicTranslate

Only used for the player color selection menu
=============
*/
static unsigned __attribute__((aligned(32))) trans[64 * 64];
void Draw_TransPicTranslate(int x, int y, qpic_t *pic, byte *translation) {
  GL_OverscanAdjust(&x, &y);
  int v, u;
  unsigned *dest;
  byte *src;
  int p;

  GL_Bind(translate_texture);

  // Use actual pic dimensions for scaling
  float scale_h = (float)pic->height / 64.0f;
  float scale_w = (float)pic->width / 64.0f;

  dest = trans;
  for (v = 0; v < 64; v++, dest += 64) {
    src = &menuplyr_pixels[(int)(v * scale_h) * pic->width];
    for (u = 0; u < 64; u++) {

      p = src[(int)(u * scale_w)];
      if (p == 255)
        dest[u] = 0; // Make transparent pixels actually transparent
      else
        dest[u] = d_8to24table[translation[p]];
    }
  }

  glTexImage2D(GL_TEXTURE_2D, 0, gl_alpha_format, 64, 64, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, trans);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  DrawQuad(x, y, pic->width, pic->height, 0, 0, 1, 1);
}

/*
================
Draw_ConsoleBackground

================
*/
void Draw_ConsoleBackground(int lines) {
  glpic_t *temp = (glpic_t *)conback->data;
  GL_Bind(temp->texnum);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  //@Note: No blending on console, eyecandy we dont need
#ifndef _arch_dreamcast
  int y = (vid.height * 3) >> 2;

  if (lines > y)
    Draw_Pic(0, lines - vid.height, conback);
  else
    Draw_AlphaPic(0, lines - vid.height, conback, (float)(1.2f * lines) / y);
#else
  Draw_Pic(0, lines - vid.height, conback);
#endif
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear(int x, int y, int w, int h) {
  unsigned int temp;
  memcpy(&temp, draw_backtile->data, sizeof(unsigned int));

  GL_Bind(temp);
  DrawQuad(x, y, w, h, x / 64.0, y / 64.0, w / 64.0, h / 64.0);
}

/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, int c) {
  glDisable(GL_TEXTURE_2D);
  DrawQuad_NoTex(x, y, w, h, c);
  glEnable(GL_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void) {
  glDisable(GL_ALPHA_TEST);
  glEnable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
  /*@Note: dreamcast hack */
  glDepthMask(GL_FALSE);
#define OPACITY 0.6f

  glvert_fast_t quadvert[4];
  /* 1----2
           |    |
                 |    |
                 4----3
                 Strip: 1423
                 2Tris: 124423
        */
  // Vertex 1
  // Quad vertex
  // quadvert[0] = (glvert_fast_t){.flags = VERTEX, .vert = {0, 0, 0}, .texture
  // = {0, 0}, .color = {0, 0, 0, (uint8_t)(255 * OPACITY)}, .pad0 = {0}};
  quadvert[0] = (glvert_fast_t){
      .flags = VERTEX,
      .vert = {0, 0, 0},
      .texture = {0, 0},
      .color = {.packed = PACK_BGRA8888(0, 0, 0, (uint8_t)(255 * OPACITY))},
      .pad0 = {0}};

  // Vertex 4
  // Quad vertex
  // quadvert[1] = (glvert_fast_t){.flags = VERTEX, .vert = {0, vid.height, 0},
  // .texture = {0, 0}, .color = {0, 0, 0, (uint8_t)(255 * OPACITY)}, .pad0 =
  // {0}};
  quadvert[1] = (glvert_fast_t){
      .flags = VERTEX,
      .vert = {0, vid.height, 0},
      .texture = {0, 0},
      .color = {.packed = PACK_BGRA8888(0, 0, 0, (uint8_t)(255 * OPACITY))},
      .pad0 = {0}};

  // Vertex 2
  // Quad vertex
  // quadvert[2] = (glvert_fast_t){.flags = VERTEX, .vert = {vid.width, 0, 0},
  // .texture = {0, 0}, .color = {0, 0, 0, (uint8_t)(255 * OPACITY)}, .pad0 =
  // {0}};
  quadvert[2] = (glvert_fast_t){
      .flags = VERTEX,
      .vert = {vid.width, 0, 0},
      .texture = {0, 0},
      .color = {.packed = PACK_BGRA8888(0, 0, 0, (uint8_t)(255 * OPACITY))},
      .pad0 = {0}};

  // Vertex 3
  // Quad vertex
  // quadvert[3] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {vid.width,
  // vid.height, 0}, .texture = {0, 0}, .color = {0, 0, 0, (uint8_t)(255 *
  // OPACITY)}, .pad0 = {0}};
  quadvert[3] = (glvert_fast_t){
      .flags = VERTEX_EOL,
      .vert = {vid.width, vid.height, 0},
      .texture = {0, 0},
      .color = {.packed = PACK_BGRA8888(0, 0, 0, (uint8_t)(255 * OPACITY))},
      .pad0 = {0}};

  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &quadvert[0].vert);
  glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &quadvert[0].texture);
#ifdef WIN98
  glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t),
                 &quadvert[0].color);
#else
  glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t),
                 &quadvert[0].color);
#endif
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDisableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  glDisable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  /*@Note: dreamcast hack */
  glDepthMask(GL_TRUE);
  Sbar_Changed();
}

//=============================================================================

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc(void) {
#if 0
  if (!draw_disc)
    return;
  glDrawBuffer(GL_FRONT);
  Draw_Pic(vid.width - 24, 0, draw_disc);
  glDrawBuffer(GL_BACK);
#endif
}

/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc(void) {}

/*
================
GL_Set2D

Setup as if the screen was 320*200
================
*/
void GL_Set2D(void) {
  glViewport(glx, gly, glwidth, glheight);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, vid.width, vid.height, 0, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_CULL_FACE);

#ifdef _arch_dreamcast
  glDisable(GL_NEARZ_CLIPPING_KOS);
#endif
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

//====================================================================

/*
================
GL_FindTexture
================
*/
int GL_FindTexture(char *identifier) {
  int i;
  gltexture_t *glt;

  for (i = 0, glt = gltextures; i < numgltextures; i++, glt++) {
    if (!strcmp(identifier, glt->identifier))
      return gltextures[i].texnum;
  }

  return -1;
}

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out,
                        int outwidth, int outheight) {
  int i, j;
  unsigned *inrow;
  unsigned frac, fracstep;

  fracstep = inwidth * 0x10000 / outwidth;
  for (i = 0; i < outheight; i++, out += outwidth) {
    inrow = in + inwidth * (i * inheight / outheight);
    frac = fracstep >> 1;
    for (j = 0; j < outwidth; j += 4) {
      out[j] = inrow[frac >> 16];
      frac += fracstep;
      out[j + 1] = inrow[frac >> 16];
      frac += fracstep;
      out[j + 2] = inrow[frac >> 16];
      frac += fracstep;
      out[j + 3] = inrow[frac >> 16];
      frac += fracstep;
    }
  }
}

/*
================
GL_Resample8BitTexture -- JACK
================
*/
void GL_Resample8BitTexture(unsigned char *in, int inwidth, int inheight,
                            unsigned char *out, int outwidth, int outheight) {
  int i, j;
  unsigned char *inrow;
  unsigned frac, fracstep;

  fracstep = inwidth * 0x10000 / outwidth;
  for (i = 0; i < outheight; i++, out += outwidth) {
    inrow = in + inwidth * (i * inheight / outheight);
    frac = fracstep >> 1;
    for (j = 0; j < outwidth; j += 4) {
      out[j] = inrow[frac >> 16];
      frac += fracstep;
      out[j + 1] = inrow[frac >> 16];
      frac += fracstep;
      out[j + 2] = inrow[frac >> 16];
      frac += fracstep;
      out[j + 3] = inrow[frac >> 16];
      frac += fracstep;
    }
  }
}

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap(byte *in, int width, int height) {
  int i, j;
  byte *out;

  width <<= 2;
  height >>= 1;
  out = in;
  for (i = 0; i < height; i++, in += width) {
    for (j = 0; j < width; j += 8, out += 4, in += 8) {
      out[0] = (in[0] + in[4] + in[width + 0] + in[width + 4]) >> 2;
      out[1] = (in[1] + in[5] + in[width + 1] + in[width + 5]) >> 2;
      out[2] = (in[2] + in[6] + in[width + 2] + in[width + 6]) >> 2;
      out[3] = (in[3] + in[7] + in[width + 3] + in[width + 7]) >> 2;
    }
  }
}

/*
================
GL_MipMap8Bit

Mipping for 8 bit textures
================
*/
void GL_MipMap8Bit(byte *in, int width, int height) {
  int i, j;
  unsigned short r, g, b;
  byte *out, *at1, *at2, *at3, *at4;

  //	width <<=2;
  height >>= 1;
  out = in;
#if 0
	byte color = 0;
	//Mipmapping Debug, fills solid colors at each level
	switch(height){
        case 256:
        color = 254;
        break;
        case 128:
        color = 244;
        break;
        case 64:
        color = 243;
        break;
        case 32:
        color = 144;
        break;
        case 16:
        color = 251;
        break;
        case 8:
        color = 95;
        break;
        case 4:
        color = 15;
        break;
        case 2:
        color = 0;
        break;
        case 1:
        color = 0;
        break;
    }

	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=2, out+=1, in+=2)
		{
			out[0] = color;
		}
	}
#else
  for (i = 0; i < height; i++, in += width) {
    for (j = 0; j < width; j += 2, out += 1, in += 2) {
      at1 = (byte *)(d_8to24table + in[0]);
      at2 = (byte *)(d_8to24table + in[1]);
      at3 = (byte *)(d_8to24table + in[width + 0]);
      at4 = (byte *)(d_8to24table + in[width + 1]);

      r = (at1[0] + at2[0] + at3[0] + at4[0]);
      r >>= 5;
      g = (at1[1] + at2[1] + at3[1] + at4[1]);
      g >>= 5;
      b = (at1[2] + at2[2] + at3[2] + at4[2]);
      b >>= 5;

      out[0] = d_15to8table[(r << 0) + (g << 5) + (b << 10)];
    }
  }
#endif
}

/*
===============
GL_Upload32
===============
*/
void GL_Upload32(unsigned *data, int width, int height, uint8_t flags) {
#ifndef _arch_dreamcast
  int samples;
  int scaled_width, scaled_height;
  qboolean alpha = ((flags & TEX_ALPHA) == TEX_ALPHA);
  qboolean mipmap = ((flags & TEX_MIP) == TEX_MIP);

  for (scaled_width = 1; scaled_width < width; scaled_width <<= 1)
    ;
  for (scaled_height = 1; scaled_height < height; scaled_height <<= 1)
    ;

  scaled_width >>= (int)gl_picmip.value;
  scaled_height >>= (int)gl_picmip.value;

  if (scaled_width > gl_max_size.value)
    scaled_width = gl_max_size.value;
  if (scaled_height > gl_max_size.value)
    scaled_height = gl_max_size.value;

  if (height < 8 || scaled_height < 8) {
    scaled_height = 8;
  }
  if (width < 8 || scaled_width < 8) {
    scaled_width = 8;
  }

  if (scaled_width * scaled_height > (int)sizeof(scaled) / 4)
    Sys_Error("GL_LoadTexture: too big");

  samples = alpha ? gl_alpha_format : gl_solid_format;

#if 0
	if (mipmap)
		gluBuild2DMipmaps (GL_TEXTURE_2D, samples, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else if (scaled_width == width && scaled_height == height)
		glTexImage2D (GL_TEXTURE_2D, 0, samples, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
	{
		gluScaleImage (GL_RGBA, width, height, GL_UNSIGNED_BYTE, data,	scaled_width, scaled_height, GL_UNSIGNED_BYTE, scaled);
		glTexImage2D (GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	}
#else

  if (scaled_width == width && scaled_height == height) {
    if (!mipmap) {
      glTexImage2D(GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, data);
      goto done;
    }
    memcpy(scaled, data, width * height * 4);
  } else
    GL_ResampleTexture(data, width, height, scaled, scaled_width,
                       scaled_height);

  glTexImage2D(GL_TEXTURE_2D, 0, samples, scaled_width, scaled_height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, scaled);
  if (mipmap) {
    int miplevel;

    miplevel = 0;
    while (scaled_width > 1 || scaled_height > 1) {
      GL_MipMap((byte *)scaled, scaled_width, scaled_height);
      scaled_width >>= 1;
      scaled_height >>= 1;
      if (scaled_width < 1)
        scaled_width = 1;
      if (scaled_height < 1)
        scaled_height = 1;
      miplevel++;
      glTexImage2D(GL_TEXTURE_2D, miplevel, samples, scaled_width,
                   scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
    }
    GL_MipMap((byte *)scaled, 1, 1);
    glTexImage2D(GL_TEXTURE_2D, miplevel, samples, 1, 1, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, scaled);
  }
done:;
#endif

  if (mipmap) {
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
  } else {
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
  }
#else
  (void)data;
  (void)width;
  (void)height;
  (void)flags;
#endif
}

static void GL_Upload8_EXT(byte *restrict data, int width, int height,
                           uint8_t flags) {
#ifdef _arch_dreamcast // BlackAura - begin
  qboolean mipmap = ((flags & TEX_MIP) == TEX_MIP);

  int scaled_width, scaled_height;
  if (width != height) {
    mipmap = false;
  }

  for (scaled_width = 1; scaled_width < width; scaled_width <<= 1)
    ;
  for (scaled_height = 1; scaled_height < height; scaled_height <<= 1)
    ;

  if (height < 8 || scaled_height < 8) {
    scaled_height = 8;
  }
  if (width < 8 || scaled_width < 8) {
    scaled_width = 8;
  }

  if (scaled_width > (int)gl_max_size.value)
    scaled_width = gl_max_size.value;
  if (scaled_height > (int)gl_max_size.value)
    scaled_height = gl_max_size.value;

  if (scaled_width == 256)
    scaled_width >>= (int)gl_picmip.value;
  if (scaled_height == 256)
    scaled_height >>= (int)gl_picmip.value;

#if 0 /*def DEBUG*/
  printf("GL_Upload8: %dx%d\t %d bytes\n", scaled_width, scaled_height, scaled_width*scaled_height);
  printf("GL Mem left:%u\n", (unsigned int)glGetFreeVRAM_INTERNAL_KOS());
#endif

  if (!scaled) {
    scaled_raw = buffer_aligned; // Hunk_TempAlloc(256 * 256 + 32);
    scaled = scaled_raw; //(unsigned char*)ALIGN((uint32_t)scaled_raw, 32);
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  (mipmap) ? gl_filter_min : gl_filter_max);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
  if ((scaled_width == width) && (scaled_height == height)) {
    uint32_t ptr_unchecked = (uint32_t)data;
    uint32_t ptr_aligned = ALIGN((uint32_t)data, 32);
    if (ptr_unchecked != ptr_aligned) {
      memcpy(buffer_aligned, data, scaled_width * scaled_height * 1);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width,
                   scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE,
                   buffer_aligned);
    } else {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width,
                   scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, data);
    }
    if (mipmap)
      memcpy(scaled, data, width * height);
    else {
      return;
    }
  } else {
    GL_Resample8BitTexture(data, width, height, scaled, scaled_width,
                           scaled_height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, scaled_width,
                 scaled_height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, scaled);
  }

  if ((scaled_width == scaled_height) && mipmap) {
    // Only using scaled_width from now on;
    // #define PARANOID
    int miplevel = 0;
#ifdef PARANOID
    printf("Begin mip for %dx%d hopefully %d: ", scaled_width, scaled_height,
           log2(scaled_width));
#endif
    while (scaled_width > 1) {
      GL_MipMap8Bit(scaled, scaled_width, scaled_width);
      scaled_width >>= 1;

      glTexImage2D(GL_TEXTURE_2D, ++miplevel, GL_COLOR_INDEX8_EXT, scaled_width,
                   scaled_width, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, scaled);
#ifdef PARANOID
      printf("..%d", miplevel);
#endif
    }
#ifdef PARANOID
    printf("\n");
#endif
#undef PARANOID
  }
  return;
#else
  (void)data;
  (void)width;
  (void)height;
  (void)flags;
#endif // BlackAura
}

/*
===============
GL_Upload8
===============
*/
void GL_Upload8(byte *data, int width, int height, uint8_t flags) {
// BlackAura - begin
#ifdef _arch_dreamcast
  /* Dreamcast is nice and simple - always use 8=bit textures */
  GL_Upload8_EXT(data, width, height, flags);
#else
  /* PCs don't usually have 8-bit textures, so convert to 32-bit */
  qboolean noalpha = true;
  unsigned int *xlated;
  unsigned int *ptr;
  int n;

  /* Work out size and allocate some memory for it */
  // BlackAura - end
  n = width * height; // BlackAura - edited
                      // BlackAura - begin
  xlated = (unsigned int *)malloc(n * sizeof(int));
  if (!xlated)
    Sys_Error("GL_Upload8: Can't change texture format - out of memory");
  ptr = xlated;

  while (n) {
    byte i = *data++;
    // BlackAura - begin
    if (i == 255) // BlackAura - edited
      noalpha = false;
    // BlackAura - begin
    *ptr++ = d_8to24table[i];
    n--;
  }

  /* If we didn't find any transparent pixels, don't upload with an alpha
   * channel */
  if (noalpha)
    flags = flags ^ TEX_ALPHA;

  /* Upload texture, and free the memory we're eating up */
  // BlackAura - end
  GL_Upload32(xlated, width, height, flags);
  // BlackAura - begin
  free(xlated);
#endif
  // BlackAura - end
}

/*
================
GL_LoadTexture
Checksum / memory leak fix thanks to LordHavoc
This will slow load times a bit, but we can't put up with memory leaks
================
*/
static int lhcsumtable[256]; // BlackAura
int GL_LoadTexture(char *identifier, int width, int height, byte *data,
                   uint8_t flags) {
  int i, s, lhcsum; // BlackAura - edited
  gltexture_t *glt;

  // BlackAura - begin
  // LordHavoc: do a checksum to confirm the data really is the same as previous
  // occurances. well this isn't exactly a checksum, it's better than that but
  // not following any standards.
  lhcsum = 0;
  s = width * height;
  for (i = 0; i < 256; i++)
    lhcsumtable[i] = i + 1;
  for (i = 0; i < s; i++)
    lhcsum += (lhcsumtable[data[i] & 255]++);
  // BlackAura - end

  // see if the texture is already present
  if (identifier[0]) {
    for (i = 0, glt = gltextures; i < numgltextures; i++, glt++) {
      if (!strcmp(identifier, glt->identifier)) {
        // BlackAura - begin
        // LordHavoc: everyone hates cache mismatchs, so I fixed it
        if (lhcsum != glt->lhcsum || width != glt->width ||
            height != glt->height) {
          Con_DPrintf(
              "GL_LoadTexture: cache mismatch, replacing old texture\n");
          goto GL_LoadTexture_setup; // drop out with glt pointing to the
                                     // texture to replace
        }
        return glt->texnum;
        // BlackAura - end
      }
    }
  }

  // LordHavoc: this was an else condition, causing disasterous results,
  // whoever at id or threewave must've been half asleep...
  glt = &gltextures[numgltextures];
  numgltextures++;

  int len = 0;
  if (identifier) {
    len = strlen(identifier);
    memcpy(glt->identifier, identifier, len);
  }
  glt->identifier[len] = '\0';

  glGenTextures(1, &glt->texnum);
  // BlackAura - begin

  // LordHavoc: label to drop out of the loop into the setup code
GL_LoadTexture_setup:
  glt->lhcsum = lhcsum; // LordHavoc: used to verify textures are identical
  // BlackAura - end
  glt->width = width;
  glt->height = height;
  glt->flags = flags;

  // BlackAura - begin
  if (!isDedicated) {
    GL_Bind(glt->texnum);
    GL_Upload8(data, width, height, flags);
  }

  return glt->texnum;
  // BlackAura - end
}

/****************************************/

void DrawQuad_NoTex(float x, float y, float w, float h, int c) {
  const uint8_t r = host_basepal[c * 3];
  const uint8_t g = host_basepal[c * 3 + 1];
  const uint8_t b = host_basepal[c * 3 + 2];
  glvert_fast_t quadvert[4];
  // Vertex 1
  // Quad vertex
  // quadvert[0] = (glvert_fast_t){.flags = VERTEX, .vert = {x, y, 0}, .texture
  // = {0, 0}, .color = {b, g, r, 255}, .pad0 = {0}};
  quadvert[0] =
      (glvert_fast_t){.flags = VERTEX,
                      .vert = {x, y, 0},
                      .texture = {0, 0},
                      .color = {.packed = PACK_BGRA8888(b, g, r, 255)},
                      .pad0 = {0}};

  // Vertex 4
  // Quad vertex
  // quadvert[1] = (glvert_fast_t){.flags = VERTEX, .vert = {x, y + h, 0},
  // .texture = {0, 0}, .color = {b, g, r, 255}, .pad0 = {0}};
  quadvert[1] =
      (glvert_fast_t){.flags = VERTEX,
                      .vert = {x, y + h, 0},
                      .texture = {0, 0},
                      .color = {.packed = PACK_BGRA8888(b, g, r, 255)},
                      .pad0 = {0}};

  // Vertex 2
  // Quad vertex
  // quadvert[2] = (glvert_fast_t){.flags = VERTEX, .vert = {x + w, y, 0},
  // .texture = {0, 0}, .color = {b, g, r, 255}, .pad0 = {0}};
  quadvert[2] =
      (glvert_fast_t){.flags = VERTEX,
                      .vert = {x + w, y, 0},
                      .texture = {0, 0},
                      .color = {.packed = PACK_BGRA8888(b, g, r, 255)},
                      .pad0 = {0}};

  // Vertex 3
  // Quad vertex
  // quadvert[3] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {x + w, y + h,
  // 0}, .texture = {0, 0}, .color = {b, g, r, 255}, .pad0 = {0}};
  quadvert[3] =
      (glvert_fast_t){.flags = VERTEX_EOL,
                      .vert = {x + w, y + h, 0},
                      .texture = {0, 0},
                      .color = {.packed = PACK_BGRA8888(b, g, r, 255)},
                      .pad0 = {0}};

  glDisable(GL_TEXTURE_2D);
  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &quadvert[0].vert);
  glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &quadvert[0].texture);
#ifdef WIN98
  glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t),
                 &quadvert[0].color);
#else
  glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t),
                 &quadvert[0].color);
#endif
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glEnable(GL_TEXTURE_2D);
}

void DrawQuad(int x, int y, int w, int h, float u, float v, float uw,
              float vh) {
  glvert_fast_t quadvert[4];

  // Ensure coordinates are hardware aligned
  x = (x + 4) & ~7;
  y = (y + 4) & ~7;

  // Adjust texture coordinates to prevent chopping
  float u2 = u + uw;
  float v2 = v + vh;

  // Triangle strip order: bottom-left, top-left, bottom-right, top-right
  // Vertex 1 (bottom-left)
  quadvert[0] = (glvert_fast_t){
      .flags = VERTEX,
      .vert = {(float)x, (float)(y + h), 0},
      .texture = {u, v2},
      .color = {.packed = PACK_BGRA8888(255, 255, 255, quad_alpha)},
      .pad0 = {0}};

  // Vertex 2 (top-left)
  quadvert[1] = (glvert_fast_t){
      .flags = VERTEX,
      .vert = {(float)x, (float)y, 0},
      .texture = {u, v},
      .color = {.packed = PACK_BGRA8888(255, 255, 255, quad_alpha)},
      .pad0 = {0}};

  // Vertex 3 (bottom-right)
  quadvert[2] = (glvert_fast_t){
      .flags = VERTEX,
      .vert = {(float)(x + w), (float)(y + h), 0},
      .texture = {u2, v2},
      .color = {.packed = PACK_BGRA8888(255, 255, 255, quad_alpha)},
      .pad0 = {0}};

  // Vertex 4 (top-right)
  quadvert[3] = (glvert_fast_t){
      .flags = VERTEX_EOL,
      .vert = {(float)(x + w), (float)y, 0},
      .texture = {u2, v},
      .color = {.packed = PACK_BGRA8888(255, 255, 255, quad_alpha)},
      .pad0 = {0}};

  glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &quadvert[0].vert);
  glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &quadvert[0].texture);
#ifdef WIN98
  glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t),
                 &quadvert[0].color);
#else
  glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t),
                 &quadvert[0].color);
#endif
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
