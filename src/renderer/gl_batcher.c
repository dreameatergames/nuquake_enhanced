// gl_batcher.c: handles creating little batches of polys

#include "quakedef.h"
#ifdef _arch_dreamcast
#ifdef BUILD_LIBGL
#include <glext.h>
#else
#include <GL/glext.h>
#endif
#endif

glvert_fast_t r_batchedfastvertexes[MAX_BATCHED_SURFVERTEXES];
glvert_fast_t r_batchedfastvertexes_text[MAX_BATCHED_SURFVERTEXES * 2];

int text_size = 8;

unsigned int r_numsurfvertexes = 0;
unsigned int r_numsurfvertexes_text = 0;

extern inline void R_BeginBatchingFastSurfaces();
extern inline void R_BeginBatchingSurfacesQuad();

void R_EndBatchingFastSurfaces(void)
{
   if (r_numsurfvertexes >= 3)
   {
      glEnableClientState(GL_COLOR_ARRAY);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &r_batchedfastvertexes[0].vert);
      glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &r_batchedfastvertexes[0].texture);
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &r_batchedfastvertexes[0].color);
      glDrawArrays(GL_TRIANGLES, 0, r_numsurfvertexes);
      r_numsurfvertexes = 0;
   }
}

void R_EndBatchingSurfacesQuads(void)
{
   if (r_numsurfvertexes_text)
   {
      glEnable(GL_BLEND);
      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &r_batchedfastvertexes_text[0].vert);
      glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &r_batchedfastvertexes_text[0].texture);
      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &r_batchedfastvertexes_text[0].color);
      glDrawArrays(GL_TRIANGLES, 0, r_numsurfvertexes_text);
      glDisable(GL_BLEND);
      r_numsurfvertexes_text = 0;
   }
}

extern inline void R_BatchSurfaceQuadText(int x, int y, float frow, float fcol, float size);

void R_BatchSurface(glpoly_t *p)
{
   const int tris = p->numverts - 2;
   if (r_numsurfvertexes + (tris * 3) >= MAX_BATCHED_SURFVERTEXES)
   {
      R_EndBatchingFastSurfaces();
   }

   int i;
   const float *v0 = p->verts[0];
   float *v = p->verts[1];
   for (i = 0; i < tris; i++)
   {
      r_batchedfastvertexes[r_numsurfvertexes++] = (glvert_fast_t){.flags = VERTEX, .vert = {v0[0], v0[1], v0[2]}, .texture = {v0[3], v0[4]}, .color = {255, 255, 255, 255}, .pad0 = {0}};
      r_batchedfastvertexes[r_numsurfvertexes++] = (glvert_fast_t){.flags = VERTEX, .vert = {v[0], v[1], v[2]}, .texture = {v[3], v[4]}, .color = {255, 255, 255, 255}, .pad0 = {0}};
      v += VERTEXSIZE;
      r_batchedfastvertexes[r_numsurfvertexes++] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {v[0], v[1], v[2]}, .texture = {v[3], v[4]}, .color = {255, 255, 255, 255}, .pad0 = {0}};
   }
}

void R_BatchSurfaceLightmap(glpoly_t *p)
{
   const int tris = p->numverts - 2;
   if (r_numsurfvertexes + (tris * 3) >= MAX_BATCHED_SURFVERTEXES)
   {
      R_EndBatchingFastSurfaces();
   }

   int i;
   const float *v0 = p->verts[0];
   float *v = p->verts[1];
   for (i = 0; i < tris; i++)
   {
      r_batchedfastvertexes[r_numsurfvertexes++] = (glvert_fast_t){.flags = VERTEX, .vert = {v0[0], v0[1], v0[2]}, .texture = {v0[5], v0[6]}, .color = {255, 255, 255, 255}, .pad0 = {0}};
      r_batchedfastvertexes[r_numsurfvertexes++] = (glvert_fast_t){.flags = VERTEX, .vert = {v[0], v[1], v[2]}, .texture = {v[5], v[6]}, .color = {255, 255, 255, 255}, .pad0 = {0}};
      v += VERTEXSIZE;
      r_batchedfastvertexes[r_numsurfvertexes++] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {v[0], v[1], v[2]}, .texture = {v[5], v[6]}, .color = {255, 255, 255, 255}, .pad0 = {0}};
   }
}
