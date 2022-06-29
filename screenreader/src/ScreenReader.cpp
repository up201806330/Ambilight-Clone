#include "ScreenReader.h"

#include <sstream>

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

void ScreenReader::initShm(){
    int width  = getScreenWidth ();
    int height = getScreenHeight();

    // Create a shared memory area
    shminfo.shmid = shmget(IPC_PRIVATE, width * height * BITS_PER_PIXEL, IPC_CREAT | 0600);
    if (shminfo.shmid == -1){
        throw std::system_error(
            std::error_code(errno, std::system_category()),
            "Reading screen"
        );
    }

    // Map the shared memory segment into the address space of this process
    shminfo.shmaddr = (char *)shmat(shminfo.shmid, 0, 0);
    if (shminfo.shmaddr == (char *)-1){
        throw std::system_error(
            std::error_code(errno, std::system_category()),
            "Reading screen"
        );
    }

    data = (unsigned int *)shminfo.shmaddr;
    shminfo.readOnly = false;

    // Mark the shared memory segment for removal
    // It will be removed even if this program crashes
    shmctl(shminfo.shmid, IPC_RMID, 0);

    // Ask the X server to attach the shared memory segment and sync
    if (XShmAttach(dsp, &shminfo) == 0){
            throw std::system_error(
            std::error_code(errno, std::system_category()),
            "Could not attach XImage structure"
        );
    }
    XSync(dsp, false);
}

void ScreenReader::initXImage(){
    // Allocate the memory needed for the XImage structure
    ximage = XShmCreateImage(
        dsp,
        XDefaultVisual(dsp, XDefaultScreen(dsp)),
        DefaultDepth(dsp, XDefaultScreen(dsp)),
        ZPixmap,
        NULL,
        &shminfo,
        0,
        0
    );
    if (!ximage){
        throw std::system_error(
            std::error_code(errno, std::system_category()),
            "Could not allocate the XImage structure"
        );
    }

    ximage->data = (char *)data;
    ximage->width  = getScreenWidth ();
    ximage->height = getScreenHeight();
}

ScreenReader::ScreenReader(int NUM_LEDS_X, int NUM_LEDS_Y){
    shminfo.shmaddr = (char *)-1;
    initDisplay();
    initShm();
    initXImage();

    MARGIN_X = getScreenWidth () / NUM_LEDS_X;
    MARGIN_Y = getScreenHeight() / NUM_LEDS_Y;
}

int ScreenReader::getScreenWidth (){ return screenWidth  = XDisplayWidth (dsp, XDefaultScreen(dsp)); }
int ScreenReader::getScreenHeight(){ return screenHeight = XDisplayHeight(dsp, XDefaultScreen(dsp)); }

void ScreenReader::update(){
    XShmGetImage(dsp, XDefaultRootWindow(dsp), ximage, 0, 0, AllPlanes);
    
    // Set alpha channel to 0xff
    uint32_t *p = data;
    for (int i = 0; i < ximage->height * ximage->width; ++i){
        *p++ |= 0xff000000;
    }
}

uint32_t ScreenReader::getPixel(int x, int y){
    if(!(
        0 <= x && x < screenWidth &&
        0 <= y && y < screenHeight
    )) throw std::invalid_argument("x and y must be within bounds");
    if(
        MARGIN_X <= x && x < screenWidth-MARGIN_X &&
        MARGIN_Y <= y && y < screenWidth-MARGIN_Y
    ){
        std::stringstream ss;
        ss  << "(" << x << ", " << y << ") is outside margins; "
            << "margins are (" << MARGIN_X << ", " << MARGIN_Y << "); "
            << "size is (" << screenWidth << ", " << screenHeight << ")";
        throw std::invalid_argument(ss.str());
    }
    return data[y * screenWidth + x];
}

ScreenReader::~ScreenReader(){
    if (ximage){
        XShmDetach(dsp, &shminfo);
        XDestroyImage(ximage);
        ximage = NULL;
    }

    if (shminfo.shmaddr != (char *)-1){
        shmdt(shminfo.shmaddr);
        shminfo.shmaddr = (char *)-1;
    }

    if(dsp){
        XCloseDisplay(dsp);
    }
}

