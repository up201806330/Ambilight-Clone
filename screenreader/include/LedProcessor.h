#pragma once

#include "ScreenProcessor.h"

#include <chrono>
#include <vector>

class LedProcessor {
    ScreenProcessor &processor;
    const int NUM_LEDS_X;
    const int NUM_LEDS_Y;
    const float SCALE_FACTOR;
    const int SIZE_X;
    const int SIZE_Y;
    const int PIXELS_PER_LED_X;
    const int PIXELS_PER_LED_Y;

    std::vector<Color<float>> leds;

    std::pair<int, int> indexToPixel(int i);

    static constexpr float NANOS_TO_SECONDS = 1.0e-9;

    std::chrono::high_resolution_clock::time_point prevTimePoint =
        std::chrono::high_resolution_clock::now();
    float getWeight();
public:
    /**
     * @brief Construct a new Led Processor object
     * 
     * @param processor_ 
     * @param NUM_LEDS_X_ 
     * @param NUM_LEDS_Y_ 
     * @param SCALE_FACTOR_ Scale factor for exponential decay. The larger it is, the
     *                      slower it updates. Default value can be 1, but best value is around 0.05
     */
    LedProcessor(ScreenProcessor &processor_, const int NUM_LEDS_X_, const int NUM_LEDS_Y_, const float SCALE_FACTOR_);

    void update();

    size_t copy(uint8_t *dest);
};
