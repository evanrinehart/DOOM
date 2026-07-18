struct framebuffer {
    byte *color;
    byte *mask;
    int width;
    int height;
    int count; // width x height
    int dirty; // contains new stuff

    // optional cropping
    int left;
    int top;
    int right;
    int bottom;
};

extern struct framebuffer fb_hud;
extern struct framebuffer fb_status;
extern struct framebuffer fb_screen;
extern struct framebuffer fb_aux;
extern struct framebuffer fb_backwall;
extern struct framebuffer fb_wipe;
extern struct framebuffer fb_wipesrc;
extern struct framebuffer fb_wipeaux;
extern struct framebuffer fb_menu;
