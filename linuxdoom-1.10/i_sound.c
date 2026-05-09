#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <math.h>

#include "raylib.h"

#include "z_zone.h"
#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "doomdef.h"


static AudioStream main_stream;

#define SAMPLERATE		11025	// Hz
#define SAMPLESIZE		2   	// 16bit

void I_SetChannels() {
    // had set up some conversion tables for volume and un/signed samples
    printf("I_SetChannels\n");
}	
 
void I_SetSfxVolume(int volume) {
    // apply the volume settings from the menu
    printf("I_SetSfxVolume(%d)\n", volume);
}

void I_SetMusicVolume(int volume) {
    // apply the volume settings from the menu
    printf("I_SetMusicVolume(%d)\n", volume);
}

//
// Retrieve the raw data lump index
//  for a given SFX name.
//
int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
    char namebuf[12];
    sprintf(namebuf, "ds%s", sfx->name);
    return W_GetNumForName(namebuf);
}

int
I_StartSound
( int		id,
  int		vol,
  int		sep,
  int		pitch,
  int		priority )
{
    printf("I_StartSound(%d,%d,%d,%d,%d)\n", id, vol, sep, pitch, priority);
    return 0;
}

void I_StopSound (int handle) {
    printf("I_StopSound(%d)\n", handle);
}

int I_SoundIsPlaying(int handle) {
    printf("I_SoundIsPlaying(%d)\n", handle);
    return 0;
}

void I_UpdateSound( void ) {
    // would do mixing of a certain amount of samples. Called by d_main if SNDSERV not defined
    //printf("I_UpdateSound()\n");
}


void I_SubmitSound(void) {
    // output sound synchronously, called by d_main if SNDINTR not defined
    //write(audio_fd, mixbuffer, SAMPLECOUNT*BUFMUL);
    //printf("I_SubmitSound()\n");
}

void
I_UpdateSoundParams
( int	handle,
  int	vol,
  int	sep,
  int	pitch)
{
    // called by s_sound
    printf("I_UpdateSoundParams(%d,%d,%d,%d)\n", handle, vol, sep, pitch);
}

void I_ShutdownSound(void) {
    // called by I_Quit
    CloseAudioDevice();
}


void I_InitSound() {

    fprintf( stderr, "I_InitSound: SAMPLERATE=%d sampleSize=%d channels=%d ... ", SAMPLERATE, SAMPLESIZE, 2);

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(4096);
    main_stream = LoadAudioStream(SAMPLERATE, SAMPLESIZE, 2);

    if (IsAudioStreamValid(main_stream)) {
        fprintf( stderr, "bingo.\n");
    }
    else {
        fprintf( stderr, "didn't work out.\n");
    }
  
    // Finished initialization.
    fprintf(stderr, "I_InitSound: sound module ready\n");

}




//
// MUSIC API.
// Still no music done.
// Remains. Dummies.
//
void I_InitMusic(void) {
    printf("I_InitMusic()\n");
}

void I_ShutdownMusic(void)	{
    printf("I_ShutdownMusic()\n");
}

static int	looping=0;
static int	musicdies=-1;

void I_PlaySong(int handle, int looping)
{
    printf("I_PlaySong(%d,%d)\n", handle, looping);
  // UNUSED.
  handle = looping = 0;
  musicdies = gametic + TICRATE*30;
}

void I_PauseSong (int handle)
{
    printf("I_PauseSong(%d)\n", handle);
  // UNUSED.
  handle = 0;
}

void I_ResumeSong (int handle)
{
    printf("I_ResumeSong(%d)\n", handle);
  // UNUSED.
  handle = 0;
}

void I_StopSong(int handle)
{
    printf("I_StopSong(%d)\n", handle);
  // UNUSED.
  handle = 0;
  
  looping = 0;
  musicdies = 0;
}

void I_UnRegisterSong(int handle)
{
    printf("I_UnRegisterSong(%d)\n", handle);
  // UNUSED.
  handle = 0;
}

int I_RegisterSong(void* data)
{
    printf("I_RegisterSong(%p)\n", data);
  // UNUSED.
  data = NULL;
  
  return 1;
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
    printf("I_QrySongPlaying(%d)\n", handle);
  // UNUSED.
  handle = 0;
  return looping || musicdies > gametic;
}
