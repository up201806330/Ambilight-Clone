#!/usr/bin/env python3

import time
from multiprocessing import shared_memory
import posix_ipc
import math

from rpi_ws281x import Color, PixelStrip

# Parametrizable LED strip configuration:
NUM_LEDS_WIDTH = 32
NUM_LEDS_HEIGHT = 20

# LED strip configuration:
LED_COUNT = 2*(NUM_LEDS_WIDTH + NUM_LEDS_HEIGHT) # Number of LED pixels.
LED_PIN = 18          # GPIO pin connected to the pixels (18 uses PWM!).
LED_FREQ_HZ = 800000  # LED signal frequency in hertz (usually 800khz)
LED_DMA = 10          # DMA channel to use for generating signal (try 10)
LED_BRIGHTNESS = 255  # Set to 0 for darkest and 255 for brightest
LED_INVERT = False    # True to invert the signal (when using NPN transistor level shift)
LED_CHANNEL = 0       # set to '1' for GPIOs 13, 19, 41, 45 or 53

# Exponential decay
EXP_DECAY_SCALE_FACTOR = 0.05

def colorWipe(strip, color, wait_ms=50):
    """Wipe color across display a pixel at a time."""
    for i in range(strip.numPixels()):
        strip.setPixelColor(i, color)
        strip.show()
        time.sleep(wait_ms / 1000.0)

def colorFromHex(hex : str, intensity : int) -> Color:
    return Color(*tuple(int(int(hex[i:i+2], 16) * intensity) for i in (0, 2, 4)))

def getWeight():
    nowTime = time.time()
    delta = nowTime-getWeight.prevTime
    w = 1-math.exp(-delta/EXP_DECAY_SCALE_FACTOR)
    getWeight.prevTime = nowTime
    return w
getWeight.prevTime = time.time()

def main():
    USE_LEDS = True

    # Hardcoded
    shm_name = "/shm_leds"
    sem_name = "/sem_leds"
    
    # Initialize shared memory & semaphore
    shm = shared_memory.SharedMemory(shm_name, create=False)
    sem = posix_ipc.Semaphore(sem_name)

    # Create NeoPixel object with appropriate configuration.
    if USE_LEDS:
        strip = PixelStrip(LED_COUNT, LED_PIN, LED_FREQ_HZ, LED_DMA, LED_INVERT, LED_BRIGHTNESS, LED_CHANNEL)
        strip.begin()
        print('[LEDS] Press Ctrl-C to quit.')
    
    colors    = [(0,0,0) for _ in range(LED_COUNT)]
    shmColors = [(0,0,0) for _ in range(LED_COUNT)]

    try:
        while True:
            sem.acquire()
            intensity = shm.buf[LED_COUNT*3] / 100

            for i in range(LED_COUNT):
                shmColors[i] = tuple(bytes(shm.buf[3*i:3*i+3]))
            sem.release()

            w = getWeight()
            for i in range(LED_COUNT):
                colors[i] = (
                    (1-w) * colors[i][0] + w * shmColors[i][0],
                    (1-w) * colors[i][1] + w * shmColors[i][1],
                    (1-w) * colors[i][2] + w * shmColors[i][2],
                )

            if USE_LEDS:
                for index, color in enumerate(colors):
                    c = Color(
                        int(color[0]*intensity),
                        int(color[1]*intensity),
                        int(color[2]*intensity)
                    )
                    strip.setPixelColor(index, c)
                strip.show()
            else:
                print(colors)

            #time.sleep(0.01)

    except KeyboardInterrupt:
        colorWipe(strip, Color(0, 0, 0), 1)
        shm.close()
        sem.close()
        

if __name__ == '__main__':
    main()
