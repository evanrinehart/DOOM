#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include <unistd.h>

#include "raylib.h"

#include "adlmidi.h"

#include "z_zone.h"
#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "doomdef.h"


static AudioStream main_stream;
static struct ADL_MIDIPlayer *midi_player = NULL;

//#define SAMPLERATE		11025	// Hz
#define SAMPLERATE		48000	// Hz
#define SAMPLESIZE		2   	// 16bit

float boost_max = 3;
float boost = 3;
int music_playing = 0;
int music_looping = 0;

struct sized_blob {
    void *data;
    size_t len;
};

struct sized_blob registered_song = {NULL};

static int callback_should_reload = 0;
static int callback_should_end = 0;


void audio_callback(void *bufferData, unsigned int num_frames) {

    if (callback_should_reload) {
        void *data = registered_song.data;
        size_t len = registered_song.len;

        if (data == NULL) {
            printf("I_PlaySong: no song ready\n");
        }

        if (adl_openData(midi_player, data, len) < 0) {
            printf("I_PlaySong: adl_openData failed\n");
        }
        else {
            adl_setLoopEnabled(midi_player, music_looping);
        }

        callback_should_reload = 0;
        music_playing = 1;
    }

    struct ADLMIDI_AudioFormat format;

    format.type = ADLMIDI_SampleType_S16;
    format.containerSize = sizeof (int16_t);
    format.sampleOffset = sizeof (int16_t) * 2;

    unsigned num_samples = num_frames * 2; // stereo;

    for (unsigned i = 0; i < num_samples; i++) {
        short *s = bufferData + 2*i;
        *s = 0;
    }

    if (callback_should_end) {
        callback_should_end = 2;
        return;
    }

    if (!music_playing) return; // a nicer cutoff would be nicer

    int n = adl_playFormat(midi_player, (int)num_samples, bufferData, bufferData + 2, &format);
    if (n == 0) {
        // end of song
    }
    else if (n < 0) {
        printf("audio_callback: adl_playFormat error\n");
    }

    for (unsigned i = 0; i < num_samples; i++) {
        short *s = bufferData + 2*i;
        float value = *s;
        value *= boost;
        if (value > 32767) value = 32767;
        if (value < -32767) value = -32767;
        *s = (short)value;
    }
}



void I_SetChannels() {
    // had set up some conversion tables for volume and un/signed samples
}	
 
void I_SetSfxVolume(int volume) {
    // apply the volume settings from the menu
}

void I_SetMusicVolume(int volume) {
    if (volume > 15) return;

    // apply the volume settings from the menu
    boost = (float)volume / 15 * boost_max;
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


static Sound sound_library[256];
static Sound sound_slot[64];
static bool sound_slot_full[64] = {false};

int
I_StartSound
( int		id,
  int		vol,
  int		sep,
  int		pitch,
  int		priority )
{

    int handle = -1;
    for (int i = 1; i < 64; i++) {
        if (!sound_slot_full[i]) { handle = i; break; }
    }

    if (handle < 0) {
        printf("too many sounds?\n");
        return 0;
    }

    Sound alias = LoadSoundAlias(sound_library[id]);
    SetSoundVolume(alias, (float)vol / 15);
    SetSoundPan(alias, (float)(sep - 128) / 128);

    sound_slot[handle] = alias;
    sound_slot_full[handle] = true;

    PlaySound(sound_slot[handle]);

    return handle;
}

void I_StopSound (int handle) {
    if (!sound_slot_full[handle]) return;
    StopSound(sound_slot[handle]);
    UnloadSoundAlias(sound_slot[handle]);
    sound_slot_full[handle] = false;
}

int I_SoundIsPlaying(int handle) {
    if (!sound_slot_full[handle]) return 0;
    if (!IsSoundPlaying(sound_slot[handle])) {
        UnloadSoundAlias(sound_slot[handle]);
        sound_slot_full[handle] = false;
        return 0;
    }
    else {
        return 1;
    }
}

void I_UpdateSound( void ) {
    // would do mixing of a certain amount of samples. Called by d_main if SNDSERV not defined
}

void I_SubmitSound(void) {
    // output sound synchronously, called by d_main if SNDINTR not defined
}

void
I_UpdateSoundParams
( int	handle,
  int	vol,
  int	sep,
  int	pitch)
{
    // called by s_sound
    //printf("I_UpdateSoundParams(%d,%d,%d,%d)\n", handle, vol, sep, pitch);

    if (!sound_slot_full[handle]) return;

    SetSoundVolume(sound_slot[handle], (float)vol / 15);
    SetSoundPan(sound_slot[handle], (float)(sep - 128) / 128);
}

void I_ShutdownSound(void) {
    // called by I_Quit

    for (int i = 0; i < 64; i++) {
        if (sound_slot_full[i]) {
            UnloadSoundAlias(sound_slot[i]);
        }
    }

    for (int i = 0; i < NUMSFX; i++) {
        if (i == 0) continue;
        UnloadSound(sound_library[i]);
    }

}

Wave load_sound_from_wad(char *name) {
    int lumpnum;
    int lumpsize;

    if (W_CheckNumForName(name) == -1) {
        lumpnum = W_GetNumForName("dspistol");
    }
    else {
        lumpnum = W_GetNumForName(name);
    }

    lumpsize = W_LumpLength(lumpnum);

    unsigned char *data = W_CacheLumpNum(lumpnum, PU_STATIC);
    int format = *((short*)(data + 0));
    int srate = *((short*)(data + 2));
    int num_samples = *((int*)(data + 4)) - 32;

    Wave w = {0,0,0,0,NULL};
    if (format != 3) {
        printf("bad format in sfx lump header\n");
        return w;
    }

    if (num_samples < 0) {
        printf("bad number of samples in sfx lump header\n");
        return w;
    }

    if (srate == -21436) { // epic HAX
        srate = 44100;
    }

    if (srate < 0) {
        printf("bad sample rate in sfx lump header\n");
        return w;
    }

    unsigned char *samples = Z_Malloc(num_samples, PU_STATIC, 0);

    //printf("%s %d %d %d %p\n", name, format, srate, num_samples, samples);
    memcpy(samples, data + 24, (size_t)num_samples);
    Z_Free(data);

    w.frameCount = (unsigned) num_samples;
    w.sampleRate = (unsigned) srate;
    w.sampleSize = 8;
    w.channels = 1;
    w.data = samples;

    return w;
}


void I_InitSound() {

    printf("I_InitSound...\n");

    InitAudioDevice();

    // preload all the sounds
    //for (int i = 1; i < NUMSFX; i++) {
    for (int i = 0; i < NUMSFX; i++) {
        if (i==0) continue;

        if (i >= 256) {
            printf("too many sounds!\n");
            continue;
        }
        sfxinfo_t *info = &S_sfx[i];
        if (info->link) continue;
        char name[16] = "ds";
        strcat(name, info->name);
        Wave w = load_sound_from_wad(name);
        sound_library[i] = LoadSoundFromWave(w);
        if (IsSoundValid(sound_library[i]) == 0) {
            // FIXME, the sound is invalid and will crash if played, make it not
            printf("failed to load sound \"%s\" from wave\n", name);
        }
    }

    fprintf(stderr, "I_InitSound: sound module ready\n");
}




void I_InitMusic(void) {
    printf("I_InitMusic()\n");

    midi_player = adl_init(SAMPLERATE);
    if (!midi_player) {
        fprintf(stderr, "Couldn't initialize ADLMIDI: %s\n", adl_errorString());
        return;
    }

    printf("   ADLMIDI initialized... switching emulator\n");
    adl_switchEmulator(midi_player, ADLMIDI_EMU_NUKED);
    adl_setVolumeRangeModel(midi_player, ADLMIDI_VolumeModel_Generic);

    fprintf( stderr, "  SAMPLERATE=%d sampleSize=%d channels=%d ... ", SAMPLERATE, SAMPLESIZE, 2);
    SetAudioStreamBufferSizeDefault(4096);
    main_stream = LoadAudioStream(SAMPLERATE, 8*SAMPLESIZE, 2);

    if (IsAudioStreamValid(main_stream)) {
        fprintf( stderr, "bingo.\n");
        SetAudioStreamCallback(main_stream, audio_callback);
        PlayAudioStream(main_stream);
        //SetAudioStreamVolume(main_stream, 1.0);
    }
    else {
        fprintf( stderr, "didn't work out.\n");
    }

}

void I_ShutdownMusic(void)	{
    printf("I_ShutdownMusic()\n");

    if (midi_player == NULL) return;

    callback_should_end = 1;
    while (callback_should_end < 2) {
        I_Sleep(0.01);
    }

    if (registered_song.data) free(registered_song.data);

    //PauseAudioStream(main_stream);
    UnloadAudioStream(main_stream);

    adl_close(midi_player);

    CloseAudioDevice();
}

void I_PlaySong(int handle, int looping)
{
    if (handle != 1) return;

    if (registered_song.data == NULL) {
        printf("I_PlaySong: song not registered!\n");
        return;
    }

    music_looping = looping;
    callback_should_reload = 1;
}

void I_PauseSong (int handle)
{
    if (handle != 1) return;

    music_playing = 0;
}

void I_ResumeSong (int handle)
{
    if (handle != 1) return;

    music_playing = 1;
}

void I_StopSong(int handle)
{
    if (handle != 1) return;

    music_playing = 0;
}

void I_UnRegisterSong(int handle)
{
    if (handle != 1) return;

    if (registered_song.data == NULL) {
        printf("I_UnRegisterSong: song not registered!\n");
        return;
    }

    music_playing = 0;
    free(registered_song.data);
    registered_song.data = NULL;
}

int I_RegisterSong(void* data, size_t len)
{
    unsigned char *buffer = malloc(len);
    memcpy(buffer, data, len);

    if (registered_song.data) free(registered_song.data);

    registered_song.len = len;
    registered_song.data = buffer;
  
    return 1;
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
    if (handle != 1) return 0;
    else return music_playing;
}
