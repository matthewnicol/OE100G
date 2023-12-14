#include <stdio.h>
#include "base/base.h"

int main(int argc, char *argv[]) {
    printf("Hello World!\n");
    NBASE_OSWindow_startup(600, 800);
    while (1) {
        NBASE_OSWindow_handleEvents();
        NBASE_OSWindow_swapBuffers();
    }
    printf("Exited successfully!\n");
    return 1;
}