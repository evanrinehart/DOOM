struct viewpoint {
    int x;
    int y;
    int z;
    float bearing;
    float inclination;
    float roll;
    float half_fovx;
    int subsectornum;
    int extralight;
    int fixedcolormap;
};

struct canvas {
    byte *screen;
    int left;
    int width;
    int top;
    int height;
};

extern void (*X_AlternateRender)(struct canvas, struct viewpoint);
