#pragma once

#include "ScreenReader.h"
#include "Color.h"

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
    );

    int getWidth ();
    int getHeight();

    Color<uint8_t> getColor(const int x, const int y);

    void update();
};
