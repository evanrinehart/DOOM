struct mus_event {
    int type;
    int channel;
    int data1;
    int data2;
};

struct mus_chunk {
    long delay;
    int num_events;
    struct mus_event *events;
    struct mus_chunk *next;
};

struct mus_data {
    int num_channels;
    int num_instruments;
    int *instruments;
    struct mus_chunk *score;
};

int ReadMUS(struct mus_data *out, unsigned char *data, size_t size);
void DumpMUS(FILE *file, struct mus_data md);
const char *GetInstrumentName(int ins);
