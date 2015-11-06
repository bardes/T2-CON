#include "pgm.h"
#include <string.h>

int main(int argc, char *argv[])
{
    char buff[256];
    PXM_Image *i = PXM_Load_image_section(argv[1], 5, 1);
    memset(i->pixels, 0x88, i->bpp * i->w * i->len);
    strcpy(buff, argv[1]);
    strcat(buff, ".new");
    PXM_write_image_section(buff, i);
    PXM_free_image(i);
    return 0;
}
