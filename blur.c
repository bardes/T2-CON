#include "blur.h"

#include <stdint.h>
#include <stdlib.h>

#include "utils.h"

static double clamp(double d, double min, double max) {
    const double t = d < min ? min : d;
    return t > max ? max : t;
}

static void x_blur_line_8(uint8_t *pixels, size_t w,
        size_t radius, float *blur_buffer)
{
    /* Calculates the average of the first pixel */
    blur_buffer[0] = pixels[0];
    for (size_t i = 1; i <= radius; ++i)
        blur_buffer[0] += pixels[i]; 
    blur_buffer[0] /= (radius + 1);

    /* And now calculates the average of the next ones */
    for (size_t p = 1; p < w; ++p) {
        blur_buffer[p] = blur_buffer[p - 1];
        float next = pixels[p + radius > w ? p + w - 1 : p + radius];
        float prev = pixels[p < radius ? 0 : p - radius];
        blur_buffer[p] += (next - prev) / ((2 * radius) + 1);
    }
}

static void x_blur_line_16(uint16_t *pixels, size_t w,
        size_t radius, float *blur_buffer)
{
    /* Calculates the average of the first pixel */
    blur_buffer[0] = pixels[0];
    for (size_t i = 1; i <= radius; ++i)
        blur_buffer[0] += pixels[i]; 
    blur_buffer[0] /= (radius + 1);

    /* And now calculates the average of the next ones */
    for (size_t p = 1; p < w; ++p) {
        blur_buffer[p] = blur_buffer[p - 1];
        float next = p + radius > w ? 0 : pixels[p + radius];
        float prev = p < radius     ? 0 : pixels[p - radius];
        blur_buffer[p] += (next - prev) / ((2 * radius) + 1);
    }
}

static void y_blur_line_8(PXM_Image *img, size_t line,
        size_t radius, float *blur_buffer)
{
   if (line < radius) {
   } else if ((line + radius) >= img->len) {
   } else {
       for (size_t p = 0; p < img->w; ++p) {
           float acc = blur_buffer[img->w * line + p];
           for (size_t r = 1; r <= radius; ++r) {
               acc += blur_buffer[img->w * (line - r) + p];
               acc += blur_buffer[img->w * (line + r) + p];
           }
           ((uint8_t*) img->pixels)[img->w * line + p] = 
               (uint8_t) clamp(acc / (2 * radius + 1), 0, 255);
       }
   }
}

void PXM_blur(PXM_Image *img, size_t radius)
{   
    float *blur_buffer = (float*) malloc(img->w * img->len * sizeof(float));
    FAIL(blur_buffer,);

    if (img->max > 255) {
        for (size_t line = 0; line < img->len; ++line) {
            x_blur_line_16(((uint16_t*) img->pixels) + line * img->w,
                        img->w, radius, blur_buffer + line * img->w);
        }
    } else {
        for (size_t line = 0; line < img->len; ++line) {
            x_blur_line_8(((uint8_t*) img->pixels) + line * img->w,
                        img->w, radius, blur_buffer + line * img->w);
        }
        for (size_t line = 0; line < img->len; ++line) {
            y_blur_line_8(img, line, radius, blur_buffer);
        }
    }

    free(blur_buffer);
}

