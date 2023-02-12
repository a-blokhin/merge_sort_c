#include "check.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

int check_sorted(const char *filename, long io_buffer_size) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open file failed");

        return CHECK_FAILED;
    }

    uint32_t *io_buffer = reinterpret_cast<uint32_t*>(malloc(io_buffer_size));
    if (io_buffer == NULL) {
        printf("cant allocate i/o buffer\n");
        close(fd);

        return CHECK_FAILED;
    }

    memset(io_buffer, 0, io_buffer_size);

    long pos = 0;

    while (1) {
        ssize_t r = read(fd, io_buffer, io_buffer_size);

        if (r % sizeof(uint32_t) != 0) {
            printf("panic: non fully 4 bytes integer read\n");
            close(fd);
            free(io_buffer);

            return CHECK_FAILED;
        }

        if (r == 0) {
            break;
        }

        ++pos;

        for (long i = 1; i < r / sizeof(int32_t); ++i) {
            if (io_buffer[i] > io_buffer[i - 1]) {
                printf("not sorted: number %u at %ld is greater than %u at %ld\n", io_buffer[i], pos, io_buffer[i - 1], pos - 1);

                close(fd);
                free(io_buffer);

                return CHECK_RESULT_NOT_SORTED;
            }

            ++pos;
        }
    }

    free(io_buffer);
    close(fd);

    printf("file '%s' is sorted\n", filename);

    return CHECK_RESULT_SORTED;
}