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
#include <vector>
#include <cmath>
#include <chrono>
#include <unistd.h>

#define ALWAYS_INLINE __attribute__((always_inline))

using namespace std;

static const double LN_2 = 0.693147180559945309417232121458; // ln(2)
static const double LN_2_inverse = 1 / LN_2; // 1 / ln(2)

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

int mandelbrot(int max_iter, const Complex& c) 
{
    int n = 0;
    Complex z = c;
    // abs(z) <= 2  -->  sqrt(z.re^2 + z.im^2) <= 2  -->  z.re^2 + z.im^2 <= 2^2
    while (abs_square(z) <= 4 and n < max_iter) {
        z = z*z + c;
        ++n;
    }
    return n;
}

void show_help(char* argv_0, int width, int height, int max_iter)
{
    cerr << "Usage: " << argv_0
        << "\n\t[-g flag -> use it to start image generation, if not used program will show help]" 
        << "\n\t[-c flag -> if used mandelbrot will use rgb instead gray scale]" 
        << "\n\t[-w argument (int) -> if is set, default (" << width << ") image width will be replaced]"
        << "\n\t[-h argument (int) -> if is set, default (" << height << ") image height will be replaced]"
        << "\n\t[-h argument (int) -> if is set, image height and width will be by it's value]"
        << "\n\t[-i argument (int) -> if is set, default maximum iterations (" << max_iter << ") will be replaced]"
        << "\n\t[-o argument (string) -> if is set, default (mandelbrot.bmp) output filename will be replaces]"
        << endl;
}

int main(int argc, char** argv) 
{
    int multiplier = 1; // Used to test bigger image sizes
    int width = 1024;
    int height = 720;
    int max_iter = 60;
    bool colored = false;
    bool generate = false;
    string out_filename = "mandelbrot.bmp";

    int opt;
    while ((opt = getopt(argc, argv, "gcw:h:m:i:o:")) != -1) {
        switch (opt) {
            case 'g':
                generate = true;
                break;
            case 'c': // Must be colored
                colored = true;  
                break;
            case 'w':
                width = stoi(optarg);
                break;
            case 'h':
                height = stoi(optarg);
                break;
            case 'm':
                multiplier = stoi(optarg);
                break;
            case 'i':
                max_iter = stoi(optarg);
                break;
            case 'o':
                out_filename = string(optarg);
                break;
            default:
                show_help(argv[0], width, height, max_iter);
                return 1;
        }
    }

    width *= multiplier;
    height *= multiplier;

    if (not generate) {
        show_help(argv[0], width, height, max_iter);
        return 0;
    }

    vector<EasyBMP::RGBColor> colors;

    if (colored) {
        colors = vector<EasyBMP::RGBColor>(max_iter);
        for (int i = 0; i < max_iter; ++i) {
            // 255*i/(i+8) = ((i<<8)-i)/(i+8)
            colors[i] = EasyBMP::RGBColor(i, ((i<<8)-i)/(i+8), 255);
        }
    }

    EasyBMP::Image image(width, height, out_filename, EasyBMP::RGBColor(0, 0, 0));
    // Multiplication is faster than division, just divide once
    double width_inverse = 1.0 / (double)width;
    double height_inverse = 1.0 / (double)height;
    double x_factor = width_inverse * (RE_END - RE_START);
    double y_factor = height_inverse * (IM_END - IM_START);

    cout << "Starting Mandelbrot computation..." << endl;

    auto start = chrono::high_resolution_clock::now();

    int operations = 0;

    #pragma omp parallel for num_threads(8)
    for (int y = 0; y < height; ++y) {
    	int tid = omp_get_thread_num();

    	#pragma omp atomic
		++operations;

    	if (tid == 0 && operations & 16) {
    	   cout << '\r' << operations * 100 / height << '%';
    	}

        for (int x = 0; x < width; ++x) {
            double re = RE_START + x * x_factor;
            double im = IM_START + y * y_factor;
            Complex c = {re, im};

            int m = mandelbrot(max_iter, c);

            if (colored) {
                image.SetPixel(x, y, colors[m]);
            }
            else {
                int grayscale = 255 - (int)(m * 255 / max_iter);
                image.SetPixel(x, y, EasyBMP::RGBColor(grayscale, grayscale, grayscale));
            }
        }
    }

    auto end = chrono::high_resolution_clock::now();
    auto millis = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << '\r' << "100%" << '\n' << "Mandelbrot computed, time delayed: " << millis / 1000 << "s " << millis % 1000 << "ms" << endl;
    cout << "Writing " << out_filename << " to disk..." << endl;

    image.Write();

    cout << out_filename << " saved succesfully." << endl;

#ifdef _WIN32
    // Free memory before open it, because if image buffer it's more than 1 GB, open the image in other process
    // like image viewer can crash because we will have 2 * image buffer size memory used.
    // Deleting the image buffer we can solve this potential issue.
    image.~Image();

    system(("start " + out_filename).c_str());
#endif
}