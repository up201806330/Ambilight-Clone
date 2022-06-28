#include "LedProcessor.h"

#include <cmath>

using hrc = std::chrono::high_resolution_clock;

std::pair<int, int> LedProcessor::indexToPixel(int i){
    if(i < 0){
        throw std::invalid_argument("i must be non-negative");
    } else if(i < NUM_LEDS_X){
        int led_x = NUM_LEDS_X - 1 - i;

        int x = PIXELS_PER_LED_X * led_x + PIXELS_PER_LED_X/2;
        int y = SIZE_Y - PIXELS_PER_LED_Y/2;

        return std::make_pair(x, y);
    } else if(i < NUM_LEDS_X+NUM_LEDS_Y){
        i -= NUM_LEDS_X;

        int led_y = NUM_LEDS_Y - 1 - i;

        int x = PIXELS_PER_LED_X/2;
        int y = led_y * PIXELS_PER_LED_Y + PIXELS_PER_LED_Y/2;

        return std::make_pair(x, y);
    } else if(i < 2*NUM_LEDS_X+NUM_LEDS_Y){
        i -= NUM_LEDS_X+NUM_LEDS_Y;

        int led_x = i;

        int x = PIXELS_PER_LED_X * led_x + PIXELS_PER_LED_X/2;
        int y = PIXELS_PER_LED_Y/2;

        return std::make_pair(x, y);
    } else if(i < 2*NUM_LEDS_X+2*NUM_LEDS_Y){
        i -= 2*NUM_LEDS_X+NUM_LEDS_Y;

        int led_y = i;

        int x = SIZE_X - PIXELS_PER_LED_X/2;
        int y = led_y * PIXELS_PER_LED_Y + PIXELS_PER_LED_Y/2;

        return std::make_pair(x, y);
    } else {
        throw std::invalid_argument("i must be smaller than 2*NUM_LEDS_X+2*NUM_LEDS_Y");
    }
}

float LedProcessor::getWeight(){
    hrc::time_point now = hrc::now();
    
    const float delta = float(std::chrono::duration_cast<std::chrono::nanoseconds> (now - prevTimePoint).count())*NANOS_TO_SECONDS;
    const float w = 1-exp(-delta/SCALE_FACTOR);
    
    prevTimePoint = now;
    
    return w;
}

LedProcessor::LedProcessor(ScreenProcessor &processor_, const int NUM_LEDS_X_, const int NUM_LEDS_Y_, const float SCALE_FACTOR_):
    processor(processor_),
    NUM_LEDS_X(NUM_LEDS_X_), NUM_LEDS_Y(NUM_LEDS_Y_),
    SCALE_FACTOR(SCALE_FACTOR_),
    SIZE_X(processor.getWidth ()),
    SIZE_Y(processor.getHeight()),
    PIXELS_PER_LED_X(processor.getWidth () / NUM_LEDS_X),
    PIXELS_PER_LED_Y(processor.getHeight() / NUM_LEDS_Y),
    leds(2*NUM_LEDS_X_+2*NUM_LEDS_Y_, Color<float>(0))
{
}

void LedProcessor::update(){
    processor.update();
    const float w = getWeight();
    for(size_t i = 0; i < leds.size(); ++i){
        const std::pair<int,int> pos = indexToPixel(i);
        const int &x = pos.first;
        const int &y = pos.second;
        leds[i] = leds[i].weightedAverage(processor.getColor(x, y), w);
    }
}

size_t LedProcessor::copy(uint8_t *dest){
    for(size_t i = 0; i < leds.size(); ++i){
        const Color<float> &c = leds[i];
        *(dest++) = (uint8_t)c.r;
        *(dest++) = (uint8_t)c.g;
        *(dest++) = (uint8_t)c.b;
    }
    return 3*leds.size();
}
