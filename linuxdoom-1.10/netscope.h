struct netstatus {
    int num_nodes;

    int gametic;
    int maketic;
    int nettics[8];
    int lowtic; // minimum of nettics

    int players[8];

    double current_time;
    double contact_time[8];

    int recent_nitro;
    int recent_brakes;
    int recent_botch;
    bool wayahead;
    
    // botchstart not zero means prior missing packet
    int botch_start[8];
    int botch_num[8];
};


void show_netstatus(struct netstatus *s);
