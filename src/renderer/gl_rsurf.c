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
// r_surf.c: surface-related refresh code

#include "quakedef.h"
#include "glquake.h"
#include "gl_local.h"

int			skytexturenum;

unsigned blocklights[18*18];
#ifdef _arch_dreamcast
#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128
#else
#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128
#endif

int			active_lightmaps;

int		lightmap_bytes;		// 1, 2, or 4
int		lightmap_filter = GL_LINEAR;
unsigned int		lightmap_textures[MAX_LIGHTMAPS] = {0};

typedef struct glRect_s {
	unsigned char l,t,w,h;
} glRect_t;

glpoly_t	*lightmap_polys[MAX_LIGHTMAPS];
qboolean	lightmap_modified[MAX_LIGHTMAPS];
glRect_t	lightmap_rectchange[MAX_LIGHTMAPS];

int			allocated[MAX_LIGHTMAPS][BLOCK_WIDTH];

// the lightmap texture data needs to be kept in
// main memory so texsubimage can update properly
byte		lightmaps[1*MAX_LIGHTMAPS*BLOCK_WIDTH*BLOCK_HEIGHT];

void R_RenderDynamicLightmaps (msurface_t *fa);

#define _PACK4(v) (((unsigned char)v * 0xF) / 0xFF)
#define PACK_ARGB4444(a,r,g,b) (unsigned int)(_PACK4(a) << 12) | (_PACK4(r) << 8) | (_PACK4(g) << 4) | (_PACK4(b))


/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (msurface_t *surf)
{
	int			lnum;
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	int			s, t;
	int			i;
	int			smax, tmax;
	mtexinfo_t	*tex;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;

	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
		if ( !(surf->dlightbits & (1<<lnum) ) )
			continue;		// not lit by this light

		rad = cl_dlights[lnum].radius;
		dist = DotProduct (cl_dlights[lnum].origin, surf->plane->normal) -
				surf->plane->dist;
		rad -= FABS(dist);
		minlight = cl_dlights[lnum].minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = cl_dlights[lnum].origin[i] -
					surf->plane->normal[i]*dist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];

		for (t = 0 ; t<tmax ; t++)
		{
			td = local[1] - t*16;
			if (td < 0)
				td = -td;
			for (s=0 ; s<smax ; s++)
			{
				sd = local[0] - s*16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td>>1);
				else
					dist = td + (sd>>1);
				if (dist < minlight)
					blocklights[t*smax + s] += (rad - dist)*256;
			}
		}
	}
}


#define TWIDTAB(x) ( (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)| \
	((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9) )
#define TWIDOUT(x, y) ( TWIDTAB((y)) | (TWIDTAB((x)) << 1) )

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildLightMap (msurface_t *surf, byte * restrict dest, int stride)
{
	int			smax, tmax;
	int			t;
	int			i, j, size;
	byte		* restrict lightmap;
	unsigned	scale;
	int			maps;
	unsigned	*restrict bl;

	surf->cached_dlight = (surf->dlightframe == r_framecount);

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	lightmap = surf->samples;

// set to full bright if no light data
	if (__builtin_expect ((r_fullbright.value || !cl.worldmodel->lightdata), 0))
	{
		for (i=0 ; i<size ; i++)
			blocklights[i] = 255*256;
		goto store;
	}

// clear to no light
	memset(blocklights, '\0', sizeof(blocklights));

// add all the lightmaps
	if (lightmap)
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ; maps++)
		{
			scale = d_lightstylevalue[surf->styles[maps]];
			surf->cached_light[maps] = scale;	// 8.8 fraction
			for (i=0 ; i<size ; i++)
				blocklights[i] += lightmap[i] * scale;
			lightmap += size;	// skip to next lightmap
		}

// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights (surf);

// bound, invert, and shift
store:
	switch (gl_lightmap_format)
	{
	case GL_RGBA:
		stride -= (smax<<2);
		bl = blocklights;
		for (i=0 ; i<tmax ; i++, dest += stride)
		{
			for (j=0 ; j<smax ; j++)
			{
				t = *bl++;
				t >>= 7;
				if (t > 255)
					t = 255;
				dest[3] = 255-t;
				dest += 4;
			}
		}
		break;
	case GL_RGBA4:
		bl = blocklights;
		for (i=0 ; i<tmax ; i++, dest += stride)
		{
			for (j=0 ; j<smax ; j++)
			{
				t = *bl++;
				t >>= 7;
				if (t > 255)
					t = 255;
				dest[j] = PACK_ARGB4444(255-t,255,255,255);
			}
		}
		break;
	case GL_ALPHA:
	case GL_LUMINANCE:
	case GL_INTENSITY:
	case GL_COLOR_INDEX8_EXT:{
		/*
		int bl_iter = 0;
		const int _255 = 255;
		*/
		bl = blocklights;
#if 0
		int min = MIN(smax, tmax);
		int mask = min - 1;
		uint16_t* restrict vtex = (uint16_t*)dest;
		int x,y;
		for (y=0; y<tmax; y++) {
			for (x=0; x<smax; x++) {
				const int val1 = bl[bl_iter] >> 7;
				const int val1_i = 255 - (val1 ^ ((_255 ^ val1) & -(_255 < val1))); // min(_255, val)
				const int val2 = bl[bl_iter+smax] >> 7;
				const int val2_i = 255 - (val2 ^ ((_255 ^ val2) & -(_255 < val2))); // min(_255, val)
				bl_iter++;
				vtex[TWIDOUT((y&mask)/2, x&mask) + (x/min + y/min)*min*min/2] = val1_i | (val2_i << 8);//pixels[y*w+x] | (pixels[(y+1)*w+x]<<8);
			}
		}
#else
		for (i=0 ; i<tmax ; i++, dest += stride)
		{
			for (j=0 ; j<smax ; j++)
			{
				/*
				const int val = bl[bl_iter] >> 7;
				const int val_i = 255 - (val ^ ((_255 ^ val) & -(_255 < val))); // min(_255, val)
				dest[j] = val_i;
				bl_iter++;*/
				t = *bl++;
				t >>= 7;
				if (t > 255)
					t = 255;
				dest[j] = 255-t;
			}
		}
#endif
	}
		break;
	default:
		Sys_Error ("Bad lightmap format");
	}
}


/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation (texture_t *base)
{
	int		reletive;
	int		count;

	if (currententity->frame)
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}

	if (!base->anim_total)
		return base;

	reletive = (int)(cl.time*10) % base->anim_total;

	count = 0;
	while (base->anim_min > reletive || base->anim_max <= reletive)
	{
		base = base->anim_next;
		if (!base)
			Sys_Error ("R_TextureAnimation: broken cycle");
		if (++count > 100)
			Sys_Error ("R_TextureAnimation: infinite cycle");
	}

	return base;
}


/*
=============================================================

	BRUSH MODELS

=============================================================
*/


extern	unsigned int		solidskytexture;
extern	unsigned int		alphaskytexture;
extern	float	speedscale;		// for top sky and bottom sky

void DrawGLWaterPoly (glpoly_t *p);

#ifndef _arch_dreamcast
static qboolean mtexenabled = false;
lpMTexFUNC qglMTexCoord2fSGIS = NULL;
lpSelTexFUNC qglSelectTextureSGIS = NULL;

GLenum gl_oldtarget;
GLenum gl_Texture0;
GLenum gl_Texture1;

void GL_SelectTexture(GLenum target) {
  if (!gl_mtexable)
    return;
  qglSelectTextureSGIS(target);
  if (target == gl_oldtarget)
    return;
  cnttextures[gl_oldtarget - TEXTURE0_SGIS] = currenttexture;
  currenttexture = cnttextures[target - TEXTURE0_SGIS];
  gl_oldtarget = target;
}

void GL_DisableMultitexture(void)
{
	if (mtexenabled) {
		glDisable(GL_TEXTURE_2D);
		GL_SelectTexture(TEXTURE0_SGIS);
		mtexenabled = false;
	}
}

void GL_EnableMultitexture(void)
{
	if (gl_mtexable) {
		GL_SelectTexture(TEXTURE1_SGIS);
		glEnable(GL_TEXTURE_2D);
		mtexenabled = true;
	}
}
#else
void GL_DisableMultitexture(void) {}
void GL_EnableMultitexture(void) {}
void GL_SelectTexture(GLenum target) { (void)target; }
#endif

/*
================
DrawGLWaterPoly

Warp the vertex coordinates
================
*/
void DrawGLWaterPoly (glpoly_t *p)
{
	int		i;
	GL_DisableMultitexture();
	extern int numwaterverts;

	const float*restrict v0 = p->verts[0];
	const float v0_x = v0[0];
	const float v0_y = v0[1];
	const float v0_z = v0[2];
	const float v0_u = v0[3];
	const float v0_v = v0[4];

	float* restrict v1 = p->verts[1];
	float* restrict v2 = p->verts[1] + VERTEXSIZE;

	const float pnv_x = v0_x + 8*SIN(v0_y*0.05f+realtime)*SIN(v0_z*0.05f+realtime);
	const float pnv_y = v0_y + 8*SIN(v0_x*0.05f+realtime)*SIN(v0_z*0.05f+realtime);
	const float pnv_z = v0_z;

  for (i = 0; i < p->numverts - 2; i++) {
		/* 2nd vertex */
		const float v1_x = v1[0];
		const float v1_y = v1[1];
		const float v1_z = v1[2];
		const float v1_u = v1[3];
		const float v1_v = v1[4];

		const float tv1_x = v1_x + 8*SIN(v1_y*0.05f+realtime)*SIN(v1_z*0.05f+realtime);
		const float tv1_y = v1_y + 8*SIN(v1_x*0.05f+realtime)*SIN(v1_z*0.05f+realtime);

		/* 3rd vertex */
		const float v2_x = v2[0];
		const float v2_y = v2[1];
		const float v2_z = v2[2];
		const float v2_u = v2[3];
		const float v2_v = v2[4];

		const float tv2_x = v2_x + 8*SIN(v2_y*0.05f+realtime)*SIN(v2_z*0.05f+realtime);
		const float tv2_y = v2_y + 8*SIN(v2_x*0.05f+realtime)*SIN(v2_z*0.05f+realtime);

		r_batchedtempverts[numwaterverts++] = (glvert_fast_t){.flags = VERTEX, .vert = {pnv_x, pnv_y, pnv_z}, .texture = {v0_u, v0_v}, VTX_COLOR_WHITE, .pad0 = {0}};
		r_batchedtempverts[numwaterverts++] = (glvert_fast_t){.flags = VERTEX, .vert = {tv1_x, tv1_y, v1_z}, .texture = {v1_u, v1_v}, VTX_COLOR_WHITE, .pad0 = {0}};
		r_batchedtempverts[numwaterverts++] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {tv2_x, tv2_y, v2_z}, .texture = {v2_u, v2_v}, VTX_COLOR_WHITE, .pad0 = {0}};

		v1 += VERTEXSIZE;
		v2 += VERTEXSIZE;
  }
}

void DrawGLWaterPolyLightmap (glpoly_t *p)
{
	int		i;
	float	*v;
	GL_DisableMultitexture();
	glvert_fast_t* submission_pointer = R_GetDirectBufferAddress();

	const float *v0 = p->verts[0];
	float pnv[3];
	float tv0, tv1;
  v = p->verts[1];

	pnv[0] = v0[0] + 8*SIN(v0[1]*0.05f+realtime)*SIN(v0[2]*0.05f+realtime);
	pnv[1] = v0[1] + 8*SIN(v0[0]*0.05f+realtime)*SIN(v0[2]*0.05f+realtime);
	pnv[2] = v0[2];

    for (i = 0; i < p->numverts - 2; i++)
    {
		tv0 = v[0] + 8*SIN(v[1]*0.05f+realtime)*SIN(v[2]*0.05f+realtime);
		tv1 = v[1] + 8*SIN(v[0]*0.05f+realtime)*SIN(v[2]*0.05f+realtime);

		*submission_pointer++ = (glvert_fast_t){.flags = VERTEX, .vert = {pnv[0], pnv[1], pnv[2]}, .texture = {v0[5], v0[6]}, VTX_COLOR_WHITE, .pad0 = {0}};
		*submission_pointer++ = (glvert_fast_t){.flags = VERTEX, .vert = {tv0, tv1, v[2]}, .texture = {v[5], v[6]}, VTX_COLOR_WHITE, .pad0 = {0}};
		v += VERTEXSIZE;

		tv0 = v[0] + 8*SIN(v[1]*0.05f+realtime)*SIN(v[2]*0.05f+realtime);
		tv1 = v[1] + 8*SIN(v[0]*0.05f+realtime)*SIN(v[2]*0.05f+realtime);
		*submission_pointer++ = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {tv0, tv1, v[2]}, .texture = {v[5], v[6]}, VTX_COLOR_WHITE, .pad0 = {0}};
    }
}

/*
================
R_BlendLightmaps
================
*/
void R_BlendLightmaps (void)
{
	int			i;
	glpoly_t	*p;
	glpoly_t	*p_temp;
	glRect_t	*theRect;

	if (r_fullbright.value)
		return;

	glDepthMask (GL_FALSE);		// don't bother writing Z

#ifndef _arch_dreamcast
	if (gl_lightmap_format == GL_LUMINANCE)
		glBlendFunc (GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	else if (gl_lightmap_format == GL_INTENSITY)
	{
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glColor4f (0,0,0,1);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
#endif

	if (!r_lightmap.value)
	{
		glEnable (GL_BLEND);
	}

	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		p = lightmap_polys[i];
		p_temp = lightmap_polys[i];
		if (!p)
			continue;
		GL_Bind(lightmap_textures[i]);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, lightmap_filter);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, lightmap_filter);
		#ifdef _arch_dreamcast
		glTexParameteri(GL_TEXTURE_2D, GL_SHARED_TEXTURE_BANK_KOS, 1);
		#endif
		if (lightmap_modified[i])
		{
			lightmap_modified[i] = false;
			theRect = &lightmap_rectchange[i];
			theRect->l = 0;
			theRect->t = 0;
			theRect->w = BLOCK_WIDTH;
			theRect->h = BLOCK_HEIGHT;
			#ifdef _arch_dreamcast
			printf("lightmap upload");
				glTexImage2D (GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT
				, BLOCK_WIDTH, theRect->h, 0,
				GL_COLOR_INDEX, GL_UNSIGNED_BYTE, lightmaps+(i*BLOCK_HEIGHT+theRect->t)*BLOCK_WIDTH*lightmap_bytes);
			#else
				glTexImage2D (GL_TEXTURE_2D, 0, lightmap_bytes
					, BLOCK_WIDTH, theRect->h, 0,
					gl_lightmap_format, GL_UNSIGNED_BYTE, lightmaps+(i*BLOCK_HEIGHT+theRect->t)*BLOCK_WIDTH*lightmap_bytes);
			#endif
			theRect->l = BLOCK_WIDTH;
			theRect->t = BLOCK_HEIGHT;
			theRect->h = 0;
			theRect->w = 0;
		}

		/* Count needed verts*/
		int num_lightmap_verts = 0;
		for ( ; p_temp ; p_temp=p_temp->chain) {
			num_lightmap_verts += (p_temp->numverts - 2)*3;
		}

		glvert_fast_t* submission_pointer = &r_batchedtempverts[0];
	  #ifdef GL_EXT_dreamcast_direct_buffer
	  glEnable(GL_DIRECT_BUFFER_KOS);
	  glDirectBufferReserve_INTERNAL_KOS(num_lightmap_verts, (int *)&submission_pointer, GL_TRIANGLES);
	  #endif
		R_SetDirectBufferAddress(submission_pointer);

	  glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->vert);
	  glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->texture);
#ifdef WIN98
	  glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#else
	  glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#endif

		for ( ; p ; p=p->chain)
		{
			/* Water Warping */
			if (__builtin_expect(p->flags & SURF_UNDERWATER, 0)) {
				DrawGLWaterPolyLightmap (p);
			} else {
				R_BatchSurfaceLightmap(p);
			}
			R_SetDirectBufferAddress(R_GetDirectBufferAddress()+((p->numverts - 2)*3));
		}

		glDrawArrays(GL_TRIANGLES, 0, num_lightmap_verts);

	  #ifdef GL_EXT_dreamcast_direct_buffer
	  glDisable(GL_DIRECT_BUFFER_KOS);
	  #endif
	}

	glDisable (GL_BLEND);

#ifndef _arch_dreamcast
	if (gl_lightmap_format == GL_LUMINANCE)
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	else if (gl_lightmap_format == GL_INTENSITY)
	{
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glColor4f (1,1,1,1);
	}
#endif

	glDepthMask (GL_TRUE);		// backto normal Z buffering
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly (msurface_t *fa)
{
	texture_t	*t;
	byte		*base;
	int			maps;
	glRect_t    *theRect;
	int smax, tmax;
	extern cvar_t mipbias;

	c_brush_polys++;

	t = R_TextureAnimation (fa->texinfo->texture);
	if(currenttexture != t->gl_texturenum){
		R_EndBatchingFastSurfaces();
		GL_Bind (t->gl_texturenum);
	}

	#if defined(GL_TEXTURE_FILTER_CONTROL_EXT)
	/* @Hack : Disable Mipmap plane control if support not present */
	glTexEnvi(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, mipbias.value);
	#endif

	if (fa->flags & SURF_DRAWTURB)
	{	// warp texture, no lightmaps
		/* Note: Controls Drawing of Water and Portals */

		int numwaterverts2 = 0;
		glpoly_t	*p;
		for (p=fa->polys ; p ; p=p->next)
		{
			numwaterverts2 += (p->numverts - 2)*3;
		}

    glvert_fast_t* submission_pointer = &r_batchedtempverts[0];

    #ifdef GL_EXT_dreamcast_direct_buffer
    glEnable(GL_DIRECT_BUFFER_KOS);
    glDirectBufferReserve_INTERNAL_KOS(numwaterverts2, (int *)&submission_pointer, GL_TRIANGLES);
    #endif

		R_SetDirectBufferAddress(submission_pointer);

		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->vert);
		glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->texture);
#ifdef WIN98
	  glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#else
	  glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#endif

		extern int numwaterverts;
		numwaterverts = 0;

		EmitWaterPolys (fa);
		glDrawArrays(GL_TRIANGLES, 0, numwaterverts2);

    #ifdef GL_EXT_dreamcast_direct_buffer
    glDisable(GL_DIRECT_BUFFER_KOS);
    #endif

		#ifdef PARANOID
    if(numwaterverts2 > 51)
    	printf("%s:%d drew: %d\n", __FILE__, __LINE__, numwaterverts2);
		#endif
		return;
	}
	/* water warping */
	if (fa->flags & SURF_UNDERWATER)
	{
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &r_batchedtempverts[0].vert);
		glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &r_batchedtempverts[0].texture);
#ifdef WIN98
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &r_batchedtempverts[0].color);
#else
		glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &r_batchedtempverts[0].color);
#endif
		extern int numwaterverts;
		numwaterverts = 0;

		DrawGLWaterPoly (fa->polys);
		glDrawArrays(GL_TRIANGLES, 0, numwaterverts);
	} else {
		R_BatchSurface (fa->polys);
	}

	// add the poly to the proper lightmap chain

	fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
	lightmap_polys[fa->lightmaptexturenum] = fa->polys;

	// check for lightmap modification
	for (maps = 0 ; maps < MAXLIGHTMAPS && fa->styles[maps] != 255 ;
		 maps++)
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
			goto dynamic;

	if (fa->dlightframe == r_framecount	// dynamic this frame
		|| fa->cached_dlight)			// dynamic previously
	{
dynamic:
		if (r_dynamic.value)
		{
			lightmap_modified[fa->lightmaptexturenum] = true;
			theRect = &lightmap_rectchange[fa->lightmaptexturenum];
			if (fa->light_t < theRect->t) {
				if (theRect->h)
					theRect->h += theRect->t - fa->light_t;
				theRect->t = fa->light_t;
			}
			if (fa->light_s < theRect->l) {
				if (theRect->w)
					theRect->w += theRect->l - fa->light_s;
				theRect->l = fa->light_s;
			}
			smax = (fa->extents[0]>>4)+1;
			tmax = (fa->extents[1]>>4)+1;
			if ((theRect->w + theRect->l) < (fa->light_s + smax))
				theRect->w = (fa->light_s-theRect->l)+smax;
			if ((theRect->h + theRect->t) < (fa->light_t + tmax))
				theRect->h = (fa->light_t-theRect->t)+tmax;
			base = lightmaps + fa->lightmaptexturenum*lightmap_bytes*BLOCK_WIDTH*BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * lightmap_bytes + fa->light_s * lightmap_bytes;
			R_BuildLightMap (fa, base, BLOCK_WIDTH*lightmap_bytes);
		}
	}
}

/*
================
R_RenderDynamicLightmaps
Multitexture
================
*/
void R_RenderDynamicLightmaps (msurface_t *fa)
{
	byte		*base;
	int			maps;
	glRect_t    *theRect;
	int smax, tmax;

	c_brush_polys++;

	if (fa->flags & ( SURF_DRAWSKY | SURF_DRAWTURB) )
		return;

	fa->polys->chain = lightmap_polys[fa->lightmaptexturenum];
	lightmap_polys[fa->lightmaptexturenum] = fa->polys;

	// check for lightmap modification
	for (maps = 0 ; maps < MAXLIGHTMAPS && fa->styles[maps] != 255 ;
		 maps++)
		if (d_lightstylevalue[fa->styles[maps]] != fa->cached_light[maps])
			goto dynamic;

	if (fa->dlightframe == r_framecount	// dynamic this frame
		|| fa->cached_dlight)			// dynamic previously
	{
dynamic:
		if (r_dynamic.value)
		{
			lightmap_modified[fa->lightmaptexturenum] = true;
			theRect = &lightmap_rectchange[fa->lightmaptexturenum];
			if (fa->light_t < theRect->t) {
				if (theRect->h)
					theRect->h += theRect->t - fa->light_t;
				theRect->t = fa->light_t;
			}
			if (fa->light_s < theRect->l) {
				if (theRect->w)
					theRect->w += theRect->l - fa->light_s;
				theRect->l = fa->light_s;
			}
			smax = (fa->extents[0]>>4)+1;
			tmax = (fa->extents[1]>>4)+1;
			if ((theRect->w + theRect->l) < (fa->light_s + smax))
				theRect->w = (fa->light_s-theRect->l)+smax;
			if ((theRect->h + theRect->t) < (fa->light_t + tmax))
				theRect->h = (fa->light_t-theRect->t)+tmax;
			base = lightmaps + fa->lightmaptexturenum*lightmap_bytes*BLOCK_WIDTH*BLOCK_HEIGHT;
			base += fa->light_t * BLOCK_WIDTH * lightmap_bytes + fa->light_s * lightmap_bytes;
			R_BuildLightMap (fa, base, BLOCK_WIDTH*lightmap_bytes);
		}
	}
}

/*
================
R_DrawWaterSurfaces
================
*/
void R_DrawWaterSurfaces (void)
{
	int			i;
	msurface_t	*s;
	texture_t	*t;

	if (r_wateralpha.value == 1.0)
		return;

	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];
		if (!t)
			continue;
		s = t->texturechain;
		if (!s)
			continue;
		if ( !(s->flags & SURF_DRAWTURB ) )
			continue;

		// set modulate mode explicitly
		glEnable (GL_BLEND);
		glDepthMask(GL_TRUE);
		glEnable(GL_TEXTURE_2D);
		GL_Bind (t->gl_texturenum);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glCullFace(GL_FRONT);
		glvert_fast_t* submission_pointer = &r_batchedtempverts[0];

		//R_SetDirectBufferAddress(submission_pointer);

		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->vert);
    glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &submission_pointer->texture);
#ifdef WIN98
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#else
    glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &submission_pointer->color);
#endif

		extern int numwaterverts;
		numwaterverts = 0;

		for ( ; s ; s=s->texturechain)
			EmitWaterPolys_Temp (s);

		glDrawArrays(GL_TRIANGLES, 0, numwaterverts);

		t->texturechain = NULL;
	}

	if (r_wateralpha.value < 1.0) {
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		//glColor4f (1,1,1,1);
		glDisable (GL_BLEND);
	}

}


/*
================
DrawTextureChains
================
*/
void DrawTextureChains (void)
{
	int		i;
	msurface_t	*s;
	texture_t	*t;

	for (i=0 ; i<cl.worldmodel->numtextures ; i++)
	{
		t = cl.worldmodel->textures[i];
		if (!t)
			continue;
		s = t->texturechain;
		if (!s)
			continue;
		if (i == skytexturenum)
			R_DrawSkyChain (s);
		else
		{
			if ((s->flags & SURF_DRAWTURB) && r_wateralpha.value != 1.0)
				continue;	// draw translucent water later

			R_BeginBatchingFastSurfaces();
			for ( ; s ; s=s->texturechain){
				R_RenderBrushPoly (s);
			}
			R_EndBatchingFastSurfaces();
		}

		t->texturechain = NULL;
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (entity_t *e)
{
	int			i, k;
	vec3_t		mins, maxs;
	msurface_t	*psurf;
	float		dot;
	mplane_t	*pplane;
	model_t		*clmodel;
	qboolean	rotated;

	currententity = e;
	currenttexture = -1;

	clmodel = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = e->origin[i] - clmodel->radius;
			maxs[i] = e->origin[i] + clmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (e->origin, clmodel->mins, mins);
		VectorAdd (e->origin, clmodel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs))
		return;

	memset (lightmap_polys, 0, sizeof(lightmap_polys));

	VectorSubtract (r_refdef.vieworg, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

	psurf = &clmodel->surfaces[clmodel->firstmodelsurface];

// calculate dynamic lighting for bmodel if it's not an
// instanced model
	if (clmodel->firstmodelsurface != 0 && !gl_flashblend.value)
	{
		for (k=0 ; k<MAX_DLIGHTS ; k++)
		{
			if ((cl_dlights[k].die < cl.time) ||
				(!cl_dlights[k].radius))
				continue;

			R_MarkLights (&cl_dlights[k], 1<<k,
				clmodel->nodes + clmodel->hulls[0].firstclipnode);
		}
	}

  glPushMatrix ();
	e->angles[0] = -e->angles[0];	// stupid quake bug
	R_RotateForEntity (e);
	e->angles[0] = -e->angles[0];	// stupid quake bug

	//
	// draw texture
	//
	for (i=0 ; i<clmodel->nummodelsurfaces ; i++, psurf++)
	{
	// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

	// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
				/*@Debug: disable for direct render testing */
				R_BeginBatchingFastSurfaces();
				R_RenderBrushPoly (psurf);
				R_EndBatchingFastSurfaces();
		}
	}

	R_BlendLightmaps ();

	glPopMatrix ();
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode (mnode_t *node)
{
	int			c, side;
	mplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	float		dot;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;
	if (R_CullBox (node->minmaxs, node->minmaxs+3))
		return;

// if a leaf node, draw stuff
	if (node->contents < 0)
	{
		pleaf = (mleaf_t *)node;

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

	// deal with model fragments in this leaf
		if (pleaf->efrags)
			R_StoreEfrags (&pleaf->efrags);

		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct (modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
		side = 0;
	else
		side = 1;

// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side]);

// draw stuff
	c = node->numsurfaces;

	if (c)
	{
		surf = cl.worldmodel->surfaces + node->firstsurface;

		if (dot < 0 -BACKFACE_EPSILON)
			side = SURF_PLANEBACK;
		else if (dot > BACKFACE_EPSILON)
			side = 0;
		{
			for ( ; c ; c--, surf++)
			{
				if (surf->visframe != r_framecount)
					continue;

				// don't backface underwater surfaces, because they warp
				if ( !(surf->flags & SURF_UNDERWATER) && ( (dot < 0) ^ !!(surf->flags & SURF_PLANEBACK)) )
					continue;		// wrong side

				// if sorting by texture, just store it out
				if (!mirror
				|| surf->texinfo->texture != cl.worldmodel->textures[mirrortexturenum])
				{
					surf->texturechain = surf->texinfo->texture->texturechain;
					surf->texinfo->texture->texturechain = surf;
				}
			}
		}

	}

// recurse down the back side
	R_RecursiveWorldNode (node->children[!side]);
}



/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
{
	entity_t	ent;

	memset (&ent, 0, sizeof(ent));
	ent.model = cl.worldmodel;

	VectorCopy (r_refdef.vieworg, modelorg);

	currententity = &ent;
	currenttexture = -1;

	memset (lightmap_polys, 0, sizeof(lightmap_polys));

	R_RecursiveWorldNode (cl.worldmodel->nodes);

	DrawTextureChains ();

	R_BlendLightmaps ();

}


/*
===============
R_MarkLeaves
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	mnode_t	*node;
	int		i;
	byte	solid[4096];

	if (r_oldviewleaf == r_viewleaf && !r_novis.value)
		return;

	if (__builtin_expect(mirror, 0))
		return;

	r_visframecount++;
	r_oldviewleaf = r_viewleaf;

	if (r_novis.value)
	{
		vis = solid;
		memset (solid, 0xff, (cl.worldmodel->numleafs+7)>>3);
	}
	else
		vis = Mod_LeafPVS (r_viewleaf, cl.worldmodel);

	for (i=0 ; i<cl.worldmodel->numleafs ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&cl.worldmodel->leafs[i+1];
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

// returns a texture number and the position inside it
int AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum=0 ; texnum<MAX_LIGHTMAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (allocated[texnum][i+j] >= best)
					break;
				if (allocated[texnum][i+j] > best2)
					best2 = allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	//Sys_Error ("AllocBlock: full");
	//return -1; // Manoel Kasimier - compiler warning fix
	return 0; // Manoel Kasimier - compiler warning fix
}


mvertex_t	*r_pcurrentvertbase;
model_t		*currentmodel;

int	nColinElim;

/*
================
BuildSurfaceDisplayList
================
*/
void BuildSurfaceDisplayList (msurface_t *fa)
{
	int			i, lindex, lnumverts;
	medge_t		*pedges, *r_pedge;
	float		*vec;
	float		s, t;
	glpoly_t	*poly;

// reconstruct the polygon
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;
	//
	// draw texture
	//
	poly = Hunk_AllocName (sizeof(glpoly_t) + (lnumverts-4) * VERTEXSIZE*sizeof(float), "polys");
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = r_pcurrentvertbase[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = r_pcurrentvertbase[r_pedge->v[1]].position;
		}
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->texture->height;

		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s*16;
		s += 8;
		s /= BLOCK_WIDTH*16; //fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t*16;
		t += 8;
		t /= BLOCK_HEIGHT*16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	//
	// remove co-linear points - Ed
	//
	if (!gl_keeptjunctions.value && !(fa->flags & SURF_UNDERWATER) )
	{
		for (i = 0 ; i < lnumverts ; ++i)
		{
			vec3_t v1, v2;
			float *prev, *this, *next;

			prev = poly->verts[(i + lnumverts - 1) % lnumverts];
			this = poly->verts[i];
			next = poly->verts[(i + 1) % lnumverts];

			VectorSubtract( this, prev, v1 );
			VectorNormalize( v1 );
			VectorSubtract( next, prev, v2 );
			VectorNormalize( v2 );

			// skip co-linear points
			#define COLINEAR_EPSILON 0.001f
			if ((FABS( v1[0] - v2[0] ) <= COLINEAR_EPSILON) &&
					(FABS( v1[1] - v2[1] ) <= COLINEAR_EPSILON) &&
					(FABS( v1[2] - v2[2] ) <= COLINEAR_EPSILON))
			{
				int j;
				for (j = i + 1; j < lnumverts; ++j)
				{
					int k;
					for (k = 0; k < VERTEXSIZE; ++k)
						poly->verts[j - 1][k] = poly->verts[j][k];
				}
				--lnumverts;
				++nColinElim;
				// retry next vertex next time, which is now current vertex
				--i;
			}
		}
	}
	poly->numverts = lnumverts;

}

/*
========================
GL_CreateSurfaceLightmap
========================
*/
void GL_CreateSurfaceLightmap (msurface_t *surf)
{
	int		smax, tmax;
	byte	*base;

	if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
		return;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	surf->lightmaptexturenum = AllocBlock (smax, tmax, &surf->light_s, &surf->light_t);
	base = lightmaps + surf->lightmaptexturenum*lightmap_bytes*BLOCK_WIDTH*BLOCK_HEIGHT;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * lightmap_bytes;
	R_BuildLightMap (surf, base, BLOCK_WIDTH*lightmap_bytes);
}


/*
==================
GL_BuildLightmaps

Builds the lightmap texture
with all the surfaces from all brush models
==================
*/
void GL_BuildLightmaps (void)
{
	int		i, j;
	model_t	*m;

	memset (allocated, 0, sizeof(allocated));

	r_framecount = 1;		// no dlightcache

	if (lightmap_textures[0] == 0)
	{
		glGenTextures(MAX_LIGHTMAPS, (GLuint*)lightmap_textures);
	}

	/*
	@Note default differently on the Dreamcast
	* use the new palette system, much much faster
	*/
#ifdef _arch_dreamcast
		gl_lightmap_format = GL_COLOR_INDEX8_EXT;
		lightmap_bytes = 1;
#else
	extern qboolean isPermedia;

	gl_lightmap_format = GL_LUMINANCE;
	// default differently on the Permedia
	if (isPermedia)
		gl_lightmap_format = GL_RGBA;

	if (COM_CheckParm ("-lm_1"))
		gl_lightmap_format = GL_LUMINANCE;
	if (COM_CheckParm ("-lm_a"))
		gl_lightmap_format = GL_ALPHA;
	if (COM_CheckParm ("-lm_i"))
		gl_lightmap_format = GL_INTENSITY;
	if (COM_CheckParm ("-lm_2"))
		gl_lightmap_format = GL_RGBA4;
	if (COM_CheckParm ("-lm_4"))
		gl_lightmap_format = GL_RGBA;

	switch (gl_lightmap_format)
	{
	case GL_RGBA:
		lightmap_bytes = 4;
		break;
	case GL_RGBA4:
		lightmap_bytes = 2;
		break;
	case GL_LUMINANCE:
	case GL_INTENSITY:
	case GL_ALPHA:
	case GL_COLOR_INDEX8_EXT:
		lightmap_bytes = 1;
		break;
	}
#endif

	for (j=1 ; j<MAX_MODELS ; j++)
	{
		m = cl.model_precache[j];
		if (!m)
			break;
		if (m->name[0] == '*')
			continue;
		r_pcurrentvertbase = m->vertexes;
		currentmodel = m;
		for (i=0 ; i<m->numsurfaces ; i++)
		{
			GL_CreateSurfaceLightmap (m->surfaces + i);
			if ( m->surfaces[i].flags & SURF_DRAWTURB )
				continue;
#ifndef QUAKE2
			if ( m->surfaces[i].flags & SURF_DRAWSKY )
				continue;
#endif
			BuildSurfaceDisplayList (m->surfaces + i);
		}
	}

	//
	// upload all lightmaps that were filled
	//
	printf("big lightmap uupload");
	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		if (!allocated[i][0])
			break;		// no more used
		lightmap_modified[i] = false;
		lightmap_rectchange[i].l = BLOCK_WIDTH;
		lightmap_rectchange[i].t = BLOCK_HEIGHT;
		lightmap_rectchange[i].w = 0;
		lightmap_rectchange[i].h = 0;
		GL_Bind(lightmap_textures[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		#ifdef _arch_dreamcast
		glTexParameteri(GL_TEXTURE_2D, GL_SHARED_TEXTURE_BANK_KOS, 1);

		glTexImage2D (GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT
		, BLOCK_WIDTH, BLOCK_HEIGHT, 0,
		GL_COLOR_INDEX, GL_UNSIGNED_BYTE, lightmaps+i*BLOCK_WIDTH*BLOCK_HEIGHT*lightmap_bytes);
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		#else
		glTexImage2D (GL_TEXTURE_2D, 0, lightmap_bytes
		, BLOCK_WIDTH, BLOCK_HEIGHT, 0,
		gl_lightmap_format, GL_UNSIGNED_BYTE, lightmaps+i*BLOCK_WIDTH*BLOCK_HEIGHT*lightmap_bytes);
		#endif
	}

}

