#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

// Get wall time in seconds
double get_wall_time() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (double)time.tv_sec + (double)time.tv_usec / 1000000.0;
}

/*
Calculate Mean Squared Error between two images
*/
double calculate_mse(unsigned char* img1, unsigned char* img2, int width, int height, int channels) {
    double mse = 0.0;
    long total_pixels = (long)width * height * channels;
    
    for (long i = 0; i < total_pixels; i++) {
        double diff = (double)img1[i] - (double)img2[i];
        mse += diff * diff;
    }
    
    return mse / total_pixels;
}

/*
Calculate Peak Signal-to-Noise Ratio
*/
double calculate_psnr(double mse) {
    if (mse == 0.0) return INFINITY;
    return 10.0 * log10((255.0 * 255.0) / mse);
}

/*
Resize dengan metode Nearest Neighbor
*/
unsigned char* nearest_neighbor_resize(
    unsigned char* src, int src_w, int src_h,
    int channels, int new_w, int new_h)
{
    unsigned char* dst = malloc(new_w * new_h * channels);
    double x_ratio = (double)src_w / new_w;
    double y_ratio = (double)src_h / new_h;

    for (int i = 0; i < new_h; i++) {
        for (int j = 0; j < new_w; j++) {
            int src_x = (int)(j * x_ratio);
            int src_y = (int)(i * y_ratio);

            for (int c = 0; c < channels; c++) {
                dst[(i * new_w + j) * channels + c] = src[(src_y * src_w + src_x) * channels + c];
            }
        }
    }

    return dst;
}

/*
Resize dengan metode Bilinear Interpolation
*/
static inline double bilinear_interpolate(double x, double y,
    double Q11, double Q21,
    double Q12, double Q22) {
    double fx1 = Q11 + (Q21 - Q11) * x;
    double fx2 = Q12 + (Q22 - Q12) * x;
    return fx1 + (fx2 - fx1) * y;
}

unsigned char* bilinear_resize(
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

/*
Downsize image untuk mendapatkan ground truth reference
*/
unsigned char* downsize_image(unsigned char* src, int src_w, int src_h, int channels, int new_w, int new_h) {
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
    const char* output_nearest = "output_nearest.png";
    const char* output_bilinear = "output_bilinear.png";

    int width, height, channels;
    unsigned char* src = stbi_load(input_path, &width, &height, &channels, 0);
    if (!src) {
        fprintf(stderr, "Gagal membaca file %s\n", input_path);
        return 1;
    }

    printf("Gambar sumber: %dx%d (%d channel)\n\n", width, height, channels);

    // Create ground truth by downsizing then upsizing
    int small_w = width / 2;
    int small_h = height / 2;
    unsigned char* small = downsize_image(src, width, height, channels, small_w, small_h);
    
    int new_w = width; // upscale kembali ke ukuran original
    int new_h = height;

    // ===== Nearest Neighbor =====
    printf("=== Nearest Neighbor ===\n");
    double nn_start = get_wall_time();
    clock_t nn_cpu_start = clock();
    
    unsigned char* resized_nn = nearest_neighbor_resize(small, small_w, small_h, channels, new_w, new_h);
    
    clock_t nn_cpu_end = clock();
    double nn_end = get_wall_time();
    double nn_wall_time = nn_end - nn_start;
    double nn_cpu_time = ((double)(nn_cpu_end - nn_cpu_start)) / CLOCKS_PER_SEC;

    if (!stbi_write_png(output_nearest, new_w, new_h, channels, resized_nn, new_w * channels)) {
        fprintf(stderr, "Gagal menyimpan file %s\n", output_nearest);
        stbi_image_free(src);
        free(small);
        free(resized_nn);
        return 1;
    }

    // Calculate MSE for Nearest Neighbor vs Original
    double mse_nn = calculate_mse(resized_nn, src, new_w, new_h, channels);
    double psnr_nn = calculate_psnr(mse_nn);

    printf("Output: %s\n", output_nearest);
    printf("Wall Time: %.6f seconds\n", nn_wall_time);
    printf("CPU Time: %.6f seconds\n", nn_cpu_time);
    printf("MSE vs Original: %.2f\n", mse_nn);
    printf("PSNR: %.2f dB\n\n", psnr_nn);

    // ===== Bilinear Interpolation =====
    printf("=== Bilinear Interpolation ===\n");
    double bi_start = get_wall_time();
    clock_t bi_cpu_start = clock();
    
    unsigned char* resized_bi = bilinear_resize(small, small_w, small_h, channels, new_w, new_h);
    
    clock_t bi_cpu_end = clock();
    double bi_end = get_wall_time();
    double bi_wall_time = bi_end - bi_start;
    double bi_cpu_time = ((double)(bi_cpu_end - bi_cpu_start)) / CLOCKS_PER_SEC;

    if (!stbi_write_png(output_bilinear, new_w, new_h, channels, resized_bi, new_w * channels)) {
        fprintf(stderr, "Gagal menyimpan file %s\n", output_bilinear);
        stbi_image_free(src);
        free(small);
        free(resized_nn);
        free(resized_bi);
        return 1;
    }

    // Calculate MSE for Bilinear vs Original
    double mse_bi = calculate_mse(resized_bi, src, new_w, new_h, channels);
    double psnr_bi = calculate_psnr(mse_bi);

    printf("Output: %s\n", output_bilinear);
    printf("Wall Time: %.6f seconds\n", bi_wall_time);
    printf("CPU Time: %.6f seconds\n", bi_cpu_time);
    printf("MSE vs Original: %.2f\n", mse_bi);
    printf("PSNR: %.2f dB\n\n", psnr_bi);

    // ===== MSE Comparison =====
    printf("=== Mean Squared Error (MSE) ===\n");
    double mse_diff = calculate_mse(resized_nn, resized_bi, new_w, new_h, channels);
    double psnr_diff = calculate_psnr(mse_diff);
    
    printf("MSE antara Nearest Neighbor dan Bilinear: %.2f\n", mse_diff);
    printf("PSNR: %.2f dB\n\n", psnr_diff);

    // ===== Perbandingan =====
    printf("=== Perbandingan ===\n");
    printf("Kecepatan:\n");
    printf("  Nearest Neighbor lebih cepat: %.2fx\n\n", bi_wall_time / nn_wall_time);
    printf("Kualitas terhadap Original:\n");
    printf("  Nearest Neighbor:\n");
    printf("    - MSE: %.2f\n", mse_nn);
    printf("    - PSNR: %.2f dB\n", psnr_nn);
    printf("  Bilinear Interpolation:\n");
    printf("    - MSE: %.2f (%.2f%% lebih baik)\n", mse_bi, ((mse_nn - mse_bi) / mse_nn) * 100);
    printf("    - PSNR: %.2f dB (%.2f dB lebih tinggi)\n\n", psnr_bi, psnr_bi - psnr_nn);
    printf("Kesimpulan:\n");
    printf("  - Nearest Neighbor: Lebih cepat, hasil bergerigi\n");
    printf("  - Bilinear: Lebih lambat, hasil lebih halus dan akurat\n");

    stbi_image_free(src);
    free(small);
    free(resized_nn);
    free(resized_bi);

    return 0;
}
