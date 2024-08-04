#ifndef _AICA_H_
#define _AICA_H_

#include <arch/irq.h>
#include <dc/g2bus.h>

/* Base of sound registers in the SH-4 */
extern volatile unsigned char *snd_base;

/* #define dc_snd_base ((volatile unsigned char *)0x00800000) */ /* arm side */
/* #define dc_snd_base ((volatile unsigned char *)0xa0700000) */ /* dc side */

/* Some convienence macros */
#define	SNDREGADDR(x)	((uint32)(snd_base + (x)))
#define	CHNREGADDR(ch,x)	SNDREGADDR(0x80*(ch)+(x))
#define SNDREG32A(x) ((volatile unsigned long*)(snd_base + (x)))
#define SNDREG32(x) (*SNDREG32A(x))
#define SNDREG8A(x) (snd_base + (x))
#define SNDREG8(x) (*SNDREG8A(x))
#define CHNREG32A(chn, x) SNDREG32A(0x80 * (chn) + (x))
#define CHNREG32(chn, x) (*CHNREG32A(chn, x))
#define CHNREG8A(chn, x) SNDREG8A(0x80 * (chn) + (x))
#define CHNREG8(chn, x) (*CHNREG8A(chn, x))

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

#define	AICA_MEM	0xa0800000

#define SM_8BIT		1
#define SM_16BIT	0
#define SM_ADPCM	2

void aica_play(int ch,int mode,unsigned long smpptr,int looptst,int loopend,uint32 freq,int vol,int pan,int loopflag);
void aica_stop(int ch);
void aica_vol(int ch,int vol);
void aica_pan(int ch,int pan);
void aica_freq(int ch,int freq);
int aica_get_pos(int ch);

#endif
