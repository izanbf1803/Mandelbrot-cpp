#include "EasyBMP.hpp"
#include <iostream>
#include <complex>

using namespace std;
using EasyBMP::Image;
using EasyBMP::RGBColor;

static const int MAX_ITER = 80;

static const int WIDTH = 1000;
static const int HEIGHT = 800;

static const int RE_START = -2;
static const int RE_END = 1;
static const int IM_START = -1;
static const int IM_END = 1;

/*struct Complex {
    double re, im;
};*/

int mandelbrot(complex<double> c) 
{
    int n = 0;
    complex<double> z(0, 0);
    while (abs(z) <= 2 and n < MAX_ITER) {
        z = z*z + c;
        ++n;
    }
    return n;
}

int main() 
{
    Image image(WIDTH, HEIGHT, "mandelbrot.bmp", RGBColor(0, 0, 0));

    for (double y = 0; y < HEIGHT; ++y) {
        for (double x = 0; x < WIDTH; ++x) {
            double re = RE_START + (x / WIDTH) * (RE_END - RE_START);
            double im = IM_START + (y / HEIGHT) * (IM_END - IM_START);
            complex<double> c(re, im);

            int m = mandelbrot(c);
            int color = 255 - (int)(m * 255 / MAX_ITER);

            image.SetPixel(x, y, RGBColor(color, color, color));
        }
    }

    image.Write();
}