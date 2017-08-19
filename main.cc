/*
Info about SSE and AVX Vectorization and optimization of code:
https://tech.io/playgrounds/283/sse-avx-vectorization/what-is-sse-and-avx
*/
#pragma GCC optimize("O3","unroll-loops","omit-frame-pointer","inline") // Optimization flags
#pragma GCC option("arch=native","tune=native","no-zero-upper") // Enable AVX
#pragma GCC target("avx")  //Enable AVX
#include <x86intrin.h> // AVX/SSE Extensions
#include <omp.h>

#include "EasyBMP.hpp"
#include <iostream>
#include <cmath>
#include <chrono>

#define ALWAYS_INLINE __attribute__((always_inline))

using namespace std;

static const double LN_2 = 0.693147180559945309417232121458; // ln(2)
static const double LN_2_inverse = 1 / LN_2; // 1 / ln(2)

static const int MAX_ITER = 60;

static const int RE_START = -2;
static const int RE_END = 1;
static const int IM_START = -1;
static const int IM_END = 1;

struct Complex {
    double re, im;
};

inline ALWAYS_INLINE Complex operator*(const Complex& a, const Complex& b) {
    return (Complex){a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re};
}

inline ALWAYS_INLINE Complex operator+(const Complex& a, const Complex& b) {
    return (Complex){a.re + b.re, a.im + b.im};
}

inline ALWAYS_INLINE double abs(const Complex& c) {
    return sqrt(c.re * c.re + c.im * c.im);
}

inline ALWAYS_INLINE double abs_square(const Complex& c) {
    return c.re * c.re + c.im * c.im;
}

int mandelbrot(const Complex& c) 
{
    int n = 0;
    Complex z = c;
    // abs(z) <= 2  -->  sqrt(z.re^2 + z.im^2) <= 2  -->  z.re^2 + z.im^2 <= 2^2
    while (abs_square(z) <= 4 and n < MAX_ITER) {
        z = z*z + c;
        ++n;
    }
    return n;
}

int main() 
{
	const int MULTIPLIER = 1;
    const int WIDTH = 1024 * MULTIPLIER;
    const int HEIGHT = 720 * MULTIPLIER;

    EasyBMP::Image image(WIDTH, HEIGHT, "mandelbrot.bmp", EasyBMP::RGBColor(0, 0, 0));
    // Multiplication is faster than division, just divide once
    double WIDTH_inverse = 1.0 / (double)WIDTH;
    double HEIGHT_inverse = 1.0 / (double)HEIGHT;
    double x_factor = WIDTH_inverse * (RE_END - RE_START);
    double y_factor = HEIGHT_inverse * (IM_END - IM_START);

    cout << "Starting Mandelbrot computation..." << endl;

    auto start = chrono::high_resolution_clock::now();

    int operations = 0;

    #pragma omp parallel for num_threads(8)
    for (int y = 0; y < HEIGHT; ++y) {
    	int tid = omp_get_thread_num();

    	#pragma omp atomic
		++operations;

    	if (tid == 0 && operations & 16) {
    	   cout << '\r' << operations * 100 / HEIGHT << '%';
    	}

        for (int x = 0; x < WIDTH; ++x) {
            double re = RE_START + x * x_factor;
            double im = IM_START + y * y_factor;
            Complex c = {re, im};

            int m = mandelbrot(c);
            int color = 255 - (int)(m * 255 / MAX_ITER);

            //color *= log(log |Z|) / log 2

            image.SetPixel(x, y, EasyBMP::RGBColor(color, color, color));
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto millis = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << '\r' << "100%" << '\n' << "Mandelbrot computed, time delayed: " << millis / 1000 << "s " << millis % 1000 << "ms" << endl;
    cout << "Writing mandelbrot.bmp to disk..." << endl;

    image.Write();

    cout << "mandelbrot.bmp saved succesfully." << endl;
}