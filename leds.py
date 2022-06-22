#!/usr/bin/env python3

import time
from rpi_ws281x import PixelStrip, Color
import sys

import sysv_ipc 

# Parametrizable LED strip configuration:
NUM_LEDS_WIDTH = 30
NUM_LEDS_HEIGHT = 20

# LED strip configuration:
LED_COUNT = 2*(NUM_LEDS_WIDTH + NUM_LEDS_HEIGHT) # Number of LED pixels.
LED_PIN = 18          # GPIO pin connected to the pixels (18 uses PWM!).
LED_FREQ_HZ = 800000  # LED signal frequency in hertz (usually 800khz)
LED_DMA = 10          # DMA channel to use for generating signal (try 10)
LED_BRIGHTNESS = 255  # Set to 0 for darkest and 255 for brightest
LED_INVERT = False    # True to invert the signal (when using NPN transistor level shift)
LED_CHANNEL = 0       # set to '1' for GPIOs 13, 19, 41, 45 or 53


# Define functions which animate LEDs in various ways.
def colorWipe(strip, color, wait_ms=50):
    """Wipe color across display a pixel at a time."""
    for i in range(strip.numPixels()):
        strip.setPixelColor(i, color)
        strip.show()
        time.sleep(wait_ms / 1000.0)


def theaterChase(strip, color, wait_ms=50, iterations=10):
    """Movie theater light style chaser animation."""
    for j in range(iterations):
        for q in range(3):
            for i in range(0, strip.numPixels(), 3):
                strip.setPixelColor(i + q, color)
            strip.show()
            time.sleep(wait_ms / 1000.0)
            for i in range(0, strip.numPixels(), 3):
                strip.setPixelColor(i + q, 0)


def wheel(pos):
    """Generate rainbow colors across 0-255 positions."""
    if pos < 85:
        return Color(pos * 3, 255 - pos * 3, 0)
    elif pos < 170:
        pos -= 85
        return Color(255 - pos * 3, 0, pos * 3)
    else:
        pos -= 170
        return Color(0, pos * 3, 255 - pos * 3)


def rainbow(strip, wait_ms=20, iterations=1):
    """Draw rainbow that fades across all pixels at once."""
    for j in range(256 * iterations):
        for i in range(strip.numPixels()):
            strip.setPixelColor(i, wheel((i + j) & 255))
        strip.show()
        time.sleep(wait_ms / 1000.0)


def rainbowCycle(strip, wait_ms=20, iterations=5):
    """Draw rainbow that uniformly distributes itself across all pixels."""
    for j in range(256 * iterations):
        for i in range(strip.numPixels()):
            strip.setPixelColor(i, wheel(
                (int(i * 256 / strip.numPixels()) + j) & 255))
        strip.show()
        time.sleep(wait_ms / 1000.0)


def theaterChaseRainbow(strip, wait_ms=50):
    """Rainbow movie theater light style chaser animation."""
    for j in range(256):
        for q in range(3):
            for i in range(0, strip.numPixels(), 3):
                strip.setPixelColor(i + q, wheel((i + j) % 255))
            strip.show()
            time.sleep(wait_ms / 1000.0)
            for i in range(0, strip.numPixels(), 3):
                strip.setPixelColor(i + q, 0)

def colorFromHex(hex : str) -> Color:
    return Color(*tuple(int(hex[i:i+2], 16) for i in (0, 2, 4)))

def main():
    colors = []
    length = 0

    shm_key = None
    for line in sys.stdin:
        if 'q' == line.rstrip():
            break
        if line.startswith('SHM_KEY'):
            shm_key = int(line.split()[1])
            print("SHM_KEY=", shm_key)
            break
    if shm_key is None:
        print('[LEDS] Shared Memory Key not received; Aborting')
        return
    
    
    while True:
        time.sleep(1)

        # Attempt to open shared memory segment
        # Size is number of LEDs * 4 bytes for RGBA
        memory = sysv_ipc.SharedMemory(shm_key, size=LED_COUNT*3)
        buffer = (memory.read())
        print(buffer)

    # print(colors, length)
    # if length != 0:
        
    #     # Create NeoPixel object with appropriate configuration.
    #     strip = PixelStrip(length, LED_PIN, LED_FREQ_HZ, LED_DMA, LED_INVERT, LED_BRIGHTNESS, LED_CHANNEL)
    #     # Intialize the library (must be called once before other functions).
    #     strip.begin()
    #     print('[LEDS] Press Ctrl-C to quit.')

    #     try:
    #         while True:
    #             # Fill in colors
    #             for index, color in enumerate(colors):
    #                 strip.setPixelColor(index, color)
    #             strip.show()

    #             # print('Color wipe animations.')
    #             # colorWipe(strip, Color(255, 0, 0))  # Red wipe
    #             # colorWipe(strip, Color(0, 255, 0))  # Green wipe
    #             # colorWipe(strip, Color(0, 0, 255))  # Blue wipe
    #             # print('Theater chase animations.')
    #             # theaterChase(strip, Color(127, 127, 127))  # White theater chase
    #             # theaterChase(strip, Color(127, 0, 0))  # Red theater chase
    #             # theaterChase(strip, Color(0, 0, 127))  # Blue theater chase
    #             # print('Rainbow animations.')
    #             # rainbow(strip)
    #             # rainbowCycle(strip)
    #             # theaterChaseRainbow(strip)

    #     except KeyboardInterrupt:
    #         colorWipe(strip, Color(0, 0, 0), 1)

# Main program logic follows:
if __name__ == '__main__':
    main()