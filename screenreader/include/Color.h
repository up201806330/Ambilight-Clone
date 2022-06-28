#pragma once

template<class T>
struct Color {
    T r, g, b, a;
    Color(uint32_t color){
        a = (uint8_t)((color & 0xFF000000) >> 24);
        r = (uint8_t)((color & 0x00FF0000) >> 16);
        g = (uint8_t)((color & 0x0000FF00) >>  8);
        b = (uint8_t)((color & 0x000000FF)      );
    }
    Color(T r_, T g_, T b_, T a_)
        :r(r_),g(g_),b(b_),a(a_){}
    
    template<class U>
    Color<T> weightedAverage(const Color<U> &c, const float &w){
        float rf = float(r)*(1.0f-w) + float(c.r)*w;
        float gf = float(g)*(1.0f-w) + float(c.g)*w;
        float bf = float(b)*(1.0f-w) + float(c.b)*w;
        float af = float(a)*(1.0f-w) + float(c.a)*w;

        rf = std::min(std::max(rf, 0.0f), 255.0f);
        gf = std::min(std::max(gf, 0.0f), 255.0f);
        bf = std::min(std::max(bf, 0.0f), 255.0f);
        af = std::min(std::max(af, 0.0f), 255.0f);
        Color<T> ret(
            (T)rf,            
            (T)gf,
            (T)bf,
            (T)af
        );

        return ret;
    }
};
