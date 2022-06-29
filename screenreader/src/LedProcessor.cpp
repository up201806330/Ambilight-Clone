#include "LedProcessor.h"

#include <cmath>
#include <iostream>

std::pair<int, int> LedProcessor::indexToPixel(int i){
    if(i < 0){
        throw std::invalid_argument("i must be non-negative");
    } else if(i < NUM_LEDS_X){ // Bottom
        int led_x = NUM_LEDS_X - 1 - i;

        int x = PIXELS_PER_LED_X * led_x + PIXELS_PER_LED_X/2;
        int y = SIZE_Y - PIXELS_PER_LED_Y/2;

        return std::make_pair(x, y);
    } else if(i < NUM_LEDS_X+NUM_LEDS_Y){ // Left
        i -= NUM_LEDS_X;

        int led_y = NUM_LEDS_Y - 1 - i;

        int x = PIXELS_PER_LED_X/2;
        int y = led_y * PIXELS_PER_LED_Y + PIXELS_PER_LED_Y/2;

        return std::make_pair(x, y);
    } else if(i < 2*NUM_LEDS_X+NUM_LEDS_Y){ // Top
        i -= NUM_LEDS_X+NUM_LEDS_Y;

        int led_x = i;

        int x = PIXELS_PER_LED_X * led_x + PIXELS_PER_LED_X/2;
        int y = PIXELS_PER_LED_Y/2;

        return std::make_pair(x, y);
    } else if(i < 2*NUM_LEDS_X+2*NUM_LEDS_Y){ // Right
        i -= 2*NUM_LEDS_X+NUM_LEDS_Y;

        int led_y = i;

        int x = SIZE_X - PIXELS_PER_LED_X/2;
        int y = led_y * PIXELS_PER_LED_Y + PIXELS_PER_LED_Y/2;

        return std::make_pair(x, y);
    } else {
        throw std::invalid_argument("i must be smaller than 2*NUM_LEDS_X+2*NUM_LEDS_Y");
    }
}

LedProcessor::LedProcessor(ScreenProcessor &processor_, const int NUM_LEDS_X_, const int NUM_LEDS_Y_):
    processor(processor_),
    NUM_LEDS_X(NUM_LEDS_X_), NUM_LEDS_Y(NUM_LEDS_Y_),
    SIZE_X(processor.getWidth ()),
    SIZE_Y(processor.getHeight()),
    PIXELS_PER_LED_X(processor.getWidth () / NUM_LEDS_X),
    PIXELS_PER_LED_Y(processor.getHeight() / NUM_LEDS_Y)
{
}

void LedProcessor::update(){
    processor.update();
}

size_t LedProcessor::copy(uint8_t *dest){
    const size_t NUM_LEDS_TOTAL = 2*NUM_LEDS_X + 2*NUM_LEDS_Y;
    for(size_t i = 0; i < NUM_LEDS_TOTAL; ++i){
        const std::pair<int,int> pos = indexToPixel(i);
        const int &x = pos.first;
        const int &y = pos.second;
        const Color<uint8_t> &c = processor.getColor(x, y);
        *(dest++) = c.r;
        *(dest++) = c.g;
        *(dest++) = c.b;
    }
    return NUM_LEDS_TOTAL*3;
}
