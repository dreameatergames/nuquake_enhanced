/*
 * File: gl_batcher.h
 * Project: renderer
 * File Created: Saturday, 23rd March 2019 5:42:06 pm
 * Author: Hayden Kowalchuk (hayden@hkowsoftware.com)
 * -----
 * Copyright (c) 2019 Hayden Kowalchuk
 */

#ifndef __GL_BATCHER__
#define __GL_BATCHER__

#include <stdint.h>
#include "glquake.h"

// these could go in a vertexbuffer/indexbuffer pair
#define MAX_BATCHED_TEMPVERTEXES (2048)
#define MAX_BATCHED_SURFVERTEXES (2048)
#define MAX_BATCHED_TEXTVERTEXES (4096)

#define VERTEX_EOL 0xf0000000
#define VERTEX 0xe0000000

extern glvert_fast_t r_batchedtempverts[MAX_BATCHED_TEMPVERTEXES];
extern glvert_fast_t r_batchedfastvertexes[MAX_BATCHED_SURFVERTEXES];
extern glvert_fast_t r_batchedfastvertexes_text[MAX_BATCHED_TEXTVERTEXES];

extern int text_size;

void R_BeginBatchingFastSurfaces();
void R_BeginBatchingSurfacesQuad();

void R_EndBatchingSurfacesQuads(void);
void R_EndBatchingFastSurfaces(void);

void R_BatchSurface(glpoly_t *p);
void R_BatchSurfaceLightmap(glpoly_t *p);
void R_BatchSurfaceQuadText(int x, int y, float frow, float fcol, float size);

extern glvert_fast_t *_tempBufferAddress;

static inline glvert_fast_t *R_GetDirectBufferAddress() {
  return _tempBufferAddress;
}

static inline void R_SetDirectBufferAddress(glvert_fast_t *address) {
  _tempBufferAddress = address;
}

#endif /* __GL_BATCHER__ */