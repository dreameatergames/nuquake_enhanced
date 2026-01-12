
/*

 SDL_rotozoom - rotozoomer

 LGPL (c) A. Schiffler

*/

#ifndef _SDL_rotozoom_h
#define _SDL_rotozoom_h

#include <math.h>

#include "math-sll.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef M_PI
#define M_PI	3.141592654
#endif

#include "SDL.h"


/* ---- Defines */

#define SMOOTHING_OFF		0
#define SMOOTHING_ON		1

/* ---- Structures */

    typedef struct tColorRGBA {
	Uint8 r;
	Uint8 g;
	Uint8 b;
	Uint8 a;
    } tColorRGBA;

    typedef struct tColorY {
	Uint8 y;
    } tColorY;


/* ---- Prototypes */

#ifndef DLLINTERFACE
#ifdef WIN32
#ifdef BUILD_DLL
#define DLLINTERFACE __declspec(dllexport)
#else
#define DLLINTERFACE __declspec(dllimport)
#endif
#else
#define DLLINTERFACE
#endif
#endif

/* 
 
 rotozoomSurface()

 Rotates and zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
 'angle' is the rotation in degrees. 'zoom' a scaling factor. If 'smooth' is 1
 then the destination 32bit surface is anti-aliased. If the surface is not 8bit
 or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

*/

    DLLINTERFACE SDL_Surface *rotozoomSurface_sll(SDL_Surface * src, sll angle, sll zoom, int smooth);
#ifdef DREAMCAST
    #define rotozoomSurface(SRC,ANGLE,ZOOM,SMOOTH) \
    	(rotozoomSurface_sll(SRC,(double)ANGLE,(double)ZOOM,SMOOTH))
#else
    DLLINTERFACE SDL_Surface *rotozoomSurface(SDL_Surface * src, double angle, double zoom, int smooth);
#endif


    DLLINTERFACE SDL_Surface *rotozoomSurfaceXY_sll
    (SDL_Surface * src, sll angle, sll zoomx, sll zoomy, int smooth);
#ifdef DREAMCAST
    #define rotozoomSurfaceXY(SRC,ANGLE,ZOOMX,ZOOMY,SMOOTH) \
    	(rotozoomSurfaceXY_sll(SRC,(double)ANGLE,(double)ZOOMX,(double)ZOOMY,SMOOTH))
#else
    DLLINTERFACE SDL_Surface *rotozoomSurfaceXY
    (SDL_Surface * src, double angle, double zoomx, double zoomy, int smooth);
#endif


/* Returns the size of the target surface for a rotozoomSurface() call */


    DLLINTERFACE void rotozoomSurfaceSize_sll(int width, int height, sll angle, sll zoom, int *dstwidth, int *dstheight);
#ifdef DREAMCAST
    #define rotozoomSurfaceSize(WD,HG,ANGLE,ZOOM,DW,DH) \
    	(rotozoomSurfaceSize_sll(WD,HG,(double)ANGLE,(double)ZOOM,DW,DH))
#else
    DLLINTERFACE void rotozoomSurfaceSize(int width, int height, double angle, double zoom, int *dstwidth, int *dstheight);
#endif


    DLLINTERFACE void rotozoomSurfaceSizeXY_sll
    (int width, int height, sll angle, sll zoomx, sll zoomy, 
     int *dstwidth, int *dstheight);
#ifdef DREAMCAST
    #define rotozoomSurfaceSizeXY(WD,HG,ANGLE,ZOOMX,ZOOMY,DW,DH) \
    	(rotozoomSurfaceSizeXY_sll(WD,HG,(double)ANGLE,(double)ZOOMX,(double)ZOOMY,DW,DH))
#else
    DLLINTERFACE void rotozoomSurfaceSizeXY
    (int width, int height, double angle, double zoomx, double zoomy, 
     int *dstwidth, int *dstheight);
#endif
    
    

/* 
 
 zoomSurface()

 Zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
 'zoomx' and 'zoomy' are scaling factors for width and height. If 'smooth' is 1
 then the destination 32bit surface is anti-aliased. If the surface is not 8bit
 or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

*/

    DLLINTERFACE SDL_Surface *zoomSurface_sll(SDL_Surface * src, sll zoomx, sll zoomy, int smooth);
#ifdef DREAMCAST
    #define zoomSurface(SRC,ZOOMX,ZOOMY,SMOOTH) \
    	(zoomSurface_sll(SRC,(double)ZOOMX,(double)ZOOMY,SMOOTH))
#else
    DLLINTERFACE SDL_Surface *zoomSurface(SDL_Surface * src, double zoomx, double zoomy, int smooth);
#endif


/* Returns the size of the target surface for a zoomSurface() call */

    DLLINTERFACE void zoomSurfaceSize_sll(int width, int height, sll zoomx, sll zoomy, int *dstwidth, int *dstheight);
#ifdef DREAMCAST
    #define zoomSurfaceSize(WD,HG,ZOOMX,ZOOMY,DW,DH) \
    	(zoomSurfaceSize_sll(WD,HG,(double)ZOOMX,(double)ZOOMY,DW,DH))
#else
    DLLINTERFACE void zoomSurfaceSize(int width, int height, double zoomx, double zoomy, int *dstwidth, int *dstheight);

#endif


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif				/* _SDL_rotozoom_h */
