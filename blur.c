#include "blur.h"

#include <stdint.h>
#include <stdlib.h>

#include "utils.h"

#define PXM_MEM_LIMIT (1024 * 0x100000)
#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)

static double clamp(double d, double min, double max) {
    return MIN(MAX(d, min), max);
}

static void x_blur_line_8(uint8_t *src, float *dest, size_t w, size_t radius)
{
    /* Calculates the average of the first pixel */
    dest[0] = (radius + 1) * src[0];
    for (size_t i = 1; i <= radius; ++i)
        dest[0] += src[i]; 
    dest[0] /= (2 * radius + 1);

    /* And now calculates the average of the next ones */
    for (size_t p = 1; p < w; ++p) {
        dest[p] = dest[p - 1];
        float next = src[p + radius < w ? p + radius : p + radius];
        float prev = src[p <= radius ? 0 : p - radius - 1];
        dest[p] += (next - prev) / ((2 * radius) + 1);
    }
}

static void y_blur_line_8(float *src, uint8_t *dest, float *line_buffer,
        size_t w, size_t rup, size_t rdw)
{
    memcpy(line_buffer, src, w * sizeof(float));

    size_t n = 1;
    for (size_t r = 1; r <= rup; ++r, ++n)
        for (size_t p = 0; p < w; ++p)
            line_buffer[p] += src[-(w * r) + p]; 

    for (size_t r = 1; r <= rdw; ++r, ++n)
        for (size_t p = 0; p < w; ++p)
            line_buffer[p] += src[w * r + p]; 

    for (size_t p = 0; p < w; ++p)
        dest[p] = (uint8_t) clamp(line_buffer[p] / n, 0, 255);
}

void PXM_blur(PXM_Image *img, size_t radius)
{   
    FAIL_MSG(img->max < 256,, "16 bit blur not implemented!");
    float *blur_buffer = (float*) malloc(img->w * img->len * sizeof(float));
    float *line_buffer = (float*) malloc(img->len * sizeof(float));
    FAIL(blur_buffer,);

    for (size_t line = 0; line < img->len; ++line) {
        x_blur_line_8(((uint8_t*)img->pixels) + line * img->w,
                blur_buffer + line * img->w, img->w, radius);
    }

    for (size_t line = 0; line < img->len; ++line) {
        y_blur_line_8(blur_buffer + line * img->w,
                ((uint8_t*)img->pixels) + line * img->w,
                line_buffer, img->w, MIN(line, radius),
                MIN(img->len - line, radius));
    }

    free(blur_buffer);
    free(line_buffer);
}

