#pragma once

#include <system_error>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

class ScreenReader {
private:
    static const int BITS_PER_PIXEL = 4;

private:
    int MARGIN_X, MARGIN_Y;

    XShmSegmentInfo shminfo;
    XImage *ximage = nullptr;
    unsigned int *data; // will point to the image's BGRA packed pixels
    XShmSegmentInfo shminfo_bot;
    XShmSegmentInfo shminfo_lef;
    XShmSegmentInfo shminfo_top;
    XShmSegmentInfo shminfo_rig;
    XImage *ximage_bot;
    XImage *ximage_lef;
    XImage *ximage_top;
    XImage *ximage_rig;
    unsigned int *data_bot;
    unsigned int *data_lef;
    unsigned int *data_top;
    unsigned int *data_rig;
    Display *dsp;
    int screenWidth, screenHeight;

private:
    void initDisplay();

    unsigned int *createShm(int width, int height, XShmSegmentInfo &shminfo);
    void initShm();

    void initXImage();

public:
    ScreenReader(int NUM_LEDS_X, int NUM_LEDS_Y);

    int getScreenWidth ();
    int getScreenHeight();

    void update();

    uint32_t getPixel(int x, int y);

    ~ScreenReader();
};
