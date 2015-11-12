#include "blur.h"

#include <aio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "utils.h"

#define PXM_WINDOW_SIZE (1000)
#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)

static double clamp(double d, double min, double max) {
    return MIN(MAX(d, min), max);
}

static void x_blur_line_grey(uint8_t *src, float *dest, size_t w, size_t radius)
{
    /* Calculates the average of the first pixel */
    dest[0] = (radius + 1) * src[0];
    for (size_t i = 1; i <= radius; ++i)
        dest[0] += src[i]; 
    dest[0] /= (2 * radius + 1);

   /* And now calculates the average of the next ones */
    for (size_t p = 1; p < w; ++p) {
        dest[p] = dest[p - 1];
        float next = src[p + radius < w ? p + radius : p];
        float prev = src[p <= radius ? 0 : p - radius - 1];
        dest[p] += (next - prev) / ((2 * radius) + 1);
    }
}

static void x_blur_line_color(uint8_t *src, float *dest, size_t w, size_t radius)
{
    /* Calculates the average of the first pixel */
    dest[0] = (radius + 1) * src[0];
    dest[1] = (radius + 1) * src[1];
    dest[2] = (radius + 1) * src[2];

    for (size_t i = 3; i <= radius * 3; i += 3) {
        dest[0] += src[i + 0]; 
        dest[1] += src[i + 1]; 
        dest[2] += src[i + 2]; 
    }

    dest[0] /= (2 * radius + 1);
    dest[1] /= (2 * radius + 1);
    dest[2] /= (2 * radius + 1);

    /* Averages the left border */
    size_t p = 3;
    for (; p < 3 * radius + 3; p += 3) {
        dest[p + 0] = dest[p - 3] +
            (src[p + 0 + radius * 3] - src[0]) / ((2 * radius) + 1.0f);

        dest[p + 1] = dest[p - 2] +
            (src[p + 1 + radius * 3] - src[1]) / ((2 * radius) + 1.0f);

        dest[p + 2] = dest[p - 1] +
            (src[p + 2 + radius * 3] - src[2]) / ((2 * radius) + 1.0f);
    }

    /* And now calculates the average of the next ones */
    for (; p < (w - radius) * 3; p += 3) {
        dest[p + 0] = dest[p - 3] +
            (src[p + 0 + radius * 3] - src[p + 0 - (radius + 1) * 3])
                        / ((2 * radius) + 1.0f);

        dest[p + 1] = dest[p - 2] +
            (src[p + 1 + radius * 3] - src[p + 1 - (radius + 1) * 3])
                        / ((2 * radius) + 1.0f);

        dest[p + 2] = dest[p - 1] +
            (src[p + 2 + radius * 3] - src[p + 2 - (radius + 1) * 3])
                        / ((2 * radius) + 1.0f);
    }

    /* Averages the right border */
    for (; p < w * 3; p += 3) {
        dest[p + 0] = dest[p - 3] +
            (src[(w * 3 - 3)] - src[p + 0 - radius * 3])
                        / ((2 * radius) + 1.0f);

        dest[p + 1] = dest[p - 2] +
            (src[(w * 3 - 2)] - src[p + 1 - radius * 3])
                        / ((2 * radius) + 1.0f);

        dest[p + 2] = dest[p - 1] +
            (src[(w * 3 - 1)] - src[p + 2 - radius * 3])
                        / ((2 * radius) + 1.0f);
    }
}

/* Bluers a line along the y axis */
static void y_blur_line_grey(float *src, uint8_t *dest, float *line_buffer,
        size_t w, size_t rup, size_t rdw)
{
    /* Initializes the buffer with the original values */
    memcpy(line_buffer, src, w * sizeof(float));

    size_t n = 1;
    for (size_t r = 1; r <= rup; ++r, ++n) /* For each line above */
        for (int p = 0; (size_t) p < w; ++p)
            line_buffer[p] += src[-(int)(w * r) + p]; 

    for (size_t r = 1; r <= rdw; ++r, ++n) /* For each line below */
        for (int p = 0; (size_t) p < w; ++p) /* For each pixel of that line */
            line_buffer[p] += src[(int)(w * r) + p]; /* Add it to the buffer */

    /* Reads each accumulator, calculates the average and stores into dest */
    for (size_t p = 0; p < w; ++p)
        dest[p] = (uint8_t) clamp(line_buffer[p] / n, 0, 255);
}

/* Bluers a line along the y axis */
static void y_blur_line_color(float *src, uint8_t *dest, float *line_buffer,
        size_t w, size_t rup, size_t rdw)
{
    /* Initializes the buffer with the original values */
    memcpy(line_buffer, src, w * sizeof(float) * 3);

    size_t n = 1;
    for (size_t r = 1; r <= rup; ++r, ++n) /* For each line above */
        for (int p = 0; (size_t) p < w * 3; ++p) /* For each pixel of that line */
            line_buffer[p] += src[-(int)(w * r * 3) + p]; /* Add it to the buffer */

    for (size_t r = 1; r <= rdw; ++r, ++n) /* For each line below */
        for (int p = 0; (size_t) p < w * 3; ++p) /* For each pixel of that line */
            line_buffer[p] += src[(int) (w * r) * 3 + p]; /* Add it to the buffer */

    /* Reads each accumulator, calculates the average and stores into dest */
    for (size_t p = 0; p < w * 3; ++p)
        dest[p] = (uint8_t) clamp(line_buffer[p] / n, 0, 255);
}

/* Blurs a block of lines along the x aixis, form img into block. */
static void x_blur_block_color(PXM_Image *img, float *block,
        size_t line_offset, size_t block_height, size_t radius)
{
    #pragma omp parallel for schedule(static)
    for (size_t line = 0; line < block_height; ++line) {
        x_blur_line_color(((uint8_t*)img->pixels) +
                (line + line_offset) * img->w * 3,
                block + line * img->w * 3, img->w, radius);
    }
}

/* Blurs a block of lines along the x aixis, form img into block. */
static void x_blur_block_grey(PXM_Image *img, float *block,
        size_t line_offset, size_t block_height, size_t radius)
{
    #pragma omp parallel for schedule(static)
    for (size_t line = 0; line < block_height; ++line) {
        x_blur_line_grey(((uint8_t*)img->pixels) +
                (line + line_offset) * img->w * img->bpp,
                block + line * img->w, img->w, radius);
    }
}

/* Blurs a block of lines along the y axis without going over the limits of
 * the image. */
static void y_blur_block_color(PXM_Image *img, float *block,
        size_t line_offset, size_t block_height, size_t radius)
{
    #pragma omp parallel
    {
        float *line_buffer = (float*) malloc(img->w * sizeof(float) * 3);

        #pragma omp for schedule(static)
        for (size_t line = 0; line < block_height; ++line) {
            y_blur_line_color(block + line * img->w * 3,
                    ((uint8_t*)img->pixels) + (line + line_offset) * img->w * 3,
                    line_buffer, img->w, MIN(line + line_offset, radius),
                    MIN(img->len - line_offset - line - 1, radius));
        }

        free(line_buffer);
    }
}

/* Blurs a block of lines along the y axis without going over the limits of
 * the image. */
static void y_blur_block_grey(PXM_Image *img, float *block,
        size_t line_offset, size_t block_height, size_t radius)
{
    #pragma omp parallel
    {
        float *line_buffer = (float*) malloc(img->w * sizeof(float));

        #pragma omp for schedule(static)
        for (size_t line = 0; line < block_height; ++line) {
            y_blur_line_grey(block + line * img->w,
                    ((uint8_t*)img->pixels) + (line + line_offset) * img->w,
                    line_buffer, img->w, MIN(line + line_offset, radius),
                    MIN(img->len - line_offset - line - 1, radius));
        }

        free(line_buffer);
    }
}

static void blur_color(PXM_Image *img, size_t radius)
{
    float *buffer = (float*) malloc(img->w * PXM_WINDOW_SIZE * sizeof(float) * 3);
    FAIL(buffer,);

    size_t lines_to_process = img->len;
    if (lines_to_process <= PXM_WINDOW_SIZE) {
        DMSG("Whole image fits into window size. Blurring in a single step...");
        x_blur_block_color(img, buffer, 0, lines_to_process, radius);
        y_blur_block_color(img, buffer, 0, lines_to_process, radius);
        DMSG("Done.");
        free(buffer);
        return;
    } else {
        DMSG("Image won't fit into window size. Blurring in pieces.");
        DMSG("Starting to process from line 0 to %zu...", PXM_WINDOW_SIZE - radius);
        x_blur_block_color(img, buffer, 0, PXM_WINDOW_SIZE, radius);
        y_blur_block_color(img, buffer, 0, PXM_WINDOW_SIZE - radius, radius);
        DMSG("Done.");
    }

    size_t current_line = PXM_WINDOW_SIZE - radius;
    lines_to_process -= current_line;

    while(lines_to_process > PXM_WINDOW_SIZE) {
        size_t finished_lines = (PXM_WINDOW_SIZE - 2 * radius);
        DMSG("Starting to process from line %zu to %zu...",
                current_line, current_line + finished_lines);

        x_blur_block_color(img, buffer, current_line - radius, PXM_WINDOW_SIZE, radius);
        y_blur_block_color(img, buffer + radius * img->w * 3,
                current_line, finished_lines, radius);

        lines_to_process -= finished_lines;
        current_line += finished_lines;
        DMSG("Done.");
    }

    if (lines_to_process) {
        DMSG("Finishing to process remaining lines, from %zu to %zu...",
                current_line, current_line + lines_to_process);

        x_blur_block_color(img, buffer, current_line - radius,
                lines_to_process + radius, radius);
        y_blur_block_color(img, buffer + radius * img->w * 3,
                current_line, lines_to_process, radius);
        DMSG("Done.");
    }

    DMSG("Finished blurring image.");
    free(buffer);
}

static void blur_grey(PXM_Image *img, size_t radius)
{
    float *buffer = (float*) malloc(img->w * PXM_WINDOW_SIZE * sizeof(float));
    FAIL(buffer,);

    size_t lines_to_process = img->len;
    if (lines_to_process <= PXM_WINDOW_SIZE) {
        DMSG("Whole image fits into window size. Blurring in a single step...");
        x_blur_block_grey(img, buffer, 0, lines_to_process, radius);
        y_blur_block_grey(img, buffer, 0, lines_to_process, radius);
        DMSG("Done.");
        free(buffer);
        return;
    } else {
        DMSG("Image won't fit into window size. Blurring in pieces.");
        DMSG("Starting to process from line 0 to %zu...", PXM_WINDOW_SIZE - radius);
        x_blur_block_grey(img, buffer, 0, PXM_WINDOW_SIZE, radius);
        y_blur_block_grey(img, buffer, 0, PXM_WINDOW_SIZE - radius, radius);
        DMSG("Done.");
    }

    size_t current_line = PXM_WINDOW_SIZE - radius;
    lines_to_process -= current_line;

    while(lines_to_process > PXM_WINDOW_SIZE) {
        size_t finished_lines = (PXM_WINDOW_SIZE - 2 * radius);
        DMSG("Starting to process from line %zu to %zu...",
                current_line, current_line + finished_lines);

        x_blur_block_grey(img, buffer, current_line - radius, PXM_WINDOW_SIZE, radius);
        y_blur_block_grey(img, buffer + radius * img->w, current_line, finished_lines, radius);

        lines_to_process -= finished_lines;
        current_line += finished_lines;
        DMSG("Done.");
    }

    if (lines_to_process) {
        DMSG("Finishing to process remaining lines, from %zu to %zu...",
                current_line, current_line + lines_to_process);

        x_blur_block_grey(img, buffer, current_line - radius, lines_to_process+ radius, radius);
        y_blur_block_grey(img, buffer + radius * img->w, current_line, lines_to_process, radius);
        DMSG("Done.");
    }

    DMSG("Finished blurring image.");
    free(buffer);
}

void PXM_blur(PXM_Image *img, size_t radius)
{   
    DMSG("Bluring %zux%zu image with a window radius of %zu pixels.",
            img->w, img->h, radius);
    FAIL_MSG(img->max < 256,, "16 bit blur not implemented!");
    if(img->type == PXM_GREYSACALE)
        blur_grey(img, radius);
    else
        blur_color(img, radius);
}

