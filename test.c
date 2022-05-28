#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_LEDS_WIDTH 30
#define NUM_LEDS_HEIGHT 20
#define EVERY_X_PIXELS 2

void convertRGB(u_int32_t color, u_int8_t *r, u_int8_t *g, u_int8_t *b)
{
    *r = (color & 0x00FF0000) >> 16;
    *g = (color & 0x0000FF00) >> 8;
    *b = (color & 0x000000FF);
}

u_int8_t *averageRGB(int total_r, int total_g, int total_b, int pixels_per_led_width, int pixels_per_led_height)
{
    u_int8_t *led = (u_int8_t *)malloc(3 * sizeof(u_int8_t));
    led[0] = total_r / (pixels_per_led_width * pixels_per_led_height / (EVERY_X_PIXELS * 2));
    led[1] = total_g / (pixels_per_led_width * pixels_per_led_height / (EVERY_X_PIXELS * 2));
    led[2] = total_b / (pixels_per_led_width * pixels_per_led_height / (EVERY_X_PIXELS * 2));
    return led;
}

u_int8_t **pixelsToLeds(u_int32_t **pixels, int pixels_per_led_height, int pixels_per_led_width, int screen_height, int screen_width)
{
    u_int8_t **leds = (u_int8_t **)malloc((NUM_LEDS_HEIGHT * 2 + NUM_LEDS_WIDTH * 2) * sizeof(u_int8_t *));
    for (int led_y = 0; led_y < NUM_LEDS_HEIGHT; led_y++)
    {
        // Top part of the led strip
        if (led_y == 0)
        {
            for (int led_x = 0; led_x < NUM_LEDS_WIDTH; led_x++)
            {
                int total_r = 0, total_g = 0, total_b = 0;
                for (int pixel_y = 0; pixel_y < pixels_per_led_height; pixel_y += EVERY_X_PIXELS)
                {
                    for (int pixel_x = pixels_per_led_width * led_x; pixel_x < pixels_per_led_width * (led_x + 1); pixel_x += EVERY_X_PIXELS)
                    {
                        u_int8_t r, g, b;
                        convertRGB(pixels[pixel_y][pixel_x], &r, &g, &b);
                        total_r += r;
                        total_g += g;
                        total_b += b;
                    }
                }
                leds[led_x] = averageRGB(total_r, total_g, total_b, pixels_per_led_width, pixels_per_led_height);
            }
        }
        // Bottom of the led strip
        else if (led_y == NUM_LEDS_HEIGHT - 1)
        {
            for (int led_x = 0; led_x < NUM_LEDS_WIDTH; led_x++)
            {
                int total_r = 0, total_g = 0, total_b = 0;
                for (int pixel_y = screen_height - pixels_per_led_height; pixel_y < screen_height; pixel_y += EVERY_X_PIXELS)
                {
                    for (int pixel_x = pixels_per_led_width * led_x; pixel_x < pixels_per_led_width * (led_x + 1); pixel_x += EVERY_X_PIXELS)
                    {
                        u_int8_t r, g, b;
                        convertRGB(pixels[pixel_y][pixel_x], &r, &g, &b);
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
        for (int pixel_y = led_y * pixels_per_led_height; pixel_y < (led_y + 1) * pixels_per_led_height; pixel_y += EVERY_X_PIXELS)
        {
            for (int pixel_x = 0; pixel_x < pixels_per_led_width; pixel_x += EVERY_X_PIXELS)
            {
                u_int8_t r, g, b;
                convertRGB(pixels[pixel_y][pixel_x], &r, &g, &b);
                total_r += r;
                total_g += g;
                total_b += b;
            }
        }
        leds[NUM_LEDS_WIDTH*2 + NUM_LEDS_HEIGHT*2 - 1 - led_y] = averageRGB(total_r, total_g, total_b, pixels_per_led_width, pixels_per_led_height);

        // Right led for y height
        total_r = 0, total_g = 0, total_b = 0;
        for (int pixel_y = led_y * pixels_per_led_height; pixel_y < (led_y + 1) * pixels_per_led_height; pixel_y += EVERY_X_PIXELS)
        {
            for (int pixel_x = screen_width - pixels_per_led_width; pixel_x < screen_width; pixel_x += EVERY_X_PIXELS)
            {
                u_int8_t r, g, b;
                convertRGB(pixels[pixel_y][pixel_x], &r, &g, &b);
                total_r += r;
                total_g += g;
                total_b += b;
            }
        }
        leds[NUM_LEDS_WIDTH + led_y] = averageRGB(total_r, total_g, total_b, pixels_per_led_width, pixels_per_led_height);
    }
    return leds;
}

int main()
{
    Display *display;
    display = XOpenDisplay(NULL);
    if (!display)
    {
        fprintf(stderr, "unable to connect to display");
        return 7;
    }
    Window w;
    int x, y, i;
    unsigned m;
    Window root = XDefaultRootWindow(display);
    if (!root)
    {
        fprintf(stderr, "unable to open rootwindow");
        return 8;
    }
    if (!XQueryPointer(display, root, &root, &w, &x, &y, &i, &i, &m))
    {
        printf("unable to query pointer\n");
        return 9;
    }
    XImage *image;
    XWindowAttributes attr;
    XGetWindowAttributes(display, root, &attr);
    while (1)
    {
        image = XGetImage(display, root, 0, 0, attr.width, attr.height, AllPlanes, XYPixmap);
        if (!image)
        {
            printf("unable to get image\n");
            return 10;
        }
        const int pixels_per_led_width = attr.width / NUM_LEDS_WIDTH;
        const int pixels_per_led_height = attr.height / NUM_LEDS_HEIGHT;
        u_int32_t *pixels[attr.height];
        for (int y = 0; y < attr.height; y += EVERY_X_PIXELS)
        {
            pixels[y] = (int *)malloc(attr.width * sizeof(int));
            // Top and bottom of the screen
            if (y < pixels_per_led_height || y > attr.height - pixels_per_led_height)
            {
                for (int x = 0; x < attr.width; x += EVERY_X_PIXELS)
                {
                    pixels[y][x] = XGetPixel(image, x, y);
                }
            }
            // Middle of the screen (only take sides not the center)
            else
            {
                for (int x = 0; x < pixels_per_led_width; x += EVERY_X_PIXELS)
                {
                    pixels[y][x] = XGetPixel(image, x, y);
                }
                for (int x = attr.height - pixels_per_led_height; x < attr.height; x += EVERY_X_PIXELS)
                {
                    pixels[y][x] = XGetPixel(image, x, y);
                }
            }
        }
        u_int8_t **leds = pixelsToLeds(pixels, pixels_per_led_height, pixels_per_led_width, attr.height, attr.width);
        for (int i = 0; i < 100; i++)
        {
            printf("led %d = %x %x %x \n", i, leds[i][0], leds[i][1], leds[i][2]);
        }
        printf("width %d\n", attr.width);
        printf("height %d\n", attr.height);
        free(leds);
        sleep(1);
    }
    XCloseDisplay(display);
}
