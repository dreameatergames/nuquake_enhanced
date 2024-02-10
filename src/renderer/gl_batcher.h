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
#define MAX_BATCHED_SURFVERTEXES 4096

#define VERTEX_EOL 0xf0000000
#define VERTEX 0xe0000000

extern glvert_fast_t r_batchedfastvertexes[];
extern glvert_fast_t r_batchedfastvertexes_text[];

extern int text_size;

extern unsigned int r_numsurfvertexes;
extern unsigned int r_numsurfvertexes_text;

inline void R_BeginBatchingFastSurfaces()
{
   r_numsurfvertexes = 0;
}

inline void R_BeginBatchingSurfacesQuad()
{
   r_numsurfvertexes_text = 0;
}

void R_EndBatchingSurfacesQuads(void);
void R_EndBatchingFastSurfaces(void);

void R_BatchSurface(glpoly_t *p);
void R_BatchSurfaceLightmap(glpoly_t *p);

inline void R_BatchSurfaceQuadText(int x, int y, float frow, float fcol, float size)
{

   if (r_numsurfvertexes_text + 6 >= MAX_BATCHED_SURFVERTEXES*2)
      R_EndBatchingSurfacesQuads();

   //Vertex 1
   //Quad vertex
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (glvert_fast_t){.flags = VERTEX, .vert = {x, y, 0}, .texture = {fcol, frow}, .color = {255, 255, 255, 255}, .pad0 = {0}};

   //Vertex 2
   //Quad vertex
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (glvert_fast_t){.flags = VERTEX, .vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {255, 255, 255, 255}, .pad0 = {0}};

   //Vertex 4
   //Quad vertex
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {255, 255, 255, 255}, .pad0 = {0}};

   //Vertex 4
   //Quad vertex
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (glvert_fast_t){.flags = VERTEX, .vert = {x, y + text_size, 0}, .texture = {fcol, frow + size}, .color = {255, 255, 255, 255}, .pad0 = {0}};

   //Vertex 2
   //Quad vertex
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (glvert_fast_t){.flags = VERTEX, .vert = {x + text_size, y, 0}, .texture = {fcol + size, frow}, .color = {255, 255, 255, 255}, .pad0 = {0}};

   //Vertex 3
   //Quad vertex
   r_batchedfastvertexes_text[r_numsurfvertexes_text++] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {x + text_size, y + text_size, 0}, .texture = {fcol + size, frow + size}, .color = {255, 255, 255, 255}, .pad0 = {0}};
}

#endif /* __GL_BATCHER__ */