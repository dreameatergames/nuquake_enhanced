/*

	fake cdda using raw read + spu stream

	(c) 2002 bero

	under GPL or notify me
	this means : if you close source code, notify me , otherwise you can use freely under GPL.

*/

#include <stdlib.h> /* for alloca */

#include <arch/spinlock.h>
#include <dc/cdrom.h>
#include <dc/spu.h>
#include "aica.h"
#include "fake_cdda.h"

extern spinlock_t *cdrom_mutex_get(void);

#define	CDROM_COOKED	0
#define	CDROM_RAW	1
#define	CDDA_SECTOR_SIZE	2352

#define MAKE_SYSCALL(rs, p1, p2, idx) \
	uint32 *syscall_bc = (uint32*)0x8c0000bc; \
	int (*syscall)() = (int (*)())(*syscall_bc); \
	rs syscall((p1), (p2), 0, (idx));
/* Set disc access mode */
static int gdc_change_data_type(void *param) { MAKE_SYSCALL(return, param, 0, 10); }

/* from dreamrip */
static int cdrom_change_mode(int n) {
	uint32	params[4];

	params[0] = 0;				/* 0 = set, 1 = get */
	params[1] = (n==CDROM_COOKED?8192:4096);			/* 8192 ? */
	params[2] = 0; //cdxa ? 2048 : 1024;		/* CD-XA mode 1/2 */
	params[3] = (n==CDROM_COOKED?2048:2352);			/* sector size */
	if (gdc_change_data_type(params) < 0) return ERR_SYS;
	return ERR_OK;
}


static int cdda_read_sectors(void *buffer, int sector, int cnt)
{
	int ret;
	if ((ret=cdrom_change_mode(CDROM_RAW))!=ERR_OK) return ret;
	if ((ret=cdrom_read_sectors(buffer,sector,cnt))!=ERR_OK) return ret;
	if ((ret=cdrom_change_mode(CDROM_COOKED))!=ERR_OK) return ret;
	return ret;
}

#define	SPURAM_BASE	0xa0800000

static void spu_memload_stereo16(unsigned int _snd_l,unsigned int _snd_r,const short *src,size_t nbyte)
{
	int i;
	unsigned long *snd_l = (unsigned long*)(_snd_l + SPURAM_BASE);
	unsigned long *snd_r = (unsigned long*)(_snd_r + SPURAM_BASE);
	

	for(i=0;i<nbyte/8;i++) {
		int lval,rval;
		lval  = (*src++)&0xffff;
		rval  = (*src++)&0xffff;
		snd_l[i] = lval | ((*src++)<<16);
		snd_r[i] = rval | ((*src++)<<16);
		if ((i&3)==0) g2_fifo_wait();
	}
	g2_fifo_wait();
}

static void spu_memload_stereo8(unsigned int _snd_l,unsigned int _snd_r,const unsigned char *src,size_t nbyte)
{
	int i;
	unsigned long *snd_l = (unsigned long*)(_snd_l + SPURAM_BASE);
	unsigned long *snd_r = (unsigned long*)(_snd_r + SPURAM_BASE);

	for(i=0;i<nbyte/8;i++) {
		int lval,rval,lval2,rval2;
		lval  = (*src++);
		rval  = (*src++);
		lval |= (*src++)<<8;
		rval |= (*src++)<<8;
		lval2  = (*src++);
		rval2  = (*src++);
		lval2 |= (*src++)<<8;
		rval2 |= (*src++)<<8;
		snd_l[i] = lval | (lval2<<16);
		snd_r[i] = rval | (rval2<<16);
		if ((i&3)==0) g2_fifo_wait();
	}
	g2_fifo_wait();
}

#define	CDDA_FREQ	44100


static unsigned cdda_chn;

static unsigned n_sec;
static int vol_l,vol_r;
static int start_sector,end_sector,current_sector,loopcnt;
static int status;
static unsigned snd_l,snd_r,sampsize;
static unsigned prev_pos,writed_pos;

static void update(int bufno);

/* you must call after snd_init() */
int fake_cdda_init(void)
{
	cdda_chn = 62;
	vol_r = vol_l = 256;
	status = CD_STATUS_STANDBY;

	n_sec = 15;
	sampsize = n_sec*(CDDA_SECTOR_SIZE/4*2);
	snd_l = 0x40000;
	snd_r = 0x40000 + sampsize*2;

	return ERR_OK;
}

int fake_cdda_shutdown(void)
{
	fake_cdda_stop();
	return ERR_OK;
}

int fake_cdda_play(int start_sec,int end_sec,int loop)
{
	start_sector = current_sector = start_sec;
	end_sector = end_sec;
	if (loop==0) loop=1;
	loopcnt = loop;

	fake_cdda_stop();

	update(0);

	status = CD_STATUS_PAUSED;
	return fake_cdda_resume();
}

int fake_cdda_resume(void)
{
	if (status!=CD_STATUS_PAUSED) return ERR_SYS;
	aica_play(cdda_chn+0,SM_16BIT,snd_l,0,sampsize,CDDA_FREQ,vol_l,  0,1);
	aica_play(cdda_chn+1,SM_16BIT,snd_r,0,sampsize,CDDA_FREQ,vol_r,255,1);
	prev_pos = 0;
	status = CD_STATUS_PLAYING;
	return ERR_OK;
}

int fake_cdda_pause(void)
{
	if (status!=CD_STATUS_PLAYING) return ERR_SYS;
	aica_stop(cdda_chn+0);
	aica_stop(cdda_chn+1);
	status = CD_STATUS_PAUSED;
	return ERR_OK;
}

int fake_cdda_stop(void)
{
	if (status==CD_STATUS_PLAYING) {
		aica_stop(cdda_chn+0);
		aica_stop(cdda_chn+1);
	}
	status = CD_STATUS_STANDBY;
	return ERR_OK;
}

int fake_cdda_volume(int lvol,int rvol)
{
	vol_l = lvol;
	vol_r = rvol;
	aica_vol(cdda_chn+0,lvol);
	aica_vol(cdda_chn+1,rvol);
	return ERR_OK;
}

/* must call each <0.2 sec*/
int fake_cdda_update(void)
{
	int pos;

	if (status!=CD_STATUS_PLAYING) return ERR_OK;

	pos = aica_get_pos(cdda_chn);
	if (pos<(sampsize/2)) {
		if (writed_pos==0) update(1);
	} else {
		if (writed_pos) update(0);
	}

	return ERR_OK;
}

static void update(int bufno)
{
	int nbyte;
	void *buf;

	if (current_sector>=end_sector) {
		if (loopcnt==LOOP_INFINITE || --loopcnt) {
			current_sector = start_sector;
		} else {
			fake_cdda_stop();
			return;
		}
	}

	nbyte = sampsize*2;
	buf = alloca(nbyte);

	cdda_read_sectors(buf,current_sector,n_sec); current_sector+=n_sec;
	spu_memload_stereo16(snd_l+bufno*sampsize,snd_r+bufno*sampsize,buf,nbyte);
	writed_pos = bufno*(sampsize/2);
}
