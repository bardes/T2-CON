#include <stdio.h>
#include <string.h>

#include "pxm.h"
#include "blur.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    PXM_Image *i = PXM_load_image(argv[1]);

    PXM_blur(i, (size_t) atoi(argv[3]));
    PXM_write_image(argv[2], i);

    PXM_free_image(i);
    return 0;
}
