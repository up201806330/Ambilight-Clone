#include "ScreenReader.h"

#include <sstream>

#include <iostream>

void ScreenReader::initDisplay(){
    dsp = XOpenDisplay(NULL);
    if (!dsp){
        throw std::system_error(
            std::error_code(errno, std::system_category()),
            "Could not open a connection to the X server"
        );
    }

    if (!XShmQueryExtension(dsp)){
        std::error_code ec(errno, std::system_category());
        XCloseDisplay(dsp);
        throw std::system_error(
            ec,
            "The X server does not support the XSHM extension\n"
        );
    }
}

unsigned int *ScreenReader::createShm(int width, int height, XShmSegmentInfo &info){
    // Create a shared memory area
    info.shmid = shmget(IPC_PRIVATE, width * height * BITS_PER_PIXEL, IPC_CREAT | 0600);
    if (info.shmid == -1){
        throw std::system_error(
            std::error_code(errno, std::system_category()),
            "Reading screen"
        );
    }

    // Map the shared memory segment into the address space of this process
    info.shmaddr = (char *)shmat(info.shmid, 0, 0);
    if (info.shmaddr == (char *)-1){
        throw std::system_error(
            std::error_code(errno, std::system_category()),
            "Reading screen"
        );
    }

    unsigned int *ret = (unsigned int *)info.shmaddr;
    info.readOnly = false;

    // Mark the shared memory segment for removal
    // It will be removed even if this program crashes
    shmctl(info.shmid, IPC_RMID, 0);

    // Ask the X server to attach the shared memory segment and sync
    if (XShmAttach(dsp, &info) == 0){
            throw std::system_error(
            std::error_code(errno, std::system_category()),
            "Could not attach XImage structure"
        );
    }
    XSync(dsp, false);

    return ret;
}

void ScreenReader::initShm(){
    data_bot = createShm(getScreenWidth(), MARGIN_Y, shminfo_bot);
    data_top = createShm(getScreenWidth(), MARGIN_Y, shminfo_top);
    data_lef = createShm(MARGIN_X, getScreenHeight() - 2*MARGIN_Y, shminfo_lef);
    data_rig = createShm(MARGIN_X, getScreenHeight() - 2*MARGIN_Y, shminfo_rig);
}

void ScreenReader::initXImage(){
    // Allocate the memory needed for the XImage structure
    ximage_bot = XShmCreateImage(dsp, XDefaultVisual(dsp, XDefaultScreen(dsp)), DefaultDepth(dsp, XDefaultScreen(dsp)), ZPixmap, NULL, &shminfo_bot, getScreenWidth(), MARGIN_Y);
    ximage_lef = XShmCreateImage(dsp, XDefaultVisual(dsp, XDefaultScreen(dsp)), DefaultDepth(dsp, XDefaultScreen(dsp)), ZPixmap, NULL, &shminfo_lef, MARGIN_X, getScreenHeight() - 2*MARGIN_Y);
    ximage_top = XShmCreateImage(dsp, XDefaultVisual(dsp, XDefaultScreen(dsp)), DefaultDepth(dsp, XDefaultScreen(dsp)), ZPixmap, NULL, &shminfo_top, getScreenWidth(), MARGIN_Y);
    ximage_rig = XShmCreateImage(dsp, XDefaultVisual(dsp, XDefaultScreen(dsp)), DefaultDepth(dsp, XDefaultScreen(dsp)), ZPixmap, NULL, &shminfo_rig, MARGIN_X, getScreenHeight() - 2*MARGIN_Y);

    if (!ximage_bot || !ximage_lef || !ximage_top || !ximage_rig){
        throw std::system_error(
            std::error_code(errno, std::system_category()),
            "Could not allocate the XImage structures"
        );
    }

    ximage_bot->data = (char *)data_bot;
    ximage_lef->data = (char *)data_lef;
    ximage_top->data = (char *)data_top;
    ximage_rig->data = (char *)data_rig;
}

ScreenReader::ScreenReader(int NUM_LEDS_X, int NUM_LEDS_Y){
    shminfo_bot.shmaddr = (char *)-1;
    shminfo_lef.shmaddr = (char *)-1;
    shminfo_top.shmaddr = (char *)-1;
    shminfo_rig.shmaddr = (char *)-1;
    initDisplay();

    MARGIN_X = getScreenWidth () / NUM_LEDS_X;
    MARGIN_Y = getScreenHeight() / NUM_LEDS_Y;

    initShm();
    initXImage();
}

int ScreenReader::getScreenWidth (){ return screenWidth  = XDisplayWidth (dsp, XDefaultScreen(dsp)); }
int ScreenReader::getScreenHeight(){ return screenHeight = XDisplayHeight(dsp, XDefaultScreen(dsp)); }

void ScreenReader::update(){
    XShmGetImage(dsp, XDefaultRootWindow(dsp), ximage_bot, 0, screenHeight-MARGIN_Y, AllPlanes);
    XShmGetImage(dsp, XDefaultRootWindow(dsp), ximage_lef, 0, MARGIN_Y, AllPlanes);
    XShmGetImage(dsp, XDefaultRootWindow(dsp), ximage_top, 0, 0, AllPlanes);
    XShmGetImage(dsp, XDefaultRootWindow(dsp), ximage_rig, screenWidth-MARGIN_X, MARGIN_Y, AllPlanes);

    // Set alpha channel to 0xff
    uint32_t *p;
    p = data_bot; for (int i = 0; i < ximage_bot->height * ximage_bot->width; ++i) *p++ |= 0xff000000;
    p = data_lef; for (int i = 0; i < ximage_lef->height * ximage_lef->width; ++i) *p++ |= 0xff000000;
    p = data_top; for (int i = 0; i < ximage_top->height * ximage_top->width; ++i) *p++ |= 0xff000000;
    p = data_rig; for (int i = 0; i < ximage_rig->height * ximage_rig->width; ++i) *p++ |= 0xff000000;
}

uint32_t ScreenReader::getPixel(int x, int y){
    if(!(
        0 <= x && x < screenWidth &&
        0 <= y && y < screenHeight
    )) throw std::invalid_argument("x and y must be within bounds");
    if(
        MARGIN_X <= x && x < screenWidth -MARGIN_X &&
        MARGIN_Y <= y && y < screenHeight-MARGIN_Y
    ){
        std::stringstream ss;
        ss  << "(" << x << ", " << y << ") is outside margins; "
            << "margins are (" << MARGIN_X << ", " << MARGIN_Y << "); "
            << "size is (" << screenWidth << ", " << screenHeight << ")";
        throw std::invalid_argument(ss.str());
    }

    if(y < MARGIN_Y){ // Top
        const int dx = x;
        const int dy = y;
        return data_top[dy * ximage_top->width + dx];
    } else if(screenHeight-MARGIN_Y <= y){ // Bottom
        const int dx = x;
        const int dy = y - (screenHeight-MARGIN_Y);
        return data_bot[dy * ximage_bot->width + dx];
    } else if(x < MARGIN_X){ // Left
        const int dx = x;
        const int dy = y-MARGIN_Y;
        return data_lef[dy * ximage_lef->width + dx];
    } else if(screenWidth-MARGIN_X <= x){ // Right
        const int dx = x - (screenWidth-MARGIN_X);
        const int dy = y-MARGIN_Y;
        return data_rig[dy * ximage_rig->width + dx];
    } else {
        throw std::invalid_argument("Something went wrong");
    }
}

ScreenReader::~ScreenReader(){
    if (ximage_bot){ XShmDetach(dsp, &shminfo_bot); XDestroyImage(ximage_bot); ximage_bot = nullptr; }
    if (ximage_lef){ XShmDetach(dsp, &shminfo_lef); XDestroyImage(ximage_lef); ximage_lef = nullptr; }
    if (ximage_top){ XShmDetach(dsp, &shminfo_top); XDestroyImage(ximage_top); ximage_top = nullptr; }
    if (ximage_rig){ XShmDetach(dsp, &shminfo_rig); XDestroyImage(ximage_rig); ximage_rig = nullptr; }

    if (shminfo_bot.shmaddr != (char *)-1){ shmdt(shminfo_bot.shmaddr); shminfo_bot.shmaddr = (char *)-1; }
    if (shminfo_lef.shmaddr != (char *)-1){ shmdt(shminfo_lef.shmaddr); shminfo_lef.shmaddr = (char *)-1; }
    if (shminfo_top.shmaddr != (char *)-1){ shmdt(shminfo_top.shmaddr); shminfo_top.shmaddr = (char *)-1; }
    if (shminfo_rig.shmaddr != (char *)-1){ shmdt(shminfo_rig.shmaddr); shminfo_rig.shmaddr = (char *)-1; }

    if(dsp){
        XCloseDisplay(dsp);
    }
}

