#pragma once

#include "ScreenProcessor.h"

#include <chrono>
#include <vector>

class LedProcessor {
    ScreenProcessor &processor;
    const int NUM_LEDS_X;
    const int NUM_LEDS_Y;
    const int SIZE_X;
    const int SIZE_Y;
    const int PIXELS_PER_LED_X;
    const int PIXELS_PER_LED_Y;

    std::pair<int, int> indexToPixel(int i);
public:
    /**
     * @brief Construct a new Led Processor object
     * 
     * @param processor_ 
     * @param NUM_LEDS_X_ 
     * @param NUM_LEDS_Y_ 
     */
    LedProcessor(ScreenProcessor &processor_, const int NUM_LEDS_X_, const int NUM_LEDS_Y_);

    void update();

    size_t copy(uint8_t *dest);
};
