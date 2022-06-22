#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <stdbool.h>

#define NUM_LEDS_WIDTH 30
#define NUM_LEDS_HEIGHT 20
#define BPP 4

struct shmimage
{
    XShmSegmentInfo shminfo;
    XImage *ximage;
    unsigned int *data; // will point to the image's BGRA packed pixels
};

void initimage(struct shmimage *image)
{
    image->ximage = NULL;
    image->shminfo.shmaddr = (char *)-1;
}

void destroyimage(Display *dsp, struct shmimage *image)
{
    if (image->ximage)
    {
        XShmDetach(dsp, &image->shminfo);
        XDestroyImage(image->ximage);
        image->ximage = NULL;
    }

    if (image->shminfo.shmaddr != (char *)-1)
    {
        shmdt(image->shminfo.shmaddr);
        image->shminfo.shmaddr = (char *)-1;
    }
}

int createimage(Display *dsp, struct shmimage *image, int width, int height)
{
    // Create a shared memory area
    image->shminfo.shmid = shmget(IPC_PRIVATE, width * height * BPP, IPC_CREAT | 0600);
    if (image->shminfo.shmid == -1)
    {
        perror("Reading screen");
        return false;
    }

    // Map the shared memory segment into the address space of this process
    image->shminfo.shmaddr = (char *)shmat(image->shminfo.shmid, 0, 0);
    if (image->shminfo.shmaddr == (char *)-1)
    {
        perror("Reading screen");
        return false;
    }

    image->data = (unsigned int *)image->shminfo.shmaddr;
    image->shminfo.readOnly = false;

    // Mark the shared memory segment for removal
    // It will be removed even if this program crashes
    shmctl(image->shminfo.shmid, IPC_RMID, 0);

    // Allocate the memory needed for the XImage structure
    image->ximage = XShmCreateImage(dsp, XDefaultVisual(dsp, XDefaultScreen(dsp)),
                                    DefaultDepth(dsp, XDefaultScreen(dsp)), ZPixmap, NULL,
                                    &image->shminfo, 0, 0);
    if (!image->ximage)
    {
        destroyimage(dsp, image);
        perror("[SCREENREADER] Could not allocate the XImage structure\n");
        return false;
    }

    image->ximage->data = (char *)image->data;
    image->ximage->width = width;
    image->ximage->height = height;

    // Ask the X server to attach the shared memory segment and sync
    if (XShmAttach(dsp, &image->shminfo) == 0){
        perror("[SCREENREADER] Could not attach XImage structure");
        return false;
    }
    XSync(dsp, false);
    return true;
}

void getrootwindow(Display *dsp, struct shmimage *image)
{
    XShmGetImage(dsp, XDefaultRootWindow(dsp), image->ximage, 0, 0, AllPlanes);
    // This is how you access the image's BGRA packed pixels
    // Lets set the alpha channel of each pixel to 0xff
    int x, y;
    unsigned int *p = image->data;
    for (y = 0; y < image->ximage->height; ++y)
    {
        for (x = 0; x < image->ximage->width; ++x)
        {
            *p++ |= 0xff000000;
        }
    }
}

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
    return led;
}

u_int8_t **pixelsToLeds(unsigned int *pixels, int pixels_per_led_height, int pixels_per_led_width, int screen_height, int screen_width)
{
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

int main()
{
    
    Display *dsp = XOpenDisplay(NULL);
    if (!dsp)
    {
        perror("[SCREENREADER] Could not open a connection to the X server\n");
        return 1;
    }

    if (!XShmQueryExtension(dsp))
    {
        XCloseDisplay(dsp);
        perror("[SCREENREADER] The X server does not support the XSHM extension\n");
        return 1;
    }

    int screen = XDefaultScreen(dsp);
    struct shmimage image;
    initimage(&image);
    if (!createimage(dsp, &image, XDisplayWidth(dsp, screen), XDisplayHeight(dsp, screen)))
    {
        XCloseDisplay(dsp);
        return 1;
    }

    printf("SHM_KEY %d\n", IPC_PRIVATE);
    fflush(stdout);

    int pixels_per_led_height = image.ximage->height / NUM_LEDS_HEIGHT;
    int pixels_per_led_width = image.ximage->width / NUM_LEDS_WIDTH;
    while (true)
    {
        getrootwindow(dsp, &image);

        u_int8_t **leds = pixelsToLeds(image.data, pixels_per_led_height, pixels_per_led_width, image.ximage->height, image.ximage->width);
        int range = NUM_LEDS_WIDTH;
        for (int i = 0; i < range; i++)
        {
            // printf("%02x%02x%02x ", leds[i][0], leds[i][1], leds[i][2]);
        }
        // printf("\n");

        for (int i = range; i < range + NUM_LEDS_HEIGHT; i++)
        {
            // printf("%02x%02x%02x ", leds[i][0], leds[i][1], leds[i][2]);
        }
        // printf("\n");
        range = range + NUM_LEDS_HEIGHT;

        for (int i = range; i < range + NUM_LEDS_WIDTH; i++)
        {
            // printf("%02x%02x%02x ", leds[i][0], leds[i][1], leds[i][2]);
        }
        // printf("\n");
        range = range + NUM_LEDS_WIDTH;

        for (int i = range; i < range + NUM_LEDS_HEIGHT; i++)
        {
            // printf("%02x%02x%02x ", leds[i][0], leds[i][1], leds[i][2]);
        }
        // printf("\n\n");
        free(leds);
        sleep(1);
    }
    destroyimage(dsp, &image);
    XCloseDisplay(dsp);
}