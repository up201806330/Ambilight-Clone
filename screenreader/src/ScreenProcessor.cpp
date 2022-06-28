#include "ScreenProcessor.h"

ScreenProcessor::ScreenProcessor(
    ScreenReader &reader_,
    int colorWidth_,
    int colorHeight_
):
    reader(reader_),
    colorWidth (colorWidth_ ),
    colorHeight(colorHeight_),
    screenWidth (reader.getScreenWidth ()),
    screenHeight(reader.getScreenHeight())
{}

int ScreenProcessor::getWidth (){ return reader.getScreenWidth (); }
int ScreenProcessor::getHeight(){ return reader.getScreenHeight(); }

Color<uint8_t> ScreenProcessor::getColor(const int x, const int y){
    int r = 0, g = 0, b = 0, a = 0;
    size_t n = 0;
    for(int deltaX = -colorWidth/2; deltaX < colorWidth/2; ++deltaX){
        for(int deltaY = -colorHeight/2; deltaY < colorHeight/2; ++deltaY){
            const int xPixel = x + deltaX;
            const int yPixel = y + deltaY;
            if(
                0 <= xPixel && xPixel < screenWidth  &&
                0 <= yPixel && yPixel < screenHeight
            ) {
                Color<uint8_t> c = reader.getPixel(xPixel, yPixel);
                r += c.r;
                g += c.g;
                b += c.b;
                a += c.a;
                n++;
            }
        }
    }
    r /= n;
    g /= n;
    b /= n;
    a /= n;
    return Color<uint8_t>(r, g, b, a);
}

void ScreenProcessor::update(){
    reader.update();
}
