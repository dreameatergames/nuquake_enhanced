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
#include <dc/cdrom.h>
#include <dc/spu.h>

#include "quakedef.h"

extern cvar_t bgmvolume;

static qboolean cdValid = false;
qboolean cdPlaying = false;
static qboolean initialized = false;
static qboolean playLooping = false;
static float cdvolume;
static byte playTrack;
static byte maxTrack;

static CDROM_TOC toc;
#define TRACK_CDDA 0
static qboolean isGDROM = false;

static void CDAudio_SetVolume(float volume)
{
	char vol = (int)(volume * 15) & 0xf;
	cdvolume = volume;

	Con_Printf("CDAudio: setvolume %d\n", vol);

	spu_cdda_volume(vol, vol);
}

static int CDAudio_GetAudioDiskInfo(void)
{
	int ret;
	unsigned int track;
	int da_tracks;

	cdValid = false;

	ret = cdrom_read_toc(&toc, isGDROM);
	if (ret != ERR_OK)
	{
		Con_Printf("cdrom_read_toc failed (%i)", ret);
		return -1;
	}

	da_tracks = 0;
	for (track = TOC_TRACK(toc.first); track <= TOC_TRACK(toc.last); track++)
	{
		if (TOC_CTRL(toc.entry[track - 1]) == TRACK_CDDA)
			da_tracks++;
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

	if (track < 1 || track > maxTrack)
	{
		Con_Printf("CDAudio: Bad track number %u.\n", track);
		return;
	}

	// don't try to play a non-audio track
	if (TOC_CTRL(toc.entry[track - 1]) != TRACK_CDDA)
	{
		Con_Printf("CDAudio: track %i is not audio\n", track);
		return;
	}

	if (cdPlaying)
	{
		if (playTrack == track)
			return;
		CDAudio_Stop();
	}

	if (bgmvolume.value != cdvolume)
	{
		CDAudio_SetVolume(cdvolume = bgmvolume.value);
	}

	if (cdvolume == 0.0)
		return;


	ret = cdrom_cdda_play(track, track, looping ? 1 : 15, CDDA_TRACKS);

	if (ret != ERR_OK)
	{
		Con_Printf("cdrom_cdda_play failed(%i)", ret);
		return;
	}

	playLooping = looping;
	playTrack = track;
	cdPlaying = true;

	Con_Printf("CDAudio_Play(%d %d)\n", track, looping);
}

void CDAudio_Stop(void)
{
	int ret;

	if (!cdPlaying)
		return;

	ret = cdrom_spin_down();

	if (ret != ERR_OK)
	{
		Con_Printf("cdrom_spin_down failed(%i)", ret);
	}

	cdPlaying = false;
}

void CDAudio_Pause(void)
{
	int ret;

	if (!cdPlaying)
		return;

	ret = cdrom_cdda_pause();

	if (ret != ERR_OK)
	{
		Con_Printf("cdrom_cdda_pause failed(%i)", ret);
	}

	cdPlaying = false;
}

void CDAudio_Resume(void)
{
	int ret;

	if (!cdValid)
		return;

	ret = cdrom_cdda_resume();
	if (ret != ERR_OK)
	{
		Con_Printf("cdrom_cdda_resume failed(%i)", ret);
		return;
	}

	cdPlaying = true;
}

static void CD_f(void)
{
	char *command;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv(1);

	if (strcasecmp(command, "on") == 0)
	{
		return;
	}

	if (strcasecmp(command, "off") == 0)
	{
		if (cdPlaying)
			CDAudio_Stop();
		bgmvolume.value = 0.0f;
		return;
	}

	if (strcasecmp(command, "reset") == 0)
	{
		if (cdPlaying)
			CDAudio_Stop();

		CDAudio_GetAudioDiskInfo();
		return;
	}

	if (strcasecmp(command, "close") == 0)
	{
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
		CDAudio_Play((byte)atoi(Cmd_Argv(2)), false);
		return;
	}

	if (strcasecmp(command, "loop") == 0)
	{
		CDAudio_Play((byte)atoi(Cmd_Argv(2)), true);
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
		if (cdPlaying)
			CDAudio_Stop();
		cdValid = false;
		return;
	}

	if (strcasecmp(command, "info") == 0)
	{
		Con_Printf("%u tracks\n", maxTrack);
		if (cdPlaying)
			Con_Printf("Currently %s track %u\n", playLooping ? "looping" : "cdPlaying", playTrack);

		Con_Printf("Volume is %f\n", cdvolume);
		return;
	}
}

void CDAudio_Update(void)
{
}

int CDAudio_Init(void)
{
	if (initialized)
	{
		return 0;
	}

	Sys_Printf("CD Audio Start Init!\n");

	if (cls.state == ca_dedicated)
		return -1;

	initialized = true;

	int status = -1, disc_type = -1;
	do
	{
		cdrom_get_status(&status, &disc_type);

		if (status == CD_STATUS_PAUSED ||
			status == CD_STATUS_STANDBY ||
			status == CD_STATUS_PLAYING)
		{
			break;
		}
	} while (1);

	if (disc_type == CD_GDROM)
	{
		isGDROM = true;
		if (CDAudio_GetAudioDiskInfo())
		{
			Con_Printf("CDAudio_Init: No GD-ROM in player.\n");
			cdValid = false;
		}
	}
	else
	{
		isGDROM = false;
		if (CDAudio_GetAudioDiskInfo())
		{
			Con_Printf("CDAudio_Init: No CD in player.\n");
			cdValid = false;
		}
	}

	Cmd_AddCommand("cd", CD_f);
	Con_Printf("CD Audio Initialized\n");

	return 0;

	/*
	cdrom->numtracks = TOC_TRACK(toc.last)-TOC_TRACK(toc.first)+1;
	for(i=0;i<cdrom->numtracks;i++) {
		unsigned long entry = toc.entry[i];
		cdrom->track[i].id = i+1;
		cdrom->track[i].type = (TOC_CTRL(toc.entry[i])==TRACK_CDDA)?SDL_AUDIO_TRACK:SDL_DATA_TRACK;
		cdrom->track[i].offset = TOC_LBA(entry)-150;
		cdrom->track[i].length = TOC_LBA((i+1<toc.last)?toc.entry[i+1]:toc.leadout_sector)-TOC_LBA(entry);
	}
	*/
}

void CDAudio_Shutdown(void)
{
	if (!initialized)
		return;
	CDAudio_Stop();
}