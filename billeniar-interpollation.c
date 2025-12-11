#include <stdio.h>
#include <stdlib.h>
#include <png.h>

unsigned char* read_png(const char* filename, int* width, int* height, int* channels) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return NULL;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    png_init_io(png, fp);
    png_read_info(png, info);

    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    *channels = png_get_channels(png, info);

    png_bytep* row_pointers = malloc(sizeof(png_bytep) * (*height));
    unsigned char* data = malloc((*width) * (*height) * (*channels));

    for (int y = 0; y < *height; y++) {
        row_pointers[y] = data + y * (*width) * (*channels);
    }

    png_read_image(png, row_pointers);

    free(row_pointers);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    return data;
}

void write_png(const char* filename, unsigned char* data, int width, int height, int channels) {
    FILE* fp = fopen(filename, "wb");

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    png_init_io(png, fp);

    png_set_IHDR(png, info, width, height,
                 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png, info);

    png_bytep* row_pointers = malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++)
        row_pointers[y] = data + y * width * channels;

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    free(row_pointers);
    fclose(fp);
}

unsigned char* bilinear_resize(unsigned char* input, int w, int h, int ch, int new_w, int new_h) {
    unsigned char* output = malloc(new_w * new_h * ch);

    for (int y = 0; y < new_h; y++) {
        float gy = (float)(y * (h - 1)) / (new_h - 1);
        int y0 = (int)gy;
        int y1 = y0 + 1;
        float dy = gy - y0;

        if (y1 >= h) y1 = h - 1;

        for (int x = 0; x < new_w; x++) {
            float gx = (float)(x * (w - 1)) / (new_w - 1);
            int x0 = (int)gx;
            int x1 = x0 + 1;
            float dx = gx - x0;

            if (x1 >= w) x1 = w - 1;

            for (int c = 0; c < ch; c++) {
                float P00 = input[(y0 * w + x0) * ch + c];
                float P10 = input[(y0 * w + x1) * ch + c];
                float P01 = input[(y1 * w + x0) * ch + c];
                float P11 = input[(y1 * w + x1) * ch + c];

                float R1 = P00 + dx * (P10 - P00);
                float R2 = P01 + dx * (P11 - P01);
                float P = R1 + dy * (R2 - R1);

                output[(y * new_w + x) * ch + c] = (unsigned char)P;
            }
        }
    }

    return output;
}

int main() {
    int w, h, ch;
    unsigned char* input = read_png("input.png", &w, &h, &ch);

    if (!input) {
        printf("Error reading PNG.\n");
        return 1;
    }

    int new_w = 800;
    int new_h = 600;

    unsigned char* output = bilinear_resize(input, w, h, ch, new_w, new_h);
    write_png("output.png", output, new_w, new_h, ch);

    free(input);
    free(output);

    printf("Done. Output saved as output.png\n");
    return 0;
}