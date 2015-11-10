#include <stdio.h>
#include <string.h>

#include "pxm.h"
#include "blur.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    PXM_Image *i = PXM_load_image(argv[1]);
    PXM_blur(i, 2);
    
    char name[256];
    strcpy(name, argv[1]);
    char *p = strrchr(name, '.');
    strcpy(p, ".blur.pgm");
    PXM_write_image(name, i);

    PXM_free_image(i);
    return 0;
}
