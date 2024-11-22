/*
  Some of this may not work. I'm not overly familiar with SDL, I just sort
  of podged this together from the SDL headers, and the other cd-rom code.

  Mark Baker <homer1@together.net>
  
  Debug and testing and fixing Ian micheal 2019
*/

#include <SDL/SDL.h>
#include <kos.h> 
#include "quakedef.h"


//void			cdda_loop_check(void);
int 			cdda_get_disk_info(void);

static qboolean cdValid = true;
static qboolean initialized = true;
static qboolean enabled = true;
static qboolean playLooping = false;
static qboolean wasPlaying = false;
static SDL_CD *cd_id;
//static float cdvolume = 1.0;
static float cdvolume;
static void CD_f();

static void CDAudio_SetVolume(float volume) {
  int vol = volume * 255;

  Con_Printf("CDAudio: setvolume %d\n", vol);


}

void CDAudio_Play(byte track, qboolean looping)
{
     cd_id = SDL_CDOpen(0);
	if(!cd_id || !enabled) return;

   	CDstatus cd_stat = SDL_CDStatus(cd_id);
	
	if(!cdValid)
	{
	if(!CD_INDRIVE(cd_stat=SDL_CDStatus(cd_id)) ||(!cd_id->numtracks)) return;
		if(!CD_INDRIVE(cd_stat) ||(!cd_id->numtracks)) return;
	//	cdValid = true;
	}
	if((track < 1) || (track >= cd_id->numtracks))
	{
		Con_DPrintf("CDAudio: Bad track number: %d\n",track);
		return;
	}

	track--; /* Convert track from person to SDL value */
	 //if (playTrack == track)
	if(cd_stat == CD_PLAYING)
	{
		if(cd_id->cur_track == track) return;
		CDAudio_Stop();
	}

  if (bgmvolume.value != cdvolume) {
    CDAudio_SetVolume(cdvolume = bgmvolume.value);
  }

  if (cdvolume == 0.0) return;

		if(SDL_CDPlay(cd_id,cd_id->track[track].offset,
			  cd_id->track[track].length))
	{
		Con_DPrintf("CDAudio_Play: Unable to play track: %d\n",track+1);
		return;
	}
//	cd_stat = CD_PLAYING
	playLooping = looping;
		/* Wait 10 seconds */
	printf("Waiting 10 seconds\n");
	SDL_Delay(10*1000);
}





void CDAudio_Update()
{
	if(!cd_id || !enabled) return;
	if(bgmvolume.value != cdvolume)
	{
		if(cdvolume)
		{
			Cvar_SetValue("bgmvolume",0.0);
			CDAudio_Pause();
		}
		else
		{
			Cvar_SetValue("bgmvolume",1.0);
			CDAudio_Resume();
		}
		cdvolume = bgmvolume.value;
		return;
	}
	if(playLooping && (SDL_CDStatus(cd_id) != CD_PLAYING)
		 && (SDL_CDStatus(cd_id) != CD_PAUSED))
		CDAudio_Play(cd_id->cur_track+1,true);
}

int CDAudio_Init()
{
    
      /* Init SDL: */
   cd_id = SDL_CDOpen(0);
 
	initialized = true;
	enabled = true;
	cdValid = true;
	
	if(!CD_INDRIVE(SDL_CDStatus(cd_id)))
	{
	
	}
	if(!cd_id->numtracks)
	{
	
	}
	SDL_Delay(3000);
	Cmd_AddCommand("cd",CD_f);
	Con_Printf("CD Audio Initialized.\n");
	return 0;
}

void CDAudio_Stop()
{
	int cdstate;
	if(!cd_id || !enabled) return;
	cdstate = SDL_CDStatus(cd_id);
	if((cdstate != CD_PLAYING) && (cdstate != CD_PAUSED)) return;
wasPlaying = false;
	if(SDL_CDStop(cd_id))
		Con_DPrintf("CDAudio_Stop: Failed to stop track.\n");
}



void CDAudio_Resume()
{
      if (!wasPlaying)
    return;
	if(!cd_id || !enabled) return;
	if(SDL_CDStatus(cd_id) != CD_PAUSED) return;
      wasPlaying = false;
	if(SDL_CDResume(cd_id))
		Con_DPrintf("CDAudio_Resume: Failed tp resume track.\n");
}
void CDAudio_Shutdown()
{
	if(!cd_id) return;
	CDAudio_Stop();
	SDL_CDClose(cd_id);
	cd_id = NULL;
}


void CDAudio_Pause()
{
      
	if(!cd_id || !enabled) return;
	if(SDL_CDStatus(cd_id) != CD_PLAYING) return;
if (!CD_PLAYING)return;
	if(SDL_CDPause(cd_id))
		Con_DPrintf("CDAudio_Pause: Failed to pause track.\n");
		  wasPlaying = CD_PLAYING;
  //CD_PLAYING = false;
}




static void CD_f()
{
	char *command;
	int cdstate;
	if(Cmd_Argc() < 2) return;

	command = Cmd_Argv(1);
	if(!strcasecmp(command,"on"))
	{
		enabled = true;
	}
	if(!strcasecmp(command,"off"))
	{
		if(!cd_id) return;
		cdstate = SDL_CDStatus(cd_id);
		if((cdstate == CD_PLAYING) || (cdstate == CD_PAUSED))
			CDAudio_Stop();
		enabled = false;
		return;
	}
	if(!strcasecmp(command,"play"))
	{
		CDAudio_Play(atoi(Cmd_Argv(2)),false);
		return;
	}
	if(!strcasecmp(command,"loop"))
	{
		CDAudio_Play(atoi(Cmd_Argv(2)),true);
		return;
	}
	if(!strcasecmp(command,"stop"))
	{
		CDAudio_Stop();
		return;
	}
	if(!strcasecmp(command,"pause"))
	{
		CDAudio_Pause();
		return;
	}
	if(!strcasecmp(command,"resume"))
	{
		CDAudio_Resume();
		return;
	}
	if(!strcasecmp(command,"info"))
	{
		if(!cd_id) return;
		cdstate = SDL_CDStatus(cd_id);
		Con_Printf("%d tracks\n",cd_id->numtracks);
		if(cdstate == CD_PLAYING)
			Con_Printf("Currently %s track %d\n",
				playLooping ? "looping" : "playing",
				cd_id->cur_track+1);
		else
		if(cdstate == CD_PAUSED)
			Con_Printf("Paused %s track %d\n",
				playLooping ? "looping" : "playing",
				cd_id->cur_track+1);
		return;
	}
}
