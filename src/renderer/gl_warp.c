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
// gl_warp.c -- sky and water polygons

#include "quakedef.h"

extern	model_t	*loadmodel;

unsigned int		solidskytexture = 0;
unsigned int		alphaskytexture = 0;
float	speedscale;		// for top sky and bottom sky

msurface_t	*warpface;

extern cvar_t gl_subdivide_size;

void BoundPoly (int numverts, float *verts, vec3_t mins, vec3_t maxs)
{
	int		i, j;
	float	*v;

	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	v = verts;
	for (i=0 ; i<numverts ; i++)
		for (j=0 ; j<3 ; j++, v++)
		{
			if (*v < mins[j])
				mins[j] = *v;
			if (*v > maxs[j])
				maxs[j] = *v;
		}
}

void SubdividePolygon (int numverts, float *verts)
{
	int		i, j, k;
	vec3_t	mins, maxs;
	float	m;
	float	*v;
	vec3_t	front[64], back[64];
	int		f, b;
	float	dist[64];
	float	frac;
	glpoly_t	*poly;
	float	s, t;

	if (numverts > 60)
		Sys_Error ("numverts = %i", numverts);

	BoundPoly (numverts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++)
	{
		m = (mins[i] + maxs[i]) * 0.5;
		m = gl_subdivide_size.value * floorf (m/gl_subdivide_size.value + 0.5f);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// cut it
		v = verts + i;
		for (j=0 ; j<numverts ; j++, v+= 3)
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v-=i;
		VectorCopy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0 ; j<numverts ; j++, v+= 3)
		{
			if (dist[j] >= 0)
			{
				VectorCopy (v, front[f]);
				f++;
			}
			if (dist[j] <= 0)
			{
				VectorCopy (v, back[b]);
				b++;
			}
			if (dist[j] == 0 || dist[j+1] == 0)
				continue;
			if ( (dist[j] > 0) != (dist[j+1] > 0) )
			{
				// clip point
				frac = dist[j] / (dist[j] - dist[j+1]);
				for (k=0 ; k<3 ; k++)
					front[f][k] = back[b][k] = v[k] + frac*(v[3+k] - v[k]);
				f++;
				b++;
			}
		}

		SubdividePolygon (f, front[0]);
		SubdividePolygon (b, back[0]);
		return;
	}

	poly = Hunk_AllocName (sizeof(glpoly_t) + (numverts-4) * VERTEXSIZE*sizeof(float), "polysub");
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts;
	for (i=0 ; i<numverts ; i++, verts+= 3)
	{
		VectorCopy (verts, poly->verts[i]);
		s = DotProduct (verts, warpface->texinfo->vecs[0]);
		t = DotProduct (verts, warpface->texinfo->vecs[1]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;
	}
}

/*
================
GL_SubdivideSurface

Breaks a polygon up along axial 64 unit
boundaries so that turbulent and sky warps
can be done reasonably.
================
*/
void GL_SubdivideSurface (msurface_t *fa)
{
	vec3_t		verts[64];
	int			numverts;
	int			i;
	int			lindex;
	float		*vec;

	warpface = fa;

	//
	// convert edges back to a normal polygon
	//
	numverts = 0;
	for (i=0 ; i<fa->numedges ; i++)
	{
		lindex = loadmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
			vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else
			vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
		VectorCopy (vec, verts[numverts]);
		numverts++;
	}

	SubdividePolygon (numverts, verts[0]);
}

//=========================================================



// speed up sin calculations - Ed
float	turbsin[] =
{
	#include "gl_warp_sin.h"
};
#define TURBSCALE (256.0 / (2 * M_PI))
/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented glpoly_t chain
=============
*/
int numwaterverts = 0;
void EmitWaterPolys (msurface_t *fa)
{
	glpoly_t	*p;
	int			i;

	glvert_fast_t* restrict submission_pointer = R_GetDirectBufferAddress();

	for (p=fa->polys ; p ; p=p->next)
	{
		const float*restrict v0 = p->verts[0];
		const float v0_x = v0[0];
		const float v0_y = v0[1];
		const float v0_z = v0[2];

    float* restrict v1 = p->verts[1];
    float* restrict v2 = p->verts[1] + VERTEXSIZE;

		const float os_0 = v0[3];
		const float ot_0 = v0[4];
		const float s0 = (os_0 + turbsin[(int)((ot_0*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);
		const float t0 = (ot_0 + turbsin[(int)((os_0*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);

    for (i = 0; i < p->numverts - 2; i++)
    {
			/* 2nd vertex */
			const float v1_x = v1[0];
			const float v1_y = v1[1];
			const float v1_z = v1[2];

			const float os_1 = v1[3];
			const float ot_1 = v1[4];
			const float s1 = (os_1 + turbsin[(int)((ot_1*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);
			const float t1 = (ot_1 + turbsin[(int)((os_1*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);

			/* 3rd vertex */
			const float v2_x = v2[0];
			const float v2_y = v2[1];
			const float v2_z = v2[2];

			const float os_2 = v2[3];
			const float ot_2 = v2[4];
			const float s2 = (os_2 + turbsin[(int)((ot_2*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);
			const float t2 = (ot_2 + turbsin[(int)((os_2*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);

			*submission_pointer++ = (glvert_fast_t){.flags = VERTEX, .vert = {v0_x, v0_y, v0_z}, .texture = {s0, t0}, VTX_COLOR_WHITE, .pad0 = {0}};
			*submission_pointer++ = (glvert_fast_t){.flags = VERTEX, .vert = {v1_x, v1_y, v1_z}, .texture = {s1, t1}, VTX_COLOR_WHITE, .pad0 = {0}};
			*submission_pointer++ = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {v2_x, v2_y, v2_z}, .texture = {s2, t2}, VTX_COLOR_WHITE, .pad0 = {0}};

			v1 += VERTEXSIZE;
			v2 += VERTEXSIZE;
			numwaterverts += 3;
		}
	}
}

unsigned char water_alpha = 0;
void EmitWaterPolys_Temp(msurface_t *fa)
{
	glpoly_t	*p;
	int			i;
	float		s, t, os, ot;
	water_alpha = (unsigned char)(255*r_wateralpha.value);

	for (p=fa->polys ; p ; p=p->next)
	{
		const float *v0 = p->verts[0];
    float *v = p->verts[1];

		os = v0[3];
		ot = v0[4];
		const float s0 = (os + turbsin[(int)((ot*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);
		const float t0 = (ot + turbsin[(int)((os*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);

    for (i = 0; i < p->numverts - 2; i++)
    {
			os = v[3];
			ot = v[4];
			s = (os + turbsin[(int)((ot*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);
			t = (ot + turbsin[(int)((os*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);

			r_batchedtempverts[numwaterverts++] = (glvert_fast_t){.flags = VERTEX, .vert = {v0[0], v0[1], v0[2]}, .texture = {s0, t0}, .color = {.array = {255, 255, 255, water_alpha}}, .pad0 = {0}};
			r_batchedtempverts[numwaterverts++] = (glvert_fast_t){.flags = VERTEX, .vert = {v[0], v[1], v[2]}, .texture = {s, t}, .color = {.array = {255, 255, 255, water_alpha}}, .pad0 = {0}};

			v += VERTEXSIZE;
			os = v[3];
			ot = v[4];
			s = (os + turbsin[(int)((ot*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);
			t = (ot + turbsin[(int)((os*0.125+realtime) * TURBSCALE) & 255])*(1.0/64);

			r_batchedtempverts[numwaterverts++] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {v[0], v[1], v[2]}, .texture = {s, t}, .color = {.array = {255, 255, 255, water_alpha}}, .pad0 = {0}};
		}
	}
}

/*
=============
EmitSkyPolys
=============
*/
int numwarpverts = 0;
void EmitSkyPolys (msurface_t *fa)
{
	glpoly_t	*p;
	float		*v;
	int			i;
	float	s, t;
	vec3_t	dir;
	float	length;

	for (p=fa->polys ; p ; p=p->next)
	{
		#ifdef PARANOID
		if(numwarpverts > MAX_BATCHED_TEMPVERTEXES){
			glDrawArrays(GL_TRIANGLES, 0, numwarpverts);
    	if(numwarpverts > 24)
    		printf("%s:%d drew: %d\n", __FILE__, __LINE__, numwarpverts);
			numwarpverts = 0;
		}
		#endif

		const float *v0 = p->verts[0];
	    v = p->verts[1];
		VectorSubtract (v0, r_origin, dir);
		dir[2] *= 3;	// flatten the sphere

		length = dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2];
#if defined(_arch_dreamcast) && defined(ENABLE_DC_MATH)
		length = (6.0f*63.0f)*MATH_fsrra(length);
#else
		length = SQRT (length);
		length = 6*63/length;
#endif

		dir[0] *= length;
		dir[1] *= length;

		const float s0 = (speedscale + dir[0]) * (1.0f/128.0f);
		const float t0 = (speedscale + dir[1]) * (1.0f/128.0f);
	    for (i = 0; i < p->numverts - 2; i++)
	    {
			VectorSubtract (v, r_origin, dir);
			dir[2] *= 3;    // flatten the sphere
			length = dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2];
		#if defined(_arch_dreamcast) && defined(ENABLE_DC_MATH)
			length = (6.0f*63.0f)*MATH_fsrra(length);
		#else
			length = SQRT (length);
			length = 6*63/length;
		#endif
			dir[0] *= length;
			dir[1] *= length;
			s = (speedscale + dir[0]) * (1.0f/128.0f);
			t = (speedscale + dir[1]) * (1.0f/128.0f);

			r_batchedtempverts[numwarpverts++] = (glvert_fast_t){.flags = VERTEX, .vert = {v0[0], v0[1], v0[2]}, .texture = {s0, t0}, VTX_COLOR_WHITE, .pad0 = {0}};
			r_batchedtempverts[numwarpverts++] = (glvert_fast_t){.flags = VERTEX, .vert = {v[0], v[1], v[2]}, .texture = {s, t}, VTX_COLOR_WHITE, .pad0 = {0}};
			v += VERTEXSIZE;
			VectorSubtract (v, r_origin, dir);
			dir[2] *= 3;    // flatten the sphere
			length = dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2];
		#if defined(_arch_dreamcast) && defined(ENABLE_DC_MATH)
			length = (6.0f*63.0f)*MATH_fsrra(length);
		#else
			length = SQRT (length);
			length = 6*63/length;
		#endif
			dir[0] *= length;
			dir[1] *= length;
			s = (speedscale + dir[0]) * (1.0f/128.0f);
			t = (speedscale + dir[1]) * (1.0f/128.0f);
			r_batchedtempverts[numwarpverts++] = (glvert_fast_t){.flags = VERTEX_EOL, .vert = {v[0], v[1], v[2]}, .texture = {s, t}, VTX_COLOR_WHITE, .pad0 = {0}};
	    }
	}
}

extern cvar_t r_sky;
/*
=================
R_DrawSkyChain
=================
*/
void R_DrawSkyChain (msurface_t *s)
{
	if(!r_sky.value)
		return;

	msurface_t	*fa;

	GL_DisableMultitexture();

	// used when gl_texsort is on
	GL_Bind(solidskytexture);
	speedscale = realtime*8;
	speedscale -= (int)speedscale & ~127 ;

	glEnableClientState(GL_COLOR_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(3, GL_FLOAT, sizeof(glvert_fast_t), &r_batchedtempverts[0].vert);
  glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_fast_t), &r_batchedtempverts[0].texture);
#ifdef WIN98
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &r_batchedtempverts[0].color);
#else
		glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, sizeof(glvert_fast_t), &r_batchedtempverts[0].color);
#endif

	numwarpverts = 0;
	for (fa=s ; fa ; fa=fa->texturechain)
		EmitSkyPolys (fa);

	glDrawArrays(GL_TRIANGLES, 0, numwarpverts);
	#ifdef PARANOID
  	if(numwarpverts > 441)
    printf("%s:%d drew: %d\n", __FILE__, __LINE__, numwarpverts);
	#endif

	glEnable(GL_BLEND);
	GL_Bind (alphaskytexture);
	speedscale = realtime*16;
	speedscale -= (int)speedscale & ~127 ;

	numwarpverts = 0;
	for (fa=s ; fa ; fa=fa->texturechain)
		EmitSkyPolys (fa);

	glDrawArrays(GL_TRIANGLES, 0, numwarpverts);
	#ifdef PARANOID
	if(numwarpverts > 441)
    printf("%s:%d drew: %d\n", __FILE__, __LINE__, numwarpverts);
	#endif

	glDisable(GL_BLEND);
}

//===============================================================

/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky (miptex_t *mt)
{
	int			i, j, p;
	byte		*src;
	unsigned __attribute__((aligned(32)))	trans[128*128];
	unsigned	transpix;
	int			r, g, b;
	unsigned	*rgba;

	src = (byte *)mt + mt->offsets[0];

	// make an average value for the back to avoid
	// a fringe on the top level

	r = g = b = 0;
	for (i=0 ; i<128 ; i++)
		for (j=0 ; j<128 ; j++)
		{
			p = src[i*256 + j + 128];
			rgba = &d_8to24table[p];
			trans[(i*128) + j] = *rgba;
			r += ((byte *)rgba)[0];
			g += ((byte *)rgba)[1];
			b += ((byte *)rgba)[2];
		}

	((byte *)&transpix)[0] = r/(128*128);
	((byte *)&transpix)[1] = g/(128*128);
	((byte *)&transpix)[2] = b/(128*128);
	((byte *)&transpix)[3] = 0;


	if (!solidskytexture)
		glGenTextures(1, (GLuint*)&solidskytexture);
	GL_Bind (solidskytexture);
	glTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


	for (i=0 ; i<128 ; i++)
		for (j=0 ; j<128 ; j++)
		{
			p = src[i*256 + j];
			if (p == 0)
				trans[(i*128) + j] = transpix;
			else
				trans[(i*128) + j] = d_8to24table[p];
		}

	if (!alphaskytexture)
		glGenTextures(1, (GLuint*)&alphaskytexture);
	GL_Bind(alphaskytexture);
	glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

}

