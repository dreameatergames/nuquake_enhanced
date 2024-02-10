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
// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "quakedef.h"

#ifdef _WIN32
#include "winquake.h"
#else
#define DWORD	unsigned long
#endif

#define	PAINTBUFFER_SIZE	512
portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
/*int*/	short	snd_scaletable[32][256];

#define G2_LOCK(OLD) \
	do { \
		if (!irq_inside_int()) \
			OLD = irq_disable(); \
		/* suspend any G2 DMA here... */ \
		while((*(volatile unsigned int *)0xa05f688c) & 0x20) \
			; \
	} while(0)

#define G2_UNLOCK(OLD) \
	do { \
		/* resume any G2 DMA here... */ \
		if (!irq_inside_int()) \
			irq_restore(OLD); \
	} while(0)

void Snd_WriteLinearBlastStereo16_separate (short *_snd_l,short *_snd_r,int *snd_p,unsigned int nsamples,int snd_vol)
{
	unsigned int i;
	int	val,old=0;
	unsigned long *snd_l = (unsigned long*)_snd_l;
	unsigned long *snd_r = (unsigned long*)_snd_r;
//	printf("%p %p %d\n",snd_l,snd_r,nsamples);

	for (i=0 ; i<nsamples/2 ; i++)
	{
		int lval,rval;
	G2_LOCK(old);
		val = (*snd_p++ *snd_vol)>>8;
		if (val > 0x7fff)
			val = 0x7fff;
		else if (val < (short)0x8000)
			val = (short)0x8000;

		lval = val;

		val = (*snd_p++ *snd_vol)>>8;
		if (val > 0x7fff)
			val = 0x7fff;
		else if (val < (short)0x8000)
			val = (short)0x8000;

		rval = val;

		val = (*snd_p++ *snd_vol)>>8;
		if (val > 0x7fff)
			val = 0x7fff;
		else if (val < (short)0x8000)
			val = (short)0x8000;

		snd_l[i] = lval | (val<<16);

		val = (*snd_p++ *snd_vol)>>8;
		if (val > 0x7fff)
			val = 0x7fff;
		else if (val < (short)0x8000)
			val = (short)0x8000;

		snd_r[i] = rval | (val<<16);
	G2_UNLOCK(old);
		if ((i&3)==0) g2_fifo_wait();
	}
	g2_fifo_wait();
}

void S_TransferPaintBuffer16 (int endtime)
{
	int		lpos;
	int		lpaintedtime;
	DWORD	*pbuf;
	short	*snd_out;
	int	*snd_p;
	int	snd_vol,snd_linear_count;
	int	samples = shm->samples>>1;
	
	snd_vol = volume.value*256;

	snd_p = (int *) paintbuffer;
	lpaintedtime = paintedtime;

	{
		pbuf = (DWORD *)shm->buffer;
	}

//	printf("painted=%x end=%x\n",paintedtime,endtime);

	while (lpaintedtime < endtime)
	{
	// handle recirculating buffer issues
		lpos = lpaintedtime & (samples-1);

		snd_out = (short *) pbuf + lpos;

		snd_linear_count = samples - lpos;
		if (lpaintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - lpaintedtime;

	// write a linear blast of samples
		Snd_WriteLinearBlastStereo16_separate (snd_out,snd_out+samples,snd_p,snd_linear_count,snd_vol);

		snd_p += snd_linear_count*2;
		lpaintedtime += snd_linear_count;
	}
}

/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

void SND_PaintChannelFrom8 (channel_t *ch, sfxcache_t *sc, int endtime);
void SND_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int endtime);

void S_PaintChannels(int endtime)
{
	int 	i;
	int 	end;
	channel_t *ch;
	sfxcache_t	*sc;
	int		ltime, count;

	while (paintedtime < endtime)
	{
	// if paintbuffer is smaller than DMA buffer
		end = endtime;
		if (endtime - paintedtime > PAINTBUFFER_SIZE)
			end = paintedtime + PAINTBUFFER_SIZE;

	// clear the paint buffer
		memset(paintbuffer, 0, (end - paintedtime) * sizeof(portable_samplepair_t));

	// paint in the channels.
		ch = channels;
		for (i=0; i<total_channels ; i++, ch++)
		{
			if (!ch->sfx)
				continue;
			if (!ch->leftvol && !ch->rightvol)
				continue;
			sc = S_LoadSound (ch->sfx);
			if (!sc)
				continue;

			ltime = paintedtime;

			while (ltime < end)
			{	// paint up to end
				if (ch->end < end)
					count = ch->end - ltime;
				else
					count = end - ltime;

				if (count > 0)
				{	
					if (sc->width == 1)
						SND_PaintChannelFrom8(ch, sc, count);
					else
						SND_PaintChannelFrom16(ch, sc, count);
	
					ltime += count;
				}

			// if at end of loop, restart
				if (ltime >= ch->end)
				{
					if (sc->loopstart >= 0)
					{
						ch->pos = sc->loopstart;
						ch->end = ltime + sc->length - ch->pos;
					}
					else				
					{	// channel just stopped
						ch->sfx = NULL;
						break;
					}
				}
			}
															  
		}

	// transfer out according to DMA format
		S_TransferPaintBuffer16(end);
		paintedtime = end;
	}
}

void SND_InitScaletable (void)
{
	int		i, j;
	
	for (i=0 ; i<32 ; i++)
		for (j=0 ; j<256 ; j++)
			snd_scaletable[i][j] = ((signed char)j) * i * 8;
}


#if	!id386

void SND_PaintChannelFrom8 (channel_t *ch, sfxcache_t *sc, int count)
{
	int 	data;
	short		*lscale, *rscale;
	unsigned char *sfx;
	int		i;

	if (ch->leftvol > 255)
		ch->leftvol = 255;
	if (ch->rightvol > 255)
		ch->rightvol = 255;
		
	lscale = snd_scaletable[ch->leftvol >> 3];
	rscale = snd_scaletable[ch->rightvol >> 3];
	sfx = (signed char *)sc->data + ch->pos;

	for (i=0 ; i<count ; i++)
	{
		data = sfx[i];
		paintbuffer[i].left += lscale[data];
		paintbuffer[i].right += rscale[data];
	}
	
	ch->pos += count;
}

#endif	// !id386


void SND_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int count)
{
	int data;
	int left, right;
	int leftvol, rightvol;
	signed short *sfx;
	int	i;

	leftvol = ch->leftvol;
	rightvol = ch->rightvol;
	sfx = (signed short *)sc->data + ch->pos;

	for (i=0 ; i<count ; i++)
	{
		data = sfx[i];
		left = (data * leftvol) >> 8;
		right = (data * rightvol) >> 8;
		paintbuffer[i].left += left;
		paintbuffer[i].right += right;
	}

	ch->pos += count;
}

