#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "mus_file.h"

static const char *gm_instruments[128];
static const char *gm_percussion[47];
static const char *gm2_percussion[6];

static struct mus_chunk *first_chunk = NULL;
static struct mus_chunk *current_chunk = NULL;

struct mus_events {
    struct mus_event *buf;
    size_t pop;
    size_t cap;
};

struct mus_events mus_events = {NULL, 0, 0};

int total = 0;

void append_event(int type, int channel, int data1, int data2) {
    if (mus_events.pop == mus_events.cap) {
        mus_events.buf = realloc(mus_events.buf, mus_events.cap * 2);
        mus_events.cap *= 2;
    }

    struct mus_event *ev = &mus_events.buf[mus_events.pop++];
    ev->type = type;
    ev->channel = channel;
    ev->data1 = data1;
    ev->data2 = data2;
    total++;
};

struct mus_event *copy_events(void) {
    size_t N = mus_events.pop * sizeof (struct mus_event);
    struct mus_event *copy = malloc(N);
    memcpy(copy, mus_events.buf, N);
    return copy;
}


unsigned int getword16(unsigned char *data) {
    unsigned w1 = data[1];
    unsigned w0 = data[0];
    return 256 * w1 + w0;
}


char *encode_controller(int which) {
    static char buf[16];

    switch (which) {
        case 0: return "INST";
        case 1: return "BANK";
        case 2: return "VIBR";
        case 3: return "VOL";
        case 4: return "PAN";
        case 5: return "EXPR";
        case 6: return "REV";
        case 7: return "CHOR";
        case 8: return "SUST";
        case 9: return "SOFT";
    }

    sprintf(buf, "%4d", which);
    return buf;
}

char *show_controller(int which) {
    switch (which) {
        case 0: return "Instrument";
        case 1: return "BankSelect";
        case 2: return "VibratoDepth";
        case 3: return "Volume";
        case 4: return "Pan";
        case 5: return "Expression";
        case 6: return "ReverbDepth";
        case 7: return "ChorusDepth";
        case 8: return "SustainPedal";
        case 9: return "SoftPedal";
        default: return "Unknown";
    }
}

void releasenote(int channel, int note) {
    append_event(0, channel, note, 0);
}

void playnote(int channel, int note) {
    append_event(1, channel, note, -1);
}

void playnote_volume(int channel, int note, int volume) {
    append_event(1, channel, note, volume);
}

void pitchbend(int channel, int value) {
    //printf("PitchBend      %2d %3d\n", channel, value);
    append_event(2, channel, value, 0);
}

void systemevent(int channel, int which) {
    //printf("SystemEvent    %2d %3d\n", channel, which);
    append_event(3, channel, which, 0);
}

void controlchange(int channel, int which, int value) {
    //printf("ControlChange  %2d %3d %3d %s\n", channel, which, value, show_controller(which));
    append_event(4, channel, which, value);
}

void endofmeasure(int channel, int unused) {
    //printf("EndOfMeasure   %2d\n", channel);
    append_event(5, channel, unused, 0);
}

void endofscore(int channel) {
    //printf("EndOfScore     %2d\n", channel);
    append_event(6, channel, 0, 0);

    current_chunk->num_events = mus_events.pop;
    current_chunk->events = copy_events();
}

void unused_event(int channel, int unused) {
    //printf("UnusedEvent    %2d %3d\n", channel, unused);
    append_event(7, channel, unused, 0);
}

void scorewait(long ticks) {

    if (first_chunk == NULL) {
        current_chunk = malloc(sizeof *current_chunk);
        current_chunk->delay = 0;
        current_chunk->num_events = 0;
        current_chunk->events = NULL;
        current_chunk->next = NULL;
        first_chunk = current_chunk;
    }

    struct mus_chunk *chunk = malloc(sizeof *chunk);
    chunk->delay = ticks;
    chunk->num_events = 0;
    chunk->events = NULL;
    chunk->next = NULL;

    current_chunk->num_events = mus_events.pop;
    current_chunk->events = copy_events();
    current_chunk->next = chunk;

    current_chunk = chunk;
    mus_events.pop = 0;
}

int mus_dispatch_event(unsigned char *data, size_t size) {
        int type = data[0] >> 4 & 7;
        int channel = data[0] & 15;

        if (type == 0 && size < 2) return -1;
        if (type == 1 && size < 2) return -1;
        if (type == 1 && data[1] & 128 && size < 3) return -1;
        if (type == 2 && size < 2) return -1;
        if (type == 3 && size < 2) return -1;
        if (type == 4 && size < 3) return -1;
        if (type == 5 && size < 2) return -1;
        if (type == 7 && size < 2) return -1;

        if (type == 0) { releasenote(channel, data[1]); return 2; }
        if (type == 1 && data[1] & 128) { playnote_volume(channel, data[1] & 127, data[2]); return 3; }
        if (type == 1) { playnote(channel, data[1]); return 2; }
        if (type == 2) { pitchbend(channel, data[1]); return 2; }
        if (type == 3) { systemevent(channel, data[1]); return 2; }
        if (type == 4) { controlchange(channel, data[1], data[2]); return 3; }
        if (type == 5) { endofmeasure(channel, data[1]); return 2; }
        if (type == 6) { endofscore(channel); return 1; }
        if (type == 7) { unused_event(channel, data[1]); return 2; }

        return -1;
}

int mus_process_score(unsigned char *data, size_t size) {
    first_chunk = NULL;
    if (mus_events.buf == NULL) {
        mus_events.buf = malloc(256 * sizeof *mus_events.buf);
        mus_events.cap = 256;
    }
    mus_events.pop = 0;

    for (;;) {
        for (;;) {
            if (size == 0) return -1;
            int type = data[0] >> 4 & 7;
            bool last = data[0] >> 7;
            int n = mus_dispatch_event(data, size);
            if (n < 0) return -1;
            data += n;
            size -= n;
            if (type == 6) return 0; // end of score
            if (last) break;
        }

        long delta = 0;
        for (;;) {
            if (size == 0) return -1;
            bool continuing = data[0] >> 7;
            delta = 128 * delta + (data[0] & 127);
            data++;
            size--;
            if (!continuing) break;
        }
        scorewait(delta);
    }
}


int *mus_get_instruments(unsigned num_instruments, unsigned char *data, size_t size) {
        if (num_instruments > 10000) return NULL;
        if (size < num_instruments * 2) return NULL;

        int *store = malloc(num_instruments * sizeof *store);
        if (store == NULL) return NULL;

        for (unsigned i = 0; i < num_instruments; i++) {
                unsigned value = getword16(data);
                if (value > 255) { free(store); return NULL; }
                store[i] = value;
                data += 2;
        }

        return store;
}

const char *GetInstrumentName(int ins) {
    if (ins < 128) return gm_instruments[ins];
    else if (135 <= ins && ins <= 181) return gm_percussion[ins - 135];
    else if (182 <= ins && ins <= 187) return gm2_percussion[ins - 182];
    else return "Unknown";
}

int ReadMUS(struct mus_data *out, unsigned char *data, size_t size) {
    if (size < 16) return -1;
    if (memcmp(data, "MUS\x1a", 4) != 0) return -2;

    unsigned score_start = getword16(data + 6);
    unsigned num_channels = getword16(data + 8);
    unsigned num_instruments = getword16(data + 12);

    if (score_start >= size) return -3;

    int *instruments = mus_get_instruments(num_instruments, data + 16, size - 16);
    if (instruments == NULL) return -4;

    int e = mus_process_score(data + score_start, size - score_start);
    if (e < 0) return -1;

    out->num_instruments = num_instruments;
    out->num_channels = num_channels;
    out->instruments = instruments;
    out->score = first_chunk;

    return 0;
}


void dump_event(FILE *file, long delay, int type, int channel, int data1, int data2) {
    char col5[24];
    char note[64];
    char *ctrl = "?";

    bool play_at_volume = type==1 && data2 >= 0;
    bool controller = type == 4;
    bool unknown = type == 7;
    bool playdrum = type == 1 && channel == 15;
    bool setinstr = type==4 && channel!=15 && data1==0;

    if (play_at_volume || controller || unknown) sprintf(col5, "%d", data2);
    else strcpy(col5, "_");

    int inscode = -1;
    if (playdrum) inscode = data1 + 100;
    if (setinstr) inscode = data2;

    if (inscode < 0)
        strcpy(note, "");
    else
        sprintf(note, "   # %s", GetInstrumentName(inscode));

    if (controller) ctrl = encode_controller(data1);

    if (delay < 0)
        fprintf(file, "            ");
    else
        fprintf(file, "delay %-5ld ", delay);

    switch(type) {
        case 0: fprintf(file, "STOP CH%X %4d %3s\n", channel, data1, col5); break;
        case 1: fprintf(file, "PLAY CH%X %4d %3s%s\n",   channel, data1, col5, note); break;
        case 2: fprintf(file, "BEND CH%X %4d %3s\n",     channel, data1, col5); break;
        case 3: fprintf(file, "SYS  CH%X %4d %3s\n",     channel, data1, col5); break;
        case 4: fprintf(file, "CTRL CH%X %4s %3s%s\n",   channel, ctrl, col5, note); break;
        case 5: fprintf(file, "MEAS CH%X %4s %3s\n",     channel, "_", "_"); break;
        case 6: fprintf(file, "end of score\n"); break;
        case 7: fprintf(file, "UNKN CH%X %4d %3s\n",     channel, data1, col5); break;
    }
}

void DumpMUS(FILE *file, struct mus_data md) {
    fprintf(file, "num_channels %d\n", md.num_channels);
    fprintf(file, "num_instruments %d\n", md.num_instruments);
    fprintf(file, "\n");

    for (int i = 0; i < md.num_instruments; i++) {
        int ins = md.instruments[i];
        fprintf(file, "instrument %3d %3d %s\n", i, ins, GetInstrumentName(ins));
    }

    fprintf(file, "\n");

    struct mus_chunk *chunk = md.score;
    while (chunk) {
        bool first = true;
        for (int i = 0; i < chunk->num_events; i++) {
            struct mus_event *ev = &chunk->events[i];
            dump_event(file, first ? chunk->delay : -1, ev->type, ev->channel, ev->data1, ev->data2);
            first = false;
        }
        chunk = chunk->next;
    }
}



static const char *gm_instruments[128] = {
    "Acoustic Grand Piano",
    "Bright Acoustic Piano",
    "Electric Grand Piano",
    "Honky-tonk Piano",
    "Electric Piano 1",
    "Electric Piano 2",
    "Harpsichord",
    "Clavinet",

    "Celesta",
    "Glockenspiel",
    "Music Box",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "Tubular Bells",
    "Dulcimer",

    "Drawbar Organ",
    "Percussive Organ",
    "Rock Organ",
    "Church Organ",
    "Reed Organ",
    "Accordion",
    "Harmonica",
    "Tango Accordion",

    "Acoustic Guitar (nylon)",
    "Acoustic Guitar (steel)",
    "Electric Guitar (jazz)",
    "Electric Guitar (clean)",
    "Electric Guitar (muted)",
    "Overdriven Guitar",
    "Distortion Guitar",
    "Guitar Harmonics",

    "Acoustic Bass",
    "Electric Bass (finger)",
    "Electric Bass (pick)",
    "Fretless Bass",
    "Slap Bass 1",
    "Slap Bass 2",
    "Synth Bass 1",
    "Synth Bass 2",

    "Violin",
    "Viola",
    "Cello",
    "Contrabass",
    "Tremolo Strings",
    "Pizzicato Strings",
    "Orchestral Harp",
    "Timpani",

    "String Ensemble 1",
    "String Ensemble 2",
    "Synth Strings 1",
    "Synth Strings 2",
    "Choir Aahs",
    "Voice Oohs",
    "Synth Voice",
    "Orchestra Hit",

    "Trumpet",
    "Trombone",
    "Tuba",
    "Muted Trumpet",
    "French Horn",
    "Brass Section",
    "Synth Brass 1",
    "Synth Brass 2",

    "Soprano Sax",
    "Alto Sax",
    "Tenor Sax",
    "Baritone Sax",
    "Oboe",
    "English Horn",
    "Bassoon",
    "Clarinet",

    "Piccolo",
    "Flute",
    "Recorder",
    "Pan Flute",
    "Blown Bottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",

    "Lead 1 (square)",
    "Lead 2 (sawtooth)",
    "Lead 3 (calliope)",
    "Lead 4 (chiff)",
    "Lead 5 (charang)",
    "Lead 6 (voice)",
    "Lead 7 (fifths)",
    "Lead 8 (bass + lead)",

    "Pad 1 (new age)",
    "Pad 2 (warm)",
    "Pad 3 (polysynth)",
    "Pad 4 (choir)",
    "Pad 5 (bowed)",
    "Pad 6 (metallic)",
    "Pad 7 (halo)",
    "Pad 8 (sweep)",

    "FX 1 (rain)",
    "FX 2 (soundtrack)",
    "FX 3 (crystal)",
    "FX 4 (atmosphere)",
    "FX 5 (brightness)",
    "FX 6 (goblins)",
    "FX 7 (echoes)",
    "FX 8 (sci-fi)",

    "Sitar",
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "Bag pipe",
    "Fiddle",
    "Shanai",

    "Tinkle Bell",
    "Agogo",
    "Steel Drums",
    "Woodblock",
    "Taiko Drum",
    "Melodic Tom",
    "Synth Drum",
    "Reverse Cymbal",

    "Guitar Fret Noise",
    "Breath Noise",
    "Seashore",
    "Bird Tweet",
    "Telephone Ring",
    "Helicopter",
    "Applause",
    "Gunshot"
};

static const char *gm_percussion[] = {
    "Bass Drum 2",      // 35
    "Bass Drum 1",      // 36
    "Side Stick",       // 37
    "Snare Drum 1",     // 38
    "Hand Clap",        // 39
    "Snare Drum 2",     // 40
    "Low Tom 2",        // 41
    "Closed Hi-hat",    // 42
    "Low Tom 1",        // 43
    "Pedal Hi-hat",     // 44
    "Mid Tom 2",        // 45
    "Open Hi-hat",      // 46
    "Mid Tom 1",        // 47
    "High Tom 2",       // 48
    "Crash Cymbal 1",   // 49
    "High Tom 1",       // 50
    "Ride Cymbal 1",    // 51
    "Chinese Cymbal",   // 52
    "Ride Bell",        // 53
    "Tambourine",       // 54
    "Splash Cymbal",    // 55
    "Cowbell",          // 56
    "Crash Cymbal 2",   // 57
    "Vibra Slap",       // 58
    "Ride Cymbal 2",    // 59
    "High Bongo",       // 60
    "Low Bongo",        // 61
    "Mute High Conga",  // 62
    "Open High Conga",  // 63
    "Low Conga",        // 64
    "High Timbale",     // 65
    "Low Timbale",      // 66
    "High Agogo",       // 67
    "Low Agogo",        // 68
    "Cabasa",           // 69
    "Maracas",          // 70
    "Short Whistle",    // 71
    "Long Whistle",     // 72
    "Short Guiro",      // 73
    "Long Guiro",       // 74
    "Claves",           // 75
    "High Wood Block",  // 76
    "Low Wood Block",   // 77
    "Mute Cuica",       // 78
    "Open Cuica",       // 79
    "Mute Triangle",    // 80
    "Open Triangle"     // 81
};


static const char *gm2_percussion[] = {
    "Shaker",      // 82
    "Jingle Bell", // 83
    "Belltree",    // 84
    "Castanets",   // 85
    "Mute Surdo",  // 86
    "Open Surdo"   // 87
};
