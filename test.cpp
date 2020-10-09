#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "display.h"

int main (int argc, char **argv) {
    if (dd_init() != 0) {
        printf("Error during init!\n");
    }

    const char *forward = "animation/forward.h264";
    const char *backward = "animation/backward.h264";
    const char *filename;
    int flip = 0;

    for (int i = 0; i < 100; i++) {
        if (flip) { filename = forward; flip = 0; }
        else { filename = backward; flip = 1; }

        FILE *in = fopen(filename, "rb");
        fseek(in, 0, SEEK_END);
        unsigned int fileLen = ftell(in);
        fseek(in, 0, SEEK_SET);
        unsigned char *buffer=(unsigned char *)malloc(fileLen+1);
        fread(buffer, fileLen, 1, in);
        fclose(in);

        if (dd_display(buffer, fileLen) != 0) {
            printf("Error during display!\n");
        }

        free(buffer);
    }

    dd_destroy();
}
