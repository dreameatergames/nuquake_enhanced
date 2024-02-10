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
// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.

/*
	(c) 2002 BERO 
	based on cd_win.c
*/

#include <dc/cdrom.h>
#include <dc/spu.h>

#include "fake_cdda.h"

#include "quakedef.h"

extern	cvar_t	bgmvolume;

static qboolean cdValid = false;
static qboolean	playing = false;
static qboolean	wasPlaying = false;
static qboolean	initialized = false;
static qboolean	enabled = false;
static qboolean playLooping = false;
static float	cdvolume;
static byte 	remap[100];
static byte		cdrom;
static byte		playTrack;
static byte		maxTrack;
static qboolean	fake_cdda = false;

static CDROM_TOC toc;
#define	TRACK_CDDA	0

static void CDAudio_SetVolume (float volume)
{
	int vol = volume*255;

	Con_Printf("CDAudio: setvolume %d\n", vol);

	if (fake_cdda)
		fake_cdda_volume(vol,vol);
	else
		spu_cdda_volume(vol,vol);
}

static void CDAudio_Eject(void)
{
}


static void CDAudio_CloseDoor(void)
{
}

static int CDAudio_GetAudioDiskInfo(void)
{
	int ret;
	int track;
	int da_tracks;

	cdValid = false;

	ret = cdrom_read_toc(&toc,0);
	if (ret!=ERR_OK) {
		Con_Printf("cdrom_read_toc failed (%i)",ret);
		return -1;
	}

	da_tracks = 0;
	for(track=TOC_TRACK(toc.first);track<=TOC_TRACK(toc.last);track++) {
		if (TOC_CTRL(toc.entry[track-1])==TRACK_CDDA) da_tracks++;
	}

	if (da_tracks == 0)
	{
		Con_Printf("CDAudio: no music tracks\n");
		return -1;
	}

	cdValid = true;
	maxTrack = TOC_TRACK(toc.last);

	return 0;
}


void CDAudio_Play(byte track, qboolean looping)
{
	int ret;

	if (!enabled)
		return;
	
	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
			return;
	}

	track = remap[track];

	if (track < 1 || track > maxTrack)
	{
		Con_Printf("CDAudio: Bad track number %u.\n", track);
		return;
	}

	// don't try to play a non-audio track
	if (TOC_CTRL(toc.entry[track-1])!=TRACK_CDDA) {
		Con_Printf("CDAudio: track %i is not audio\n", track);
		return;
	}

	if (playing)
	{
		if (playTrack == track)
			return;
		CDAudio_Stop();
	}

	if (bgmvolume.value != cdvolume)
	{
		CDAudio_SetVolume(cdvolume = bgmvolume.value);
	}

	if (cdvolume == 0.0) return;

	if (fake_cdda) {
		int start_sec,end_sec;
		start_sec = TOC_LBA(toc.entry[track-1]);
		end_sec = TOC_LBA((track==TOC_TRACK(toc.last)?toc.leadout_sector:toc.entry[track]));
		ret = fake_cdda_play(start_sec,end_sec,looping?1:15);
	} else {
		ret = cdrom_cdda_play(track,track,looping?1:15,CDDA_TRACKS);
	}
	if (ret!=ERR_OK) {
		Con_Printf("cdrom_cdda_play failed(%i)", ret);
		return;
	}

	playLooping = looping;
	playTrack = track;
	playing = true;

	Con_Printf("CDAudio_Play(%d %d)\n", track,looping);
}


void CDAudio_Stop(void)
{
	int ret;

	if (!enabled)
		return;
	
	if (!playing)
		return;

	if (fake_cdda)
		ret = fake_cdda_stop();
	else
		ret = cdrom_spin_down();
	if (ret!=ERR_OK) {
		Con_Printf("cdrom_spin_down failed(%i)", ret);
	}

	wasPlaying = false;
	playing = false;
}


void CDAudio_Pause(void)
{
	int ret;

	if (!enabled)
		return;

	if (!playing)
		return;

	if (fake_cdda)
		ret = fake_cdda_pause();
	else
		ret = cdrom_cdda_pause();
	if (ret!=ERR_OK) {
		Con_Printf("cdrom_cdda_pause failed(%i)", ret);
	}

	wasPlaying = playing;
	
	playing = false;
}


void CDAudio_Resume(void)
{
	int ret;

	if (!enabled)
		return;
	
	if (!cdValid)
		return;

	if (!wasPlaying)
		return;

	if (fake_cdda)
		ret = fake_cdda_resume();
	else
		ret = cdrom_cdda_resume();
	if (ret!=ERR_OK) {
		Con_Printf("cdrom_cdda_resume failed(%i)", ret);
		return;
	}

	playing = true;
}


static void CD_f (void)
{
	char	*command;
	int		ret;
	int		n;
	int		startAddress;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv (1);

	if (strcasecmp(command, "on") == 0)
	{
		enabled = true;
		return;
	}

	if (strcasecmp(command, "off") == 0)
	{
		if (playing)
			CDAudio_Stop();
		enabled = false;
		return;
	}

	if (strcasecmp(command, "reset") == 0)
	{
		enabled = true;
		if (playing)
			CDAudio_Stop();
		for (n = 0; n < 100; n++)
			remap[n] = n;
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if (strcasecmp(command, "remap") == 0)
	{
		ret = Cmd_Argc() - 2;
		if (ret <= 0)
		{
			for (n = 1; n < 100; n++)
				if (remap[n] != n)
					Con_Printf("  %u -> %u\n", n, remap[n]);
			return;
		}
		for (n = 1; n <= ret; n++)
			remap[n] = atoi(Cmd_Argv (n+1));
		return;
	}

	if (strcasecmp(command, "close") == 0)
	{
		CDAudio_CloseDoor();
		return;
	}

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
		{
			Con_Printf("No CD in player.\n");
			return;
		}
	}

	if (strcasecmp(command, "play") == 0)
	{
		CDAudio_Play((byte)atoi(Cmd_Argv (2)), false);
		return;
	}

	if (strcasecmp(command, "loop") == 0)
	{
		CDAudio_Play((byte)atoi(Cmd_Argv (2)), true);
		return;
	}

	if (strcasecmp(command, "stop") == 0)
	{
		CDAudio_Stop();
		return;
	}

	if (strcasecmp(command, "pause") == 0)
	{
		CDAudio_Pause();
		return;
	}

	if (strcasecmp(command, "resume") == 0)
	{
		CDAudio_Resume();
		return;
	}

	if (strcasecmp(command, "eject") == 0)
	{
		if (playing)
			CDAudio_Stop();
		CDAudio_Eject();
		cdValid = false;
		return;
	}

	if (strcasecmp(command, "info") == 0)
	{
		Con_Printf("%u tracks\n", maxTrack);
		if (playing)
			Con_Printf("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		else if (wasPlaying)
			Con_Printf("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		Con_Printf("Volume is %f\n", cdvolume);
		return;
	}
}

void CDAudio_Update(void)
{
	if (!enabled)
		return;

	if (bgmvolume.value != cdvolume)
	{
		if (cdvolume==0) {
			CDAudio_Resume();
		} else if (bgmvolume.value==0) {
			CDAudio_Pause ();
		}
		CDAudio_SetVolume(cdvolume = bgmvolume.value);
	}

	if (!playing) return;
	
	if (fake_cdda) {
		fake_cdda_update();
	} else {
#if 0
		int status,disc_type,ret;
		ret = cdrom_get_status(&status,&disc_type);
		if (ret!=ERR_OK) {
			Con_Printf("cdrom_get_status failed(%i)\n", ret);
		} else {
			if (status!=CD_STATUS_PLAYING) {
			//	Con_Printf("Replay %i %d %d\n",status,playTrack,playLooping);
				cdrom_cdda_play(playTrack,playTrack,playLooping?1:15,CDDA_TRACKS);
			} else {
			//	Con_Printf("Playing\n");
			}
		}
#endif
	}
}


int CDAudio_Init(void)
{
	if(initialized){
		return 0;
	}
	
	int n;
	Sys_Printf("CD Audio Start Init!\n");
	
	if (cls.state == ca_dedicated)
		return -1;

	if (COM_CheckParm("-nocdaudio"))
		return -1;
	
	for (n = 0; n < 100; n++)
		remap[n] = n;
	initialized = true;
	enabled = true;

	if (CDAudio_GetAudioDiskInfo())
	{
		Con_Printf("CDAudio_Init: No CD in player.\n");
		cdValid = false;
	}

	Cmd_AddCommand ("cd", CD_f);

	if (fake_cdda)
		fake_cdda_init();

	Con_Printf("CD Audio Initialized\n");

	return 0;
}


void CDAudio_Shutdown(void)
{
	if (!initialized)
		return;
	CDAudio_Stop();
}
