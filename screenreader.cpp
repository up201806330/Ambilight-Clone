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

#include <exception>
#include <system_error>

#define NUM_LEDS_WIDTH 32
#define NUM_LEDS_HEIGHT 20
#define BPP 4

const int NUM_LEDS_TOTAL = 2 * (NUM_LEDS_WIDTH + NUM_LEDS_HEIGHT);

struct shmimage {
    XShmSegmentInfo shminfo;
    XImage *ximage = nullptr;
    unsigned int *data; // will point to the image's BGRA packed pixels
    Display *dsp;

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

    shmimage(){
        shminfo.shmaddr = (char *)-1;
        initDisplay();
        initShm();
        initXImage();
    }

    int getScreenWidth (){ return XDisplayWidth (dsp, XDefaultScreen(dsp)); }
    int getScreenHeight(){ return XDisplayHeight(dsp, XDefaultScreen(dsp)); }

    void update(){
        XShmGetImage(dsp, XDefaultRootWindow(dsp), ximage, 0, 0, AllPlanes);
        
        // Set alpha channel to 0xff
        uint32_t *p = data;
        for (int i = 0; i < ximage->height * ximage->width; ++i){
            *p++ |= 0xff000000;
        }
    }

    ~shmimage(){
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

void convertRGB(u_int32_t color, u_int8_t *r, u_int8_t *g, u_int8_t *b)
{
    *r = (color & 0x00FF0000) >> 16;
    *g = (color & 0x0000FF00) >> 8;
    *b = (color & 0x000000FF);
}

u_int8_t *averageRGB(int total_r, int total_g, int total_b, int pixels_per_led_width, int pixels_per_led_height)
{
    u_int8_t *led = (u_int8_t *)malloc(3 * sizeof(u_int8_t));
    led[0] = total_r / (pixels_per_led_width * pixels_per_led_height);
    led[1] = total_g / (pixels_per_led_width * pixels_per_led_height);
    led[2] = total_b / (pixels_per_led_width * pixels_per_led_height);

    led[0] = (led[0] > 10 ? led[0] : 0);
    led[1] = (led[1] > 10 ? led[1] : 0);
    led[2] = (led[2] > 10 ? led[2] : 0);
    return led;
}

u_int8_t **pixelsToLeds(shmimage &image, int pixels_per_led_height, int pixels_per_led_width)
{
    unsigned int *pixels = image.data;
    const int screen_width  = image.getScreenWidth();
    const int screen_height = image.getScreenHeight();

    u_int8_t **leds = (u_int8_t **)malloc((NUM_LEDS_HEIGHT * 2 + NUM_LEDS_WIDTH * 2) * sizeof(u_int8_t *));
    for (int led_y = 0; led_y < NUM_LEDS_HEIGHT; led_y++)
    {
        // Bottom of the led strip
        if (led_y == NUM_LEDS_HEIGHT - 1)
        {
            for (int led_x = 0; led_x < NUM_LEDS_WIDTH; led_x++)
            {
                int total_r = 0, total_g = 0, total_b = 0;
                for (int pixel_y = 0; pixel_y < pixels_per_led_height; pixel_y++)
                {
                    for (int pixel_x = pixels_per_led_width * led_x; pixel_x < pixels_per_led_width * (led_x + 1); pixel_x++)
                    {
                        u_int8_t r, g, b;
                        convertRGB(pixels[pixel_y * screen_width + pixel_x], &r, &g, &b);
                        total_r += r;
                        total_g += g;
                        total_b += b;
                    }
                }
                leds[led_x] = averageRGB(total_r, total_g, total_b, pixels_per_led_width, pixels_per_led_height);
            }
        }
        // Top part of the led strip
        else if (led_y == 0)
        {
            for (int led_x = 0; led_x < NUM_LEDS_WIDTH; led_x++)
            {
                int total_r = 0, total_g = 0, total_b = 0;
                for (int pixel_y = screen_height - pixels_per_led_height; pixel_y < screen_height; pixel_y++)
                {
                    for (int pixel_x = pixels_per_led_width * led_x; pixel_x < pixels_per_led_width * (led_x + 1); pixel_x++)
                    {
                        u_int8_t r, g, b;
                        convertRGB(pixels[pixel_y * screen_width + pixel_x], &r, &g, &b);
                        total_r += r;
                        total_g += g;
                        total_b += b;
                    }
                }
                leds[NUM_LEDS_HEIGHT + NUM_LEDS_WIDTH*2 - 1 - led_x] = averageRGB(total_r, total_g, total_b, pixels_per_led_width, pixels_per_led_height);
            }
        }

        // Left led for y height
        int total_r = 0, total_g = 0, total_b = 0;
        for (int pixel_y = led_y * pixels_per_led_height; pixel_y < (led_y + 1) * pixels_per_led_height; pixel_y++)
        {
            for (int pixel_x = screen_width - pixels_per_led_width; pixel_x < screen_width; pixel_x++)
            {
                u_int8_t r, g, b;
                convertRGB(pixels[pixel_y * screen_width + pixel_x], &r, &g, &b);
                total_r += r;
                total_g += g;
                total_b += b;
            }
        }
        leds[NUM_LEDS_WIDTH + led_y] = averageRGB(total_r, total_g, total_b, pixels_per_led_width, pixels_per_led_height);

        // Right led for y height
        total_r = 0, total_g = 0, total_b = 0;
        for (int pixel_y = led_y * pixels_per_led_height; pixel_y < (led_y + 1) * pixels_per_led_height; pixel_y++)
        {
            for (int pixel_x = 0; pixel_x < pixels_per_led_width; pixel_x++)
            {
                u_int8_t r, g, b;
                convertRGB(pixels[pixel_y * screen_width + pixel_x], &r, &g, &b);
                total_r += r;
                total_g += g;
                total_b += b;
            }
        }
        leds[NUM_LEDS_WIDTH*2 + NUM_LEDS_HEIGHT*2 - 1 - led_y] = averageRGB(total_r, total_g, total_b, pixels_per_led_width, pixels_per_led_height);
    }
    return leds;
}

void ledPrint(u_int8_t **leds){
    int range = NUM_LEDS_WIDTH;
    for (int i = 0; i < range; i++)
    {
        printf("%02x%02x%02x ", leds[i][0], leds[i][1], leds[i][2]);
    }
    printf("\n");

    for (int i = range; i < range + NUM_LEDS_HEIGHT; i++)
    {
        printf("%02x%02x%02x ", leds[i][0], leds[i][1], leds[i][2]);
    }
    printf("\n");
    range = range + NUM_LEDS_HEIGHT;

    for (int i = range; i < range + NUM_LEDS_WIDTH; i++)
    {
        printf("%02x%02x%02x ", leds[i][0], leds[i][1], leds[i][2]);
    }
    printf("\n");
    range = range + NUM_LEDS_WIDTH;

    for (int i = range; i < range + NUM_LEDS_HEIGHT; i++)
    {
        printf("%02x%02x%02x ", leds[i][0], leds[i][1], leds[i][2]);
    }
    printf("\n\n");
}

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
int writeToShm(u_int8_t **leds){
    sem_wait(sem);

    uint8_t *shm_leds = (uint8_t*)shm;
    int idx = 0;
    for(int i = 0; i < NUM_LEDS_TOTAL; ++i){
        for(int j = 0; j < 3; ++j){
            shm_leds[idx++] = leds[i][j];
        }
    }

    sem_post(sem);
    return 0;
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

    shmimage image;

    int pixels_per_led_height = image.getScreenHeight() / NUM_LEDS_HEIGHT;
    int pixels_per_led_width  = image.getScreenWidth () / NUM_LEDS_WIDTH;
    while (true)
    {
        image.update();

        u_int8_t **leds = pixelsToLeds(image, pixels_per_led_height, pixels_per_led_width);
        if (writeToShm(leds) == 0){
            ledPrint(leds);
        }
        else {
            printf("Error writting to shared memory");
        }

        // free(leds);
        sleep(0.05);
    }

    if(closeAndDeleteShm()) return 1;

    return 0;
}
