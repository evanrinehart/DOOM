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


void X_AfterSummonMenu(void);
void X_AfterUnsummonMenu(void);
void X_OnEvent(int type, int data1, int data2, int data3);
