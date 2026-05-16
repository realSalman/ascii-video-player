#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "../include/image.h"

// Characters to print
#define VALUE_CHARS " .-=+*x#$&X@"
#define N_VALUES (sizeof(VALUE_CHARS) - 1) // Exclude null

// Color ANSI codes
#define RESET "\x1b[0m"


typedef struct {
    double hue;
    double saturation;
    double value;
} hsv_t;


static inline double* get_max(double* a, double* b, double* c) {
    if ((*a >= *b) && (*a >= *c)) return a;
    else if (*b >= *c) return b;
    else return c;
}

static inline double* get_min(double* a, double* b, double* c) {
    if ((*a <= *b) && (*a <= *c)) return a;
    else if (*b <= *c) return b;
    else return c;
}


static hsv_t rgb_to_hsv(double red, double green, double blue) {
    hsv_t hsv;
    double* max = get_max(&red, &green, &blue);
    double* min = get_min(&red, &green, &blue);
    hsv.value = *max;
    double chroma = hsv.value - *min;
    if (fabs(hsv.value) < 1e-4) hsv.saturation = 0.0;
    else hsv.saturation = chroma / hsv.value;
    if (chroma < 1e-4) hsv.hue = 0.0;
    else if (max == &red) {
        hsv.hue = 60.0 * fmod((green - blue) / chroma, 6.0);
        if (hsv.hue < 0.0) hsv.hue += 360.0;
    } else if (max == &green) hsv.hue = 60.0 * (2.0 + (blue - red) / chroma);
    else hsv.hue = 60.0 * (4.0 + (red - green) / chroma);
    return hsv;
}


static void hsv_to_rgb(const hsv_t* hsv, double* r, double* g, double* b) {
    double c = hsv->value * hsv->saturation;
    double h_prime = hsv->hue / 60.0;
    double x = c * (1.0 - fabs(fmod(h_prime, 2.0) - 1.0));
    double r1, g1, b1;
    if (h_prime >= 0.0 && h_prime < 1.0) { r1 = c; g1 = x; b1 = 0.0; }
    else if (h_prime >= 1.0 && h_prime < 2.0) { r1 = x; g1 = c; b1 = 0.0; }
    else if (h_prime >= 2.0 && h_prime < 3.0) { r1 = 0.0; g1 = c; b1 = x; }
    else if (h_prime >= 3.0 && h_prime < 4.0) { r1 = 0.0; g1 = x; b1 = c; }
    else if (h_prime >= 4.0 && h_prime < 5.0) { r1 = x; g1 = 0.0; b1 = c; }
    else { r1 = c; g1 = 0.0; b1 = x; }
    double m = hsv->value - c;
    *r = r1 + m; *g = g1 + m; *b = b1 + m;
}


static void get_retro_rgb(const hsv_t* hsv, int* out_r, int* out_g, int* out_b) {
    hsv_t q = *hsv;
    q.value = 1.0;
    q.hue = round(q.hue / 60.0) * 60.0;
    if (q.hue >= 360.0) q.hue = 0.0;
    q.saturation = (q.saturation < 0.25) ? 0.0 : 1.0;
    double r, g, b;
    hsv_to_rgb(&q, &r, &g, &b);
    *out_r = (int)(r * 255); *out_g = (int)(g * 255); *out_b = (int)(b * 255);
}


static char get_ascii_char(double grayscale) {
    size_t index = (size_t) (grayscale * N_VALUES);
    if (index >= N_VALUES) index = N_VALUES - 1;
    return VALUE_CHARS[index];
}


static char get_sobel_angle_char(double sobel_angle) {
    if ((22.5 <= sobel_angle && sobel_angle <= 67.5) || (-157.5 <= sobel_angle && sobel_angle <= -112.5)) return '\\';
    else if ((67.5 <= sobel_angle && sobel_angle <= 112.5) || (-112.5 <= sobel_angle && sobel_angle <= -67.5)) return '_';
    else if ((112.5 <= sobel_angle && sobel_angle <= 157.5) || (-67.5 <= sobel_angle && sobel_angle <= -22.5)) return '/';
    else return '|';
}


void print_image(image_t* image, double edge_threshold, int use_retro_colors, int is_video) {
    if (is_video) printf("\x1b[H");

    int use_sobel = (edge_threshold < 4.0);
    double* sobel_x = NULL;
    double* sobel_y = NULL;
    image_t grayscale_img = {0};

    if (use_sobel) {
        grayscale_img = make_grayscale(image);
        sobel_x = calloc(image->width * image->height, sizeof(*sobel_x));
        sobel_y = calloc(image->width * image->height, sizeof(*sobel_y));
        get_sobel(&grayscale_img, sobel_x, sobel_y);
    }

    // Pre-allocate large buffer for the whole frame to avoid thousands of printf calls
    // ~20 bytes for color code + 1 for char + some extra
    size_t buffer_size = image->width * image->height * 32 + 2048;
    char* buffer = malloc(buffer_size);
    if (!buffer) return;
    char* p = buffer;

    for (size_t y = 0; y < image->height; y++) {
        for (size_t x = 0; x < image->width; x++) {
            double* pixel = get_pixel(image, x, y);
            int r, g, b;
            double grayscale;

            if (image->channels <= 2) {
                grayscale = pixel[0];
                r = g = b = (int)(pixel[0] * 255);
            } else {
                hsv_t hsv = rgb_to_hsv(pixel[0], pixel[1], pixel[2]);
                grayscale = hsv.value * hsv.value; // Contrast
                hsv.value = 1.0;
                if (use_retro_colors) {
                    get_retro_rgb(&hsv, &r, &g, &b);
                } else {
                    double rd, gd, bd;
                    hsv_to_rgb(&hsv, &rd, &gd, &bd);
                    r = (int)(rd * 255); g = (int)(gd * 255); b = (int)(bd * 255);
                }
            }

            char c = get_ascii_char(grayscale);

            if (use_sobel) {
                size_t idx = y * image->width + x;
                double sx = sobel_x[idx];
                double sy = sobel_y[idx];
                if (sx*sx + sy*sy >= edge_threshold * edge_threshold) {
                    c = get_sobel_angle_char(atan2(sy, sx) * 180. / M_PI);
                }
            }

            p += sprintf(p, "\x1b[38;2;%d;%d;%dm%c", r, g, b, c);
        }
        // Only print newline if it's not the last line
        if (y < image->height - 1) {
            *p++ = '\n';
        }
    }
    
    strcpy(p, RESET);
    p += strlen(RESET);

    fwrite(buffer, 1, p - buffer, stdout);
    fflush(stdout);

    free(buffer);
    if (use_sobel) {
        free(sobel_x); free(sobel_y);
        free_image(&grayscale_img);
    }
}
