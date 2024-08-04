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


	(c)2002 bero

*/
#include "quakedef.h"
#include <dc/spu.h>
#include "aica.h"

static int snd_inited;

#define	SAMPLE_SIZE	0x8000
#define	SAMPLE_START	0x11000
#define	FREQ	11025
#define	VOL	 240
#define	CHANNELS	2

qboolean SNDDMA_Init(void)
{
	if (snd_inited) {
		printf("Sound already init'd\n");
		return 1;
	}

	shm = &sn;
	shm->splitbuffer = 0;

	shm->speed = FREQ;
	shm->samplebits = 16;
	shm->channels = CHANNELS;

	shm->soundalive = true;
	shm->samples = SAMPLE_SIZE*CHANNELS; /* real samples * channels */
	shm->samplepos = 0;
	shm->submission_chunk = 1;
	shm->buffer = (void*)(AICA_MEM+SAMPLE_START);

	spu_init();

//	return 0;

	/* stream start */
	aica_play(0,SM_16BIT,SAMPLE_START,0,SAMPLE_SIZE,FREQ,VOL,0,1);
	aica_play(1,SM_16BIT,SAMPLE_START+SAMPLE_SIZE*2,0,SAMPLE_SIZE,FREQ,VOL,255,1);

	snd_inited = 1;

	return 1;
}

int SNDDMA_GetDMAPos(void)
{
	int ret;
	if (!snd_inited)
		return (0);

	ret = aica_get_pos(0);
//	printf("pos:%x\n",ret);
	ret = (ret&~1)*CHANNELS;
	return ret;
}

void SNDDMA_Shutdown(void)
{
	if (snd_inited) {
		spu_disable();
		snd_inited = 0;
	}
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
}

