#ifndef __PGM_H__
#define __PGM_H__

#include <stdint.h>
#include <stdio.h>

typedef enum {
    PXM_GRAYSACALE,
    PXM_COLOR
} PXM_ImgType;

typedef struct {
    void *pixels;           /* Pixel data */
    size_t w, h;            /* Dimensions of the image */
    size_t offset, len;     /* Used to load only part of the data */
    size_t max;             /* Maximum channel value (usually 255) */
    size_t bpp;             /* Bytes per pixel */
    PXM_ImgType type;       /* Image type (grayscale/color)*/
} PXM_Image;

/**
 * Reads the header from f and sores on dest.
 *
 * Returns 0 on success.
 */
int PXM_read_header(FILE *f, PXM_Image* dest);

/**
 * Loads an image file based on it's extension. Or null if failed.
 */
PXM_Image *PXM_load_image(const char *path);

/**
 * Writes the given image to the path.
 */
int PXM_write_image(const char *path, const PXM_Image *img);

/**
 * Loads only a section limited by the total number of lines and an offset.
 */
PXM_Image *PXM_Load_image_section(const char *path, size_t offset,
                                  size_t lines);

/**
 * Writes a section into an existing image.
 */
int PXM_write_image_section(const char *path, const PXM_Image *i);

/**
 * Releases the memory used by the given image.
 */
void PXM_free_image(PXM_Image *img);

#endif //__PGM_H__
