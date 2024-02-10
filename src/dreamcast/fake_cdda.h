/*

	fake cdda using raw read + spu stream

	(c) 2002 bero

	under GPL or notify me
	this means : if you close source code, notify me , otherwise you can use freely under GPL.

*/

/* you must call after snd_init() */
int fake_cdda_init(void);
/* dmy */
int fake_cdda_shutdown(void);

#define	LOOP_INFINITE	15
int fake_cdda_play(int start_sec,int end_sec,int loop);
int fake_cdda_resume(void);
int fake_cdda_pause(void);
int fake_cdda_stop(void);
int fake_cdda_volume(int lval,int rval); /* 0..255 */
/* must call each <0.2 sec*/
int fake_cdda_update(void);
