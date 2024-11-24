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
#include <arch/arch.h>
#include <dc/video.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/time.h>


#include <dc/sq.h>
#include <gl/glkos.h>

#include "errno.h"
/* Not using LZO1X right now */
//#include "minilzo.h"
#include "quakedef.h"

#if 0
__attribute__((no_instrument_function)) void __cyg_profile_func_enter (void *this_fn, void *call_site) {
  printf("e %x %x %lu\n", this_fn, call_site, time(NULL));
}

__attribute__((no_instrument_function)) void __cyg_profile_func_exit  (void *this_fn, void *call_site) {
  printf("x %x %x %lu\n", this_fn, call_site, time(NULL));
}
#endif

#define NO_CD 1

#ifndef NO_CD
static qboolean wasPlaying = false;
extern qboolean cdPlaying;
#endif // !NO_CD

static void drawtext(int x, int y, char *string);
extern void getRamStatus(void);
extern char *menu(int *argc, char **argv, char **basedir, int num_dirs);
extern void snd_bump_poll(void);
extern void dreamcast_sound_init(void);
extern void setSystemRam(void);

extern void arch_stk_trace(int n);
void *__stack_chk_guard = (void *)0x69420A55;
void __stack_chk_fail(void)
{
  char strbuffer[1024];
  uint32_t pr = arch_get_ret_addr();

  /* Reset video mode, clear screen */
  vid_set_mode(DM_640x480, PM_RGB565);
  vid_empty();

  /* Display the error message on screen */
  drawtext(32, 64, "nuQuake - Stack failure");
  sprintf(strbuffer, "PR = 0x%016X\n", pr);
  drawtext(32, 96, strbuffer);
  arch_stk_trace(2);


#ifdef FRAME_POINTERS
/* Lifted from Kallistios: kernel/arch/dreamcast/kernel/stack.c
   stack.c
   (c)2002 Dan Potter
*/
  int y=96+32;
  uint32_t fp = arch_get_fptr();
  int n = 3;
  drawtext(32, (y+=32), "-------- Stack Trace (innermost first) ---------");
#if 1
  while((fp > 0x100) && (fp != 0xffffffff)) {
      if((fp & 3) || (fp < 0x8c000000) || (fp > 0x8d000000)) {
          drawtext(32, (y+=32),"   (invalid frame pointer)\n");
          break;
      }

      if(n <= 0) {
          sprintf(strbuffer, "   %08lx\n", arch_fptr_ret_addr(fp));
          drawtext(32, (y+=32), strbuffer);
      }
      else n--;

      fp = arch_fptr_next(fp);
  }
#endif
  drawtext(32, (y+=32), "-------------- End Stack Trace -----------------\n");
#else
  drawtext(32, 128, "Stack Trace: frame pointers not enabled!\n");
#endif

}

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES 12
//#define MAX_HANDLES 8
FILE *sys_handles[MAX_HANDLES];

static int findhandle(void) {
  for (int i = 0; i < MAX_HANDLES; i++)
    if (!sys_handles[i])
      return i;
  Sys_Error("out of handles");
  return -1;
}


void Sys_DebugLog(char *file, char *fmt, ...)
{
}


/* Used for generating the lightmap multiple ways for debugging */
void VID_GenerateLighting(qboolean alpha) {
#ifdef _arch_dreamcast
  static unsigned int *lightingPalette = NULL;
  if(!lightingPalette)
    lightingPalette = (unsigned int *)Z_Malloc(256 * sizeof(uint32_t));

  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      if (alpha) {
        lightingPalette[i * 16 + j] = (i * 16 + j) << 24 | 0x0 << 16 | 0x0 << 8 | 0x0;
      } else {
        lightingPalette[i * 16 + j] = 0XFF << 24 | (i * 16 + j) << 16 | (i * 16 + j) << 8 | (i * 16 + j);
      }
    }
  }
  glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_1_KOS, GL_RGBA8, 256, GL_RGBA, GL_UNSIGNED_BYTE, (void *)lightingPalette);
#else
  (void)alpha;
#endif
}

void VID_GenerateLighting_f(void) {
  if (Cmd_Argc() < 1)
    return;


}

void VID_ChangeLightmapFilter_f(void) {

}

void VID_Gamma_f(void) {
}


void CDAudio_Pause(void)
{

}
/*
================
filelength
================
*/
int filelength(FILE *f) {
#ifndef NO_CD
  if (cdPlaying) {
    CDAudio_Pause();
    wasPlaying = true;
  }
#endif

  int pos;
  int end;

  pos = ftell(f);
  fseek(f, 0, SEEK_END);
  end = ftell(f);
  fseek(f, pos, SEEK_SET);

#ifndef NO_CD
  if (wasPlaying) {
    CDAudio_Resume();
    wasPlaying = false;
  }
#endif

  return end;
}

int Sys_FileOpenRead(char *path, int *hndl) {
#ifndef NO_CD
  if (cdPlaying) {
    CDAudio_Pause();
    wasPlaying = true;
  }
#endif

  FILE *f;
  int i;

  i = findhandle();
  if (i == -1) {
    printf("FILE: ERROR!\n");
    return -1;
  }

  f = fopen(path, "r");
  if (!f) {
    *hndl = -1;
    return -1;
  }
  sys_handles[i] = f;
  *hndl = i;

#ifndef NO_CD
  if (wasPlaying) {
    CDAudio_Resume();
    wasPlaying = false;
  }
#endif

  return filelength(f);
}

int Sys_FileOpenWrite(char *path) {
#ifndef NO_CD
  if (cdPlaying) {
    CDAudio_Pause();
    wasPlaying = true;
  }
#endif

  FILE *f;
  int i;

  i = findhandle();

  f = fopen(path, "w");
  if (f == NULL)
    Sys_Error("[%s] Error opening %s: %s", __func__, path, strerror(errno));
  sys_handles[i] = f;

#ifndef NO_CD
  if (wasPlaying) {
    CDAudio_Resume();
    wasPlaying = false;
  }
#endif

  return i;
}

void Sys_FileClose(int handle) {
  fclose(sys_handles[handle]);
  sys_handles[handle] = NULL;
}

void Sys_FileSeek(int handle, int position) {
  fseek(sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead(int handle, void *dest, int count) {
#ifndef NO_CD
  if (cdPlaying) {
    CDAudio_Pause();
    wasPlaying = true;
  }
#endif

  int x;
  x = fread(dest, 1, count, sys_handles[handle]);
  if (x == -1) {
    printf("%s: ERROR!\n", __func__);
  }

#ifndef NO_CD
  if (wasPlaying) {
    CDAudio_Resume();
    wasPlaying = false;
  }
#endif

  return x;
}

int Sys_FileWrite(int handle, void *data, int count) {
#ifndef NO_CD
  if (cdPlaying) {
    CDAudio_Pause();
    wasPlaying = true;
  }
#endif

  int x;
  x = fwrite(data, 1, count, sys_handles[handle]);
  if (x == -1) {
    printf("%s: ERRO!\n",__func__);
  }

#ifndef NO_CD
  if (wasPlaying) {
    CDAudio_Resume();
    wasPlaying = false;
  }
#endif

  return x;
}

int Sys_FileTime(char *path) {
  struct stat buffer;
  return ((stat(path, &buffer) == 0) ? 1 : -1);
}

void Sys_mkdir(char *path) {
  (void)path;
}

int VCR_Init(void) {
  return 0;
}

/*
===============================================================================

SYSTEM IO

===============================================================================
*/
static quakeparms_t parms;

void Sys_Error(char *error, ...) {
  va_list argptr;

  printf("Sys_Error: ");
  va_start(argptr, error);
  vprintf(error, argptr);
  va_end(argptr);
  printf("\n");
  fflush(stdout);

  Sys_Quit();
}

void Sys_Printf(char *fmt, ...) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr,fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

  //#ifndef QUIET
	printf("%s", text);
  //#endif
}

extern void glKosSwapBuffers_NO_PT(void);
void Sys_Quit(void) {
 //glKosSwapBuffers_NO_PT();
  glKosSwapBuffers();
  Host_Shutdown();
  vid_set_mode(DM_640x480, PM_RGB565);
  vid_empty();

  /* Display the error message on screen */
  drawtext(32, 64, "nuQuake shutdown...");
  arch_menu();
  //arch_exit();
}

extern void timer_ms_gettime(uint32_t *secs, uint32_t *msecs);
float Sys_FloatTime (void) {
  uint32_t s, ms;
  timer_ms_gettime(&s, &ms);
  return (float)s+(ms/1000.0f);
}

char *Sys_ConsoleInput(void) {
  return NULL;
}

void Sys_Sleep(void) {
}

qboolean isDedicated = false;

//-----------------------------------------------------------------------------
extern void bfont_draw_str(uint16 *buffer, int bufwidth, int opaque, char *str);
static void drawtext(int x, int y, char *string) {
  printf("%s\n", string);
  fflush(stdout);
  int offset = ((y * 640) + x);
  bfont_draw_str(vram_s + offset, 640, 1, string);
}

static void assert_hnd(const char *file, int line, const char *expr, const char *msg, const char *func) {
  char strbuffer[1024];

  /* Reset video mode, clear screen */
  vid_set_mode(DM_640x480, PM_RGB565);
  vid_empty();

  /* Display the error message on screen */
  drawtext(32, 64, "nuQuake - Assertion failure");

  sprintf(strbuffer, " Location: %s, line %d (%s)", file, line, func);
  drawtext(32, 96, strbuffer);

  sprintf(strbuffer, "Assertion: %s", expr);
  drawtext(32, 128, strbuffer);

  sprintf(strbuffer, "  Message: %s", msg);
  drawtext(32, 160, strbuffer);
  arch_exit();
}

double Sys_DoubleTime (void)
{
    struct timeval tp;
    struct timezone tzp; 
    static int      secbase; 
    
    gettimeofday(&tp, &tzp);  

    if (!secbase)
    {
        secbase = tp.tv_sec;
        return tp.tv_usec/1000000.0;
    }

    return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
}

//=============================================================================

//KOS_INIT_FLAGS(INIT_NONE | INIT_NO_DCLOAD | INIT_IRQ | INIT_THD_PREEMPT);

#define FIRST_CHUNK (2 * 1024 * 1024)    // 2MB initial chunk
#define SECOND_CHUNK (1.5 * 1024 * 1024) // 1.5MB second chunk
#define THIRD_CHUNK (1 * 1024 * 1024)    // 1MB third chunk
#define FINAL_CHUNK (512 * 1024)         // 0.5MB final chunk - smaller!

//#include <SDL/SDL.h>
//extern void handle_libc_overrides(void);
int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  /* Turn off newlib buffering */
  setbuf(stdout, NULL);
  fflush(stdout);

  //handle_libc_overrides();
  // Set up assertion handler
  assert_set_handler(assert_hnd);

  setSystemRam();
  //broken?

  GLdcConfig config;
  glKosInitConfig(&config);
  config.autosort_enabled = GL_FALSE;
  config.fsaa_enabled = GL_FALSE;
  config.internal_palette_format = GL_RGBA8;
  /*
  config.initial_op_capacity = 1024;
  config.initial_pt_capacity = 1024;
  config.initial_tr_capacity = 1024;
  config.initial_immediate_capacity = 1024;
  */
  config.initial_op_capacity = 4096 * 3;
  config.initial_pt_capacity = 256 * 3;
  config.initial_tr_capacity = 1024 * 3;
  config.initial_immediate_capacity = 256 * 3;
  glKosInitEx(&config);

  //glKosInit();
  memset(&parms, 0, sizeof(quakeparms_t));

  /* 17.6, 55.2 fps*/

#if 0
	//init lzo
	if (lzo_init() != LZO_E_OK)
    {
        printf("internal error - lzo_init() failed !!!\n");
        printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable '-DLZO_DEBUG' for diagnostics)\n");
        return -1;
    }
#endif

  char *basedirs[7] = {
      "/cd",          /* just dumped files */
      "/cd/QUAKE",    /* installed  */
      "/cd/QUAKE_SW", /* shareware */
      "/cd/data",     /* official CD-ROM */
      "/pc/quake",    /* debug */
      "/pc/quake_sw", /* debug */
      NULL};

  char *basedir;
#if 1
//Load Directly
	int i;
	for(i=0;(basedir = basedirs[i])!=NULL;i++) {
		int fd  = fs_open(basedir, O_RDONLY | O_DIR);
		if (fd < 0) continue;
			fs_close(fd);
			break;
	}
	if (basedir==NULL)
		Sys_Error("can't find quake dir");
	static char *args[10] = {"quake",NULL};
#else
#if 1
  //Display Mod Menu
  char *args[10] = {"quake", NULL};
  argc = 1;
  argv = args;
  basedir = menu(&argc, argv, basedirs, 6);
#else
  //Don't use mod menu
  static char *args[10] = {"quake", "-game", "kickflip", NULL};
  basedir = "/cd/data";
  argc = 3;
#endif
#endif

  float time, oldtime, newtime;

  COM_InitArgv(argc, args);

  parms.argc = com_argc;
  parms.argv = com_argv;
  ("Ram Info:");

  parms.memsize =  4 * 1024 * 1024;
  parms.membase = aligned_alloc(0x20, parms.memsize);
 	getRamStatus();
  malloc_stats();
  //printf("PVR Mem left:%u\n", (unsigned int)pvr_mem_available());
#ifdef GL_EXT_dreamcast_yalloc
  printf("GL Mem left:%u\n", (unsigned int)glGetFreeVRAM_INTERNAL_KOS());
#endif

  if (!parms.membase)
    Sys_Error("Not enough memory free;\n");

  parms.basedir = basedir;
  //SDL_Init(SDL_INIT_CDROM | SDL_INIT_AUDIO);

  Host_Init(&parms);
  oldtime = Sys_DoubleTime();
  //profiler_enable();
  while (1) {
    newtime = Sys_DoubleTime();
    time = newtime - oldtime;

  
    irq_disable();  
    Host_Frame(time);
    irq_enable();   

    oldtime = newtime;
}
  return 1;
}

