#include "pxm.h"

#include "utils.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

static size_t read_file_until(FILE *stream, char **dest,
                            size_t *len, const char* sep)
{
    size_t contagem = 1;

    /* Guardando o char lido num int, pois EOF "nao cabe" num char... */
    int charLido;

    /* Testando a validade dos argumentos passados */
    if(!*dest || *len < 2)
        return 0;

    /* Pulando eventuais separadores iniciais */
    while(!feof(stream))
        if(!strchr(sep, (*dest)[0] = fgetc(stream)))
            break;

    /* Lendo os caracters um de cada vez ate chegar num separador */
    while(!strchr(sep, (charLido = fgetc(stream))) && !feof(stream))
    {
        /* "len - 1" garante que sempre havera espaco para o '\0' */
        if(contagem >= *len - 1)
        {
            /* Tenta alocar mais... */
            char *tmp = (char*) realloc(*dest, (*len) *= 2);

            /* Se falhar para de ler e retorna a contagem */
            if(!tmp)
                return contagem;
            *dest = tmp;
        }
        /* Escrevendo o char lido na string */
        (*dest)[contagem++] = charLido;
    }

    /* Sem esquecer o '\0' */
    (*dest)[contagem] = '\0';

    return contagem;
}

static int is_empty(const char* str)
{
    for(const char *c = str; *c; ++c) if(!isspace(*c)) return 0;
    return 1;
}

/**
 * Returns the nex non comment line
 */
static char *next_line(FILE *f)
{
    size_t buff_len = 4096;
    char *line = (char*) malloc(buff_len);
    FAIL(line, NULL);
    do read_file_until(f, &line, &buff_len, "\n");
    while(line[0] == '#' || is_empty(line));
    return line;
}

int PXM_read_header(FILE *f, PXM_Image* dest)
{
    /* Reads the magic numbrer */
    DMSG("Reading header from file.");
    char *line = next_line(f);
    FAIL(line, -1);

    if(!strcmp(line, "P5")) {
        DMSG("PGM file header detected.");
        dest->type = PXM_GRAYSACALE;
    } else if(!strcmp(line, "P6")) {
        DMSG("PPM file header detected.");
        dest->type = PXM_COLOR;
    } else {
        FAIL_MSG(0, -1, "Unknown magic number: %s", line);
    }

    /* Reads the dimensions */
    line = next_line(f);
    FAIL(line, -1);
    FAIL_MSG(sscanf(line, "%zu %zu", &(dest->w), &(dest->h)) == 2, -1,
        "Failed to read the image dimensions!");
    DMSG("Image dimensions are: %zux%zu.", dest->w, dest->h);

    /* Reads the maximum channel value */
    line = next_line(f);
    FAIL(line, -1);
    FAIL_MSG(sscanf(line, "%zu", &(dest->max)) == 1, -1,
        "Failed to read maximum cannel value!");
    FAIL_MSG(dest->max <= 0xffff, 0,
             "Maximum channel value too big (%zu) must be at most 65535.",
             dest->max);
    DMSG("Maximum channel value is: %zu.", dest->max);

    DMSG("Done reading header.");
    return 0;
}

PXM_Image* PXM_load_image(const char* path)
{
    return PXM_Load_image_section(path, 0, 0);
}

int PXM_write_image(const char* path, const PXM_Image* img)
{
    DMSG("Writing image to file: %s", path);
    if(img->len != img->h)
        DMSG("[Warinig] Writing partial image as whole file!");

    FILE *f = fopen(path, "w+");
    FAIL(f, -1);

    fprintf(f, "%s\n", img->type == PXM_COLOR ? "P6" : "P5");
    fprintf(f, "%zu %zu\n%zu\n", img->w, img->len, img->max);
    fwrite(img->pixels, img->bpp, img->w * img->len, f);

    fclose(f);
    return 0;
}

int PXM_write_image_section(const char* path, const PXM_Image* i)
{
    FILE *f = fopen(path, "r+");
    FAIL(f, -1);

    /* Skips the header */
    next_line(f);
    next_line(f);
    next_line(f);

    fseek(f, (int)(i->offset * i->w * i->bpp), SEEK_CUR);
    fwrite(i->pixels, i->bpp, i->len * i->w, f);

    fclose(f);
    return 0;
}

PXM_Image* PXM_Load_image_section(const char* path, size_t offset, size_t lines)
{
    DMSG("Loading image from: %s.", path);
    PXM_Image *img = (PXM_Image*) malloc(sizeof(PXM_Image));
    FAIL(img, NULL);

    FILE *img_file = fopen(path, "r");
    if(!img_file) {
        free(img);
        FAIL(0, NULL);
    }

    PXM_read_header(img_file, img);

    img->offset = offset;
    img->len = lines ? lines : img->h;
    img->bpp = (img->type == PXM_COLOR ? 3U : 1U) * (img->max > 0xff ? 2U : 1U);
    img->pixels = malloc(img->bpp * img->w * img->len);
    if(!img->pixels) {
        free(img);
        FAIL(0, NULL);
    }

    /*TODO check failure*/
    DMSG("Reading pixel data.");
    fseek(img_file, (int)(img->w * img->offset * img->bpp), SEEK_CUR);
    int p = fread(img->pixels, img->bpp, img->w * img->len, img_file);
    fclose(img_file);
    DMSG("%d pixels read, whith %zu bytes each.", p, img->bpp);

    DMSG("Finished reading image.");
    return img;

}

void PXM_free_image(PXM_Image* img)
{
    if(img) {
        free(img->pixels);
        free(img);
    }
}
