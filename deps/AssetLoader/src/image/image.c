#include "assets/image/image.h"
#include <stdlib.h>

struct image* image_blank(int width, int height, int channels) {
    struct image* i = malloc(sizeof(struct image));
    i->width = width;
    i->height = height;
    i->channels = channels;
    i->data = malloc(width * height * channels * sizeof(unsigned char));
    return i;
}

void image_delete(struct image* i) {
    free(i->data);
    free(i);
}
