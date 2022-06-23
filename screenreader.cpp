#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <stdbool.h>

#include <vector>

#include <exception>
#include <system_error>

#define NUM_LEDS_WIDTH 32
#define NUM_LEDS_HEIGHT 20
#define BPP 4
#define LED_PIXELS_COMPENSATION 15

const int NUM_LEDS_TOTAL = 2 * (NUM_LEDS_WIDTH + NUM_LEDS_HEIGHT);

class ScreenReader {
private:
    XShmSegmentInfo shminfo;
    XImage *ximage = nullptr;
    unsigned int *data; // will point to the image's BGRA packed pixels
    Display *dsp;
    int screenWidth, screenHeight;

private:
    void initDisplay(){
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

    void initShm(){
        int width  = getScreenWidth ();
        int height = getScreenHeight();

        // Create a shared memory area
        shminfo.shmid = shmget(IPC_PRIVATE, width * height * BPP, IPC_CREAT | 0600);
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

    void initXImage(){
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

public:
    ScreenReader(){
        shminfo.shmaddr = (char *)-1;
        initDisplay();
        initShm();
        initXImage();
    }

    int getScreenWidth (){ return screenWidth  = XDisplayWidth (dsp, XDefaultScreen(dsp)); }
    int getScreenHeight(){ return screenHeight = XDisplayHeight(dsp, XDefaultScreen(dsp)); }

    void update(){
        XShmGetImage(dsp, XDefaultRootWindow(dsp), ximage, 0, 0, AllPlanes);
        
        // Set alpha channel to 0xff
        uint32_t *p = data;
        for (int i = 0; i < ximage->height * ximage->width; ++i){
            *p++ |= 0xff000000;
        }
    }

    uint32_t getPixel(int x, int y){
        return data[y * screenWidth + x];
    }

    ~ScreenReader(){
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
};

struct Color {
    uint8_t r, g, b, a;
    Color(uint32_t color){
        a = (color & 0xFF000000) >> 24;
        r = (color & 0x00FF0000) >> 16;
        g = (color & 0x0000FF00) >> 8;
        b = (color & 0x000000FF);
    }
    Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_)
        :r(r_),g(g_),b(b_),a(a_){}
    
    uint8_t *toArray() const {
        uint8_t *ret = new uint8_t[3];
        ret[0] = r;
        ret[1] = g;
        ret[2] = b;
        return ret;
    }
};

class ScreenProcessor {
    ScreenReader &reader;
    int colorWidth;
    int colorHeight;

    int screenWidth;
    int screenHeight;
public:
    ScreenProcessor(
        ScreenReader &reader_,
        int colorWidth_,
        int colorHeight_
    ):
        reader(reader_),
        colorWidth (colorWidth_ ),
        colorHeight(colorHeight_),
        screenWidth (reader.getScreenWidth ()),
        screenHeight(reader.getScreenHeight())
    {}

    int getWidth (){ return reader.getScreenWidth (); }
    int getHeight(){ return reader.getScreenHeight(); }

    Color getColor(const int x, const int y){
        int r = 0, g = 0, b = 0, a = 0;
        size_t n = 0;
        for(int deltaX = -colorWidth/2; deltaX < colorWidth/2; ++deltaX){
            for(int deltaY = -colorHeight/2; deltaY < colorHeight/2; ++deltaY){
                const int xPixel = x + deltaX;
                const int yPixel = y + deltaY;
                if(
                    0 <= xPixel && xPixel < screenWidth  &&
                    0 <= yPixel && yPixel < screenHeight
                ) {
                    Color c = reader.getPixel(xPixel, yPixel);
                    r += c.r;
                    g += c.g;
                    b += c.b;
                    a += c.a;
                    n++;
                }
            }
        }
        r /= n;
        g /= n;
        b /= n;
        a /= n;
        return Color(r, g, b, a);
    }

    void update(){
        reader.update();
    }
};

class LedProcessor {
    ScreenProcessor &processor;
    const int NUM_LEDS_X;
    const int NUM_LEDS_Y;
    const int SIZE_X;
    const int SIZE_Y;
    const int PIXELS_PER_LED_X;
    const int PIXELS_PER_LED_Y;

    std::vector<Color> leds;

    std::pair<int, int> indexToPixel(int i){
        if(i < 0){
            throw std::invalid_argument("i must be non-negative");
        } else if(i < NUM_LEDS_WIDTH){
            int led_x = NUM_LEDS_WIDTH - 1 - i;

            int x = PIXELS_PER_LED_X * led_x + PIXELS_PER_LED_X/2;
            int y = SIZE_Y - PIXELS_PER_LED_Y/2;

            return std::make_pair(x, y);
        } else if(i < NUM_LEDS_WIDTH+NUM_LEDS_HEIGHT){
            i -= NUM_LEDS_WIDTH;

            int led_y = NUM_LEDS_HEIGHT - 1 - i;

            int x = PIXELS_PER_LED_X/2;
            int y = led_y * PIXELS_PER_LED_Y + PIXELS_PER_LED_Y/2;

            return std::make_pair(x, y);
        } else if(i < 2*NUM_LEDS_WIDTH+NUM_LEDS_HEIGHT){
            i -= NUM_LEDS_WIDTH+NUM_LEDS_HEIGHT;

            int led_x = i;

            int x = PIXELS_PER_LED_X * led_x + PIXELS_PER_LED_X/2;
            int y = PIXELS_PER_LED_Y/2;

            return std::make_pair(x, y);
        } else if(i < 2*NUM_LEDS_WIDTH+2*NUM_LEDS_HEIGHT){
            i -= 2*NUM_LEDS_WIDTH+NUM_LEDS_HEIGHT;

            int led_y = i;

            int x = SIZE_X - PIXELS_PER_LED_X/2;
            int y = led_y * PIXELS_PER_LED_Y + PIXELS_PER_LED_Y/2;

            return std::make_pair(x, y);
        } else {
            throw std::invalid_argument("i must be smaller than 2*NUM_LEDS_WIDTH+2*NUM_LEDS_HEIGHT");
        }
    }
public:
    LedProcessor(ScreenProcessor &processor_, const int NUM_LEDS_X_, const int NUM_LEDS_Y_):
        processor(processor_),
        NUM_LEDS_X(NUM_LEDS_X_), NUM_LEDS_Y(NUM_LEDS_Y_),
        SIZE_X(processor.getWidth ()),
        SIZE_Y(processor.getHeight()),
        PIXELS_PER_LED_X(processor.getWidth () / NUM_LEDS_X),
        PIXELS_PER_LED_Y(processor.getHeight() / NUM_LEDS_Y),
        leds(2*NUM_LEDS_X_+2*NUM_LEDS_Y_, Color(0))
    {
    }

    void update(){
        processor.update();
        for(size_t i = 0; i < leds.size(); ++i){
            const std::pair<int,int> pos = indexToPixel(i);
            const int &x = pos.first;
            const int &y = pos.second;
            leds[i] = processor.getColor(x, y);
        }
    }

    size_t copy(uint8_t *dest){
        for(size_t i = 0; i < leds.size(); ++i){
            const Color &c = leds[i];
            *(dest++) = c.r;
            *(dest++) = c.g;
            *(dest++) = c.b;
        }
        return 3*leds.size();
    }
};

const char SHM_NAME[] = "/shm_leds";
const char SEM_NAME[] = "/sem_leds";
const mode_t SHM_MODE = 0777;
const off_t SHM_SIZE = NUM_LEDS_TOTAL*3;
int shm_fd;
void *shm = NULL;
sem_t *sem = NULL;

int createAndOpenShm(){
    // Shared memory
    shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, SHM_MODE);
    if(shm_fd == -1) return 1;

    if(ftruncate(shm_fd, SHM_SIZE) < 0) return 1;

    shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm == MAP_FAILED) return 1;

    // Semaphore
    if (sem_unlink(SEM_NAME) != 0){
        perror("createAndOpenShm");
    }

    sem = sem_open(SEM_NAME, O_CREAT, 0777, 1);

    if (sem == SEM_FAILED){
        return 1;
    }

    return 0;
}

int closeAndDeleteShm(){
    if(munmap((void*)shm, SHM_SIZE) != 0) return 1;

    if(close(shm_fd) != 0) return 1;

    if(shm_unlink(SHM_NAME) != 0) return 1;

    if (sem_destroy(sem) != 0) return 1;

    if (sem_unlink(SEM_NAME) != 0) return 1;

    return 0;
}

// Flatten array of LED color components
// and write to shared memory
int writeToShm(LedProcessor &ledProcessor){
    sem_wait(sem);

    uint8_t *shm_leds = (uint8_t*)shm;
    ledProcessor.copy(shm_leds);

    sem_post(sem);
    return 0;
}

void ledPrint(){
    uint8_t *leds = (uint8_t*)shm;

    int i = 0;
    int range = NUM_LEDS_WIDTH;
    printf("Bottom: ");
    for (; i < range; i++) printf("%02x%02x%02x ", leds[3*i], leds[3*i+1], leds[3*i+2]);
    printf("\n");

    printf("Left   :");
    range += NUM_LEDS_HEIGHT;
    for (; i < range; i++) printf("%02x%02x%02x ", leds[3*i], leds[3*i+1], leds[3*i+2]);
    printf("\n");

    printf("Top    :");
    range += NUM_LEDS_WIDTH;
    for (; i < range; i++) printf("%02x%02x%02x ", leds[3*i], leds[3*i+1], leds[3*i+2]);
    printf("\n");

    printf("Right  :");
    range += NUM_LEDS_HEIGHT;
    for (; i < range; i++) printf("%02x%02x%02x ", leds[3*i], leds[3*i+1], leds[3*i+2]);
    printf("\n\n");
}

pid_t pid;

void signal_callback_handler(int signum) {
    fprintf(stderr, "Signal callback\n");

    kill(pid, SIGINT);

    if(closeAndDeleteShm()) exit(1);

    exit(0);
}

int main()
{
    if(createAndOpenShm()) {
        perror("[SCREENREADER] Could not open shared memory");
        return 1;
    }

    signal(SIGINT, signal_callback_handler);
    
    // pid = fork();
    // if(pid == 0){ // Child
    //     int ret = execlp("python", "python", "leds.py", NULL);
    //     perror("[screenreader child] Could not exec python");
    //     return ret;
    // }

    ScreenReader screen;

    const int PIXELS_PER_LED_AVG_X = screen.getScreenWidth () / NUM_LEDS_WIDTH;
    const int PIXELS_PER_LED_AVG_Y = screen.getScreenHeight() / NUM_LEDS_HEIGHT;
    ScreenProcessor screenProcessor(
        screen,
        PIXELS_PER_LED_AVG_X,
        PIXELS_PER_LED_AVG_Y
    );

    LedProcessor ledProcessor(screenProcessor, NUM_LEDS_WIDTH, NUM_LEDS_HEIGHT);

    while (true)
    {
        ledProcessor.update();

        if (writeToShm(ledProcessor) == 0){
            ledPrint();
        }
        else {
            printf("Error writting to shared memory");
        }

        sleep(0.05);
    }

    if(closeAndDeleteShm()) return 1;

    return 0;
}
