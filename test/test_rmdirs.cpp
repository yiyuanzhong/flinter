#include <stdio.h>
#include <stdlib.h>

#include <flinter/rmdirs.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "%s <pathname> [pathname]...\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i) {
        if (rmdirs(argv[i])) {
            perror("rmdirs");
            return EXIT_FAILURE;
        }

        printf("Removed %s\n", argv[i]);
    }

    return EXIT_SUCCESS;
}
