#include "EasyBMP.hpp"
#include <iostream>
#include <cmath>
#include <chrono>

using namespace std;

static const int MAX_ITER = 80;

static const int WIDTH = 1000;
static const int HEIGHT = 800;

static const int RE_START = -2;
static const int RE_END = 1;
static const int IM_START = -1;
static const int IM_END = 1;

struct Complex {
    double re, im;
};

inline __attribute__((always_inline)) Complex operator*(const Complex& a, const Complex& b) {
    return (Complex){a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re};
}

inline __attribute__((always_inline)) Complex operator+(const Complex& a, const Complex& b) {
    return (Complex){a.re + b.re, a.im + b.im};
}

inline __attribute__((always_inline)) double abs(const Complex& c) {
    return sqrt(c.re * c.re + c.im * c.im);
}

int mandelbrot(const Complex& c) 
{
    int n = 0;
    Complex z = {0, 0};
    while (abs(z) <= 2 and n < MAX_ITER) {
        z = z*z + c;
        ++n;
    }
    return n;
}

int main() 
{
    EasyBMP::Image image(WIDTH, HEIGHT, "mandelbrot.bmp", EasyBMP::RGBColor(0, 0, 0));

    auto start = chrono::high_resolution_clock::now();

    for (double y = 0; y < HEIGHT; ++y) {
        for (double x = 0; x < WIDTH; ++x) {
            double re = RE_START + (x / WIDTH) * (RE_END - RE_START);
            double im = IM_START + (y / HEIGHT) * (IM_END - IM_START);
            Complex c = {re, im};

            int m = mandelbrot(c);
            int color = 255 - (int)(m * 255 / MAX_ITER);

            image.SetPixel(x, y, EasyBMP::RGBColor(color, color, color));
        }
    }

    auto end = chrono::high_resolution_clock::now();

    auto millis = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "mandelbrot.bmp saved, time delayed: " << millis / 1000 << "s " << millis % 1000 << "ms" << endl;

    image.Write();
}