#include <string.h>

#include "pxm.h"
#include "blur.h"

int main(int argc, char *argv[])
{
    char name[256] = "blur-";
    PXM_Image *i = PXM_load_image(argv[1]);
    strcat(name, argv[1]);
    PXM_blur(i, 2);
    PXM_write_image(name, i);
    PXM_free_image(i);
    return 0;
}
