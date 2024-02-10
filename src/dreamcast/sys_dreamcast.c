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
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <dc/sq.h>

#include "errno.h"
/* Not using LZO1X right now */
//#include "minilzo.h" 
#include "quakedef.h"

static qboolean wasPlaying = false;
extern qboolean cdPlaying;
static void drawtext(int x, int y, char *string);
extern void getRamStatus(void);
extern char *menu(int *argc, char **argv, char **basedir, int num_dirs);
extern void snd_bump_poll(void);
extern void dreamcast_sound_init(void);
extern void setSystemRam(void);

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES 10
FILE *sys_handles[MAX_HANDLES];

int findhandle(void) {
  int i;

  for (i = 1; i < MAX_HANDLES; i++)
    if (!sys_handles[i])
      return i;
  Sys_Error("out of handles");
  return -1;
}

/*
================
filelength
================
*/
int filelength(FILE *f) {
  if (cdPlaying) {
    CDAudio_Pause();
    wasPlaying = true;
  }

  int pos;
  int end;

  pos = ftell(f);
  fseek(f, 0, SEEK_END);
  end = ftell(f);
  fseek(f, pos, SEEK_SET);

  if (wasPlaying) {
    CDAudio_Resume();
    wasPlaying = false;
  }

  return end;
}

int Sys_FileOpenRead(char *path, int *hndl) {
  if (cdPlaying) {
    CDAudio_Pause();
    wasPlaying = true;
  }

  FILE *f;
  int i;

  i = findhandle();
  if (i == -1) {
    return -1;
  }

  f = fopen(path, "r");
  if (!f) {
    *hndl = -1;
    return -1;
  }
  sys_handles[i] = f;
  *hndl = i;

  if (wasPlaying) {
    CDAudio_Resume();
    wasPlaying = false;
  }

  return filelength(f);
}

int Sys_FileOpenWrite(char *path) {
  if (cdPlaying) {
    CDAudio_Pause();
    wasPlaying = true;
  }

  FILE *f;
  int i;

  i = findhandle();

  f = fopen(path, "w");
  if (f == NULL)
    Sys_Error("[%s] Error opening %s: %s", __func__, path, strerror(errno));
  sys_handles[i] = f;

  if (wasPlaying) {
    CDAudio_Resume();
    wasPlaying = false;
  }

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
  if (cdPlaying) {
    CDAudio_Pause();
    wasPlaying = true;
  }

  int x;
  x = fread(dest, 1, count, sys_handles[handle]);
  if (x == -1) {
    printf("%s: ERRO!\n", __func__);
  }

  if (wasPlaying) {
    CDAudio_Resume();
    wasPlaying = false;
  }

  return x;
}

int Sys_FileWrite(int handle, void *data, int count) {
  if (cdPlaying) {
    CDAudio_Pause();
    wasPlaying = true;
  }

  int x;
  x = fwrite(data, 1, count, sys_handles[handle]);
  if (x == -1) {
    //printf("%s: ERRO!\n",__func__);
  }

  if (wasPlaying) {
    CDAudio_Resume();
    wasPlaying = false;
  }

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

  Sys_Quit();
}

void Sys_Printf(char *fmt, ...) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr,fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);
  
  #ifndef QUIET
	printf("%s", text);
  #endif
}

void Sys_Quit(void) {
  glKosSwapBuffers();
  Host_Shutdown();
  vid_set_mode(DM_640x480, PM_RGB565);
  vid_empty();

  /* Display the error message on screen */
  drawtext(32, 64, "nuQuake shutdown...");
  arch_exit();
}

#include <sys/time.h>

float Sys_FloatTime(void) {
  struct timeval tp;
  struct timezone tzp;
  static int secbase;

  gettimeofday(&tp, &tzp);

  if (!secbase) {
    secbase = tp.tv_sec;
    return tp.tv_usec / 1000000.0f;
  }

  return (tp.tv_sec - secbase) + tp.tv_usec / 1000000.0f;
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

//=============================================================================

//KOS_INIT_FLAGS(INIT_NONE | INIT_NO_DCLOAD | INIT_IRQ | INIT_THD_PREEMPT);

int main(int argc, char **argv) {
  // Set up assertion handler
  assert_set_handler(assert_hnd);

  setSystemRam();

  GLdcConfig config;
  glKosInitConfig(&config);
  config.autosort_enabled = true;
  config.initial_op_capacity = 4096;
  config.initial_pt_capacity = 0;
  config.initial_tr_capacity = 1024;
  config.initial_immediate_capacity = 0;

  glKosInitEx(&config);
  memset(&parms, 0, sizeof(quakeparms_t));

#if 0
	//init lzo
	if (lzo_init() != LZO_E_OK)
    {
        printf("internal error - lzo_init() failed !!!\n");
        printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable '-DLZO_DEBUG' for diagnostics)\n");
        return -1;
    }
#endif

  char *basedirs[6] = {
      "/cd/QUAKE",    /* installed  */
      "/cd/QUAKE_SW", /* shareware */
      "/cd/data",     /* official CD-ROM */
      "/pc/quake",    /* debug */
      "/pc/quake_sw", /* debug */
      NULL};

  char *basedir;
#if 0
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
  parms.memsize = 10 * 1024 * 1024;

  int t;
  if (COM_CheckParm("-heapsize")) {
    t = COM_CheckParm("-heapsize") + 1;

    if (t < com_argc)
      parms.memsize = atoi(com_argv[t]) * 1024;
  }

  INFO_MSG("Ram Info:");
  printf("Memsize: %dMB\n", parms.memsize / 1024 / 1024);
  parms.membase = memalign(0x20, parms.memsize);
 	getRamStatus();
  malloc_stats();
  printf("GL Mem left:%u\n", (unsigned int)pvr_mem_available());

  if (!parms.membase)
    Sys_Error("Not enough memory free;\n");

  sq_clr(parms.membase, 0x0);
  parms.basedir = basedir;

  Host_Init(&parms);
  oldtime = Sys_FloatTime();
  //profiler_enable();
  while (1) {
    newtime = Sys_FloatTime();
    time = newtime - oldtime;

    Host_Frame(time);
    oldtime = newtime;
    SNDDMA_Submit();
    //snd_bump_poll();
  }
  return 1;
}

#if 1
#include <stdio.h>
#include <assert.h>
#include <arch/rtc.h>
#include <kos/fs.h>
#include <kos/thread.h>
#include <kos/fs_pty.h>
#include <kos/fs_romdisk.h>
#include <kos/fs_ramdisk.h>
#include <kos/library.h>
#include <kos/net.h>
#include <kos/dbgio.h>
#include <dc/fs_iso9660.h>
#include <dc/fs_vmu.h>
#include <dc/vmufs.h>
#include <dc/fs_dcload.h>
#include <dc/fs_dclsocket.h>
#include <dc/spu.h>
#include <dc/pvr.h>
#include <dc/maple.h>
#include <dc/sound/sound.h>
#include <dc/scif.h>
#include <arch/irq.h>
#include <arch/timer.h>
#include <dc/fb_console.h>
int arch_auto_init() {

    /* Initialize memory management */
    mm_init();

    /* Do this immediately so we can receive exceptions for init code
       and use ints for dbgio receive. */
    irq_init();         /* IRQs */
    irq_disable();          /* Turn on exceptions */

    fs_dcload_init_console();   /* Init dc-load console, if applicable */

    // Init SCIF for debug stuff (maybe)
    scif_init();

    /* Init debug IO */
    dbgio_init();

    /* Print a banner */
    if(__kos_init_flags & INIT_QUIET)
        dbgio_disable();
    else {
        // PTYs not initialized yet
    }

    timer_init();           /* Timers */
    hardware_sys_init();        /* DC low-level hardware init */

    /* Initialize our timer */
    timer_ms_enable();
    rtc_init();

    /* Threads */
    thd_init(THD_MODE_PREEMPT);

    nmmgr_init();

    fs_init();          /* VFS */
    fs_pty_init();          /* Pty */
    fs_ramdisk_init();      /* Ramdisk */

    hardware_periph_init();     /* DC peripheral init */


    if(!(__kos_init_flags & INIT_NO_DCLOAD) && *DCLOADMAGICADDR == DCLOADMAGICVALUE) {
        dbglog(DBG_INFO, "dc-load console support enabled\n");
        fs_dcload_init();
    }

    fs_iso9660_init();
    vmufs_init();
    fs_vmu_init();

    // Initialize library handling
    library_init();

    /* Now comes the optional stuff */
    if(__kos_init_flags & INIT_IRQ) {
        irq_enable();       /* Turn on IRQs */
    }

    return 0;
}
#endif