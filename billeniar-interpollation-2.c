#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
Fungsi interpolasi bilinear untuk 1 kanal
*/
static inline double bilinear_interpolate(double x, double y,
    double Q11, double Q21,
    double Q12, double Q22) {
    double fx1 = Q11 + (Q21 - Q11) * x;
    double fx2 = Q12 + (Q22 - Q12) * x;
    return fx1 + (fx2 - fx1) * y;
}

/*
Resize bilinear untuk gambar png
*/
unsigned char* bilinear_resize_rgb(
    unsigned char* src, int src_w, int src_h,
    int channels, int new_w, int new_h)
{
    unsigned char* dst = malloc(new_w * new_h * channels);
    double x_ratio = (double)(src_w - 1) / (new_w - 1);
    double y_ratio = (double)(src_h - 1) / (new_h - 1);

    for (int i = 0; i < new_h; i++) {
        for (int j = 0; j < new_w; j++) {
            double src_x = j * x_ratio;
            double src_y = i * y_ratio;

            int x1 = (int)src_x;
            int y1 = (int)src_y;
            int x2 = (x1 == src_w - 1) ? x1 : x1 + 1;
            int y2 = (y1 == src_h - 1) ? y1 : y1 + 1;

            double dx = src_x - x1;
            double dy = src_y - y1;

            for (int c = 0; c < channels; c++) {
                double Q11 = src[(y1 * src_w + x1) * channels + c];
                double Q21 = src[(y1 * src_w + x2) * channels + c];
                double Q12 = src[(y2 * src_w + x1) * channels + c];
                double Q22 = src[(y2 * src_w + x2) * channels + c];

                double val = bilinear_interpolate(dx, dy, Q11, Q21, Q12, Q22);
                if (val < 0) val = 0;
                if (val > 255) val = 255;

                dst[(i * new_w + j) * channels + c] = (unsigned char)(val + 0.5);
            }
        }
    }

    return dst;
}

int main() {
    const char* input_path = "input2.png";
    const char* output_path = "output2.png";

    int width, height, channels;
    unsigned char* src = stbi_load(input_path, &width, &height, &channels, 0);
    if (!src) {
        fprintf(stderr, "Gagal membaca file %s\n", input_path);
        return 1;
    }

    printf("Gambar sumber: %dx%d (%d channel)\n", width, height, channels);

    int new_w = width * 2; // scaling faktor 2
    int new_h = height * 2;

    unsigned char* resized = bilinear_resize_rgb(src, width, height, channels, new_w, new_h);

    if (!stbi_write_png(output_path, new_w, new_h, channels, resized, new_w * channels)) {
        fprintf(stderr, "Gagal menyimpan file %s\n", output_path);
        stbi_image_free(src);
        free(resized);
        return 1;
    }

    printf("Gambar hasil disimpan ke: %s\n", output_path);

    stbi_image_free(src);
    free(resized);

    return 0;
}
