/*
 * Filename: d:\Dev\Dreamcast\UB_SHARE\clean_nuQuake\src\dreamcast\snd_dreamcast.c
 * Path: d:\Dev\Dreamcast\UB_SHARE\clean_nuQuake\src\dreamcast
 * Created Date: Friday, January 31st 2020, 11:42:09 am
 * Author: Hayden Kowalchuk
 * 
 * Copyright (c) 2020 HaydenKow
 */

#include <AL/al.h>
#include <AL/alc.h>
#include "quakedef.h"

// 64K is > 1 second at 16-bit, 22050 Hz
#define WAV_BUFFERS 64
#define WAV_BUFFER_SIZE 0x0800
#define SECONDARY_BUFFER_SIZE (WAV_BUFFERS * WAV_BUFFER_SIZE + 0x2000)

typedef enum { SIS_SUCCESS,
               SIS_FAILURE,
               SIS_NOTAVAIL } sndinitstat;

static qboolean wavonly;
static qboolean wav_init;
static qboolean snd_firsttime = true, snd_iswave;

static int sample16;
static int snd_sent;
extern int new_wav_buffer_written;

/*
 * Global variables. Must be visible to window-procedure function
 *  so it can unlock and free the data block after it has been played.
 */
int hWaveOut = 0;
int hWaveHdr = 0;
int gSndBufSize;
unsigned char *lpData;
unsigned char *hData;
unsigned char *addr_out;
static ALuint wav_buffer[WAV_BUFFERS];

ALCenum error;

ALint iTotalBuffersProcessed;

ALenum format = AL_FORMAT_STEREO16;

ALuint snd_source;
ALboolean enumeration;
const ALCchar *defaultDeviceName = NULL;
ALCdevice *device;
ALCcontext *context;
ALsizei size;

ALfloat listenerOri[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};
ALint source_state;

qboolean SNDDMA_InitWav(void);

#define AMPLITUDE 16384
#define INCREMENT 1.0f / 100.0f
static float pos = 0.0f;
/* Generate a nice pretty sine wave for some simple audio. */
void audio_callback(void *userdata, uint8_t *stream, int length) {
  (void)userdata;
  uint16_t *out = (uint16_t *)stream;
  int i;

  /* Length is in bytes, we're generating 16 bit samples, so divide the length
       by two to get the number of samples to generate. */
  length >>= 1;

  /* Generate samples on demand. */
  for (i = 0; i < length; i += 2) {
    out[i] = (uint16_t)(fsin(pos * F_PI) * AMPLITUDE);
    out[i + 1] = (uint16_t)(fsin(pos * F_PI) * AMPLITUDE);
    pos += INCREMENT;

    if (pos >= 1.0f)
      pos -= 1.0f;
  }
}

static void list_audio_devices(const ALCchar *devices) {
  const ALCchar *device = devices, *next = devices + 1;
  size_t len = 0;

  fprintf(stdout, "Devices list:\n");
  fprintf(stdout, "----------\n");
  while (device && *device != '\0' && next && *next != '\0') {
    fprintf(stdout, "%s\n", device);
    len = strlen(device);
    device += (len + 1);
    next += (len + 2);
  }
  fprintf(stdout, "----------\n");
}

/*
==================
S_BlockSound
==================
*/
void S_BlockSound(void) {
  if (snd_iswave) {
    //snd_blocked++;

    if (snd_blocked == 1) {
      //waveOutReset (hWaveOut);
    }
  }
}

/*
==================
S_UnblockSound
==================
*/
void S_UnblockSound(void) {
  if (snd_iswave) {
    //snd_blocked--;
  }
}

/*
==================
FreeSound
==================
*/
void FreeSound(void) {
  if (hData) {
    //		waveOutReset (hWaveOut);
#if 0
    if (lpWaveHdr) {
      for (int i = 0; i < WAV_BUFFERS; i++)
      //waveOutUnprepareHeader (hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR));
    }
#endif
    //waveOutClose (hWaveOut);

    //free(hData);
  }

  hData = NULL;
  lpData = NULL;
  wav_init = false;
}

/*
==================
SNDDM_InitWav

Crappy windows multimedia base
==================
*/
qboolean SNDDMA_InitWav(void) {
  snd_sent = 0;
  //snd_completed = 0;

  shm = &sn;

  shm->channels = 2;
  shm->samplebits = 16;
  shm->speed = 11025;
  //shm->speed = 22050; /* ISSUES */

  /* Build OpenAL interface */
  enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
  if (enumeration == AL_FALSE)
    Con_SafePrintf("enumeration extension not available\n");

  list_audio_devices(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

  if (!defaultDeviceName)
    defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

  device = alcOpenDevice(defaultDeviceName);
  if (!device) {
    Con_SafePrintf("unable to open default device\n");
  }

  Con_SafePrintf("Device: %s\n", alcGetString(device, ALC_DEVICE_SPECIFIER));

  context = alcCreateContext(device, NULL);
  if (!alcMakeContextCurrent(context)) {
    Con_SafePrintf("failed to make default context\n");
  }

  /* Default Listener stuff */
  alListener3f(AL_POSITION, 0, 0, 1.0f);
  alListener3f(AL_VELOCITY, 0, 0, 0);
  alListenerfv(AL_ORIENTATION, listenerOri);

  /*
	 * Allocate and lock memory for the waveform data.
	*/
  gSndBufSize = WAV_BUFFERS * WAV_BUFFER_SIZE;
  hData = Hunk_AllocName(SECONDARY_BUFFER_SIZE, "shmbuf");
  if (!hData) {
    Con_SafePrintf("Sound: Out of memory.\n");
    FreeSound();
    return false;
  }
  lpData = hData;
  addr_out = lpData;

  memset(hData, '\0', gSndBufSize);

  /* After allocation, set up and prepare headers. */
  alGenSources((ALuint)1, &snd_source);

  alSourcef(snd_source, AL_PITCH, 1);
  alSourcef(snd_source, AL_GAIN, 1);
  alSource3f(snd_source, AL_POSITION, 0, 0, 0);
  alSource3f(snd_source, AL_VELOCITY, 0, 0, 0);

  alGenBuffers((ALsizei)WAV_BUFFERS, wav_buffer);

  /* Unsure about if we need or not ? */
#if 0
  for (int i = 0; i < 1; i++) {
    alBufferData(wav_buffer[i], AL_FORMAT_STEREO16, lpData + i * WAV_BUFFER_SIZE, WAV_BUFFER_SIZE, shm->speed);
    alSourceQueueBuffers(snd_source, 1, &wav_buffer[i]);
  }
  alSourcePlay(snd_source);
  printf("Started! %f\n", Sys_FloatTime());
#endif

  shm->soundalive = true;
  shm->splitbuffer = false;
  shm->samples = gSndBufSize / (shm->samplebits / 8);
  shm->samplepos = 0;
  shm->submission_chunk = 1;
  shm->buffer = (unsigned char *)lpData;
  sample16 = (shm->samplebits / 8) - 1;

  wav_init = true;

  return true;
}

/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/

qboolean SNDDMA_Init(void) {
  sndinitstat stat;

  if (COM_CheckParm("-wavonly"))
    wavonly = true;

  wav_init = 0;

  stat = SIS_FAILURE;

  // try to initialize waveOut sound
  if ((stat != SIS_NOTAVAIL)) {
    if (snd_firsttime || snd_iswave) {
      snd_iswave = SNDDMA_InitWav();

      if (snd_iswave) {
        if (snd_firsttime)
          Con_SafePrintf("Wave sound initialized\n");
      } else {
        Con_SafePrintf("Wave sound failed to init\n");
      }
    }
  }

  snd_firsttime = false;

  if (!wav_init) {
    if (snd_firsttime)
      Con_SafePrintf("No sound device initialized\n");

    return false;
  }

  return true;
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos(void) {
  int s = snd_sent * WAV_BUFFER_SIZE;

  s >>= sample16;

  s &= (shm->samples - 1);

  return s;
}
void SNDDMA_Submit(void) {
  ALenum source_state;
  ALuint uiBuffer;
  ALint queued;
  int iBuffersProcessed = 0;
  static int buffer_start = 0;
#define BUFFERS_SMOOTH 4

  alGetSourcei(snd_source, AL_BUFFERS_QUEUED, &queued);
  alGetSourcei(snd_source, AL_SOURCE_STATE, &source_state);
  alGetSourcei(snd_source, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

  iTotalBuffersProcessed += iBuffersProcessed;
  int c = 0;
  // For each processed buffer, remove it from the source queue, read the next chunk of
  // audio data from the file, fill the buffer with new data, and add it to the source queue
  if (source_state == AL_PLAYING) {
    while (iBuffersProcessed > 0) {
      // Remove the buffer from the queue (uiBuffer contains the buffer ID for the dequeued buffer)
      uiBuffer = 0;
      alSourceUnqueueBuffers(snd_source, 1, &uiBuffer);
      // Copy audio data to buffer
      alBufferData(uiBuffer, AL_FORMAT_STEREO16, addr_out + (c * WAV_BUFFER_SIZE), WAV_BUFFER_SIZE, shm->speed);
      // Insert the audio buffer to the source queue
      alSourceQueueBuffers(snd_source, 1, &uiBuffer);

      iBuffersProcessed--;
      snd_sent++;
      c++;
      new_wav_buffer_written -= WAV_BUFFER_SIZE;
    }
    if (queued < BUFFERS_SMOOTH) {
      printf("Queued Extra Sound!\n");
      if(uiBuffer == wav_buffer[buffer_start])
        buffer_start++;
      alBufferData(wav_buffer[buffer_start], AL_FORMAT_STEREO16, addr_out + c * WAV_BUFFER_SIZE, WAV_BUFFER_SIZE, shm->speed);
      alSourceQueueBuffers(snd_source, 1, &wav_buffer[buffer_start++]);
      snd_sent++;
    }
  } else {
    while (iBuffersProcessed-- > 0) {
      alSourceUnqueueBuffers(snd_source, 1, &uiBuffer);
    }
    int num_buffs_needed = new_wav_buffer_written / WAV_BUFFER_SIZE;
    if (num_buffs_needed != 0)
      printf("SND_Stopped: Queueing %d buffers\n", num_buffs_needed);
    for (int i = 0; i <= num_buffs_needed; i++) {
      alBufferData(wav_buffer[buffer_start], AL_FORMAT_STEREO16, addr_out + i * WAV_BUFFER_SIZE, WAV_BUFFER_SIZE, shm->speed);
      alSourceQueueBuffers(snd_source, 1, &wav_buffer[buffer_start++]);
      snd_sent++;
    }
    alSourcePlay(snd_source);

    if (queued >= BUFFERS_SMOOTH) {
      //alSourcePlay(snd_source);
      //buffer_start = 0;
    }
  }
#if 0
    alGetSourcei(snd_source, AL_BUFFERS_PROCESSED, &iBuffersProcessed);
    while (iBuffersProcessed--) {
      alSourceUnqueueBuffers(snd_source, 1, buf);
    }
    alSourcei(snd_source, AL_BUFFER, (0));
    alDeleteBuffers((ALsizei)WAV_BUFFERS, wav_buffer);
    alGenBuffers((ALsizei)WAV_BUFFERS, wav_buffer);

    for (int i = 0; i < 4; i++) { /* 4  gets sound but still delay */
      alBufferData(wav_buffer[i], AL_FORMAT_STEREO16, lpData + i * WAV_BUFFER_SIZE, WAV_BUFFER_SIZE, shm->speed);
      alSourceQueueBuffers(snd_source, 1, &wav_buffer[i]);
    }
    alSourcePlay(snd_source);
    memset(lpData, '\0', gSndBufSize);
#endif

  thd_pass();
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown(void) {
  FreeSound();
}
