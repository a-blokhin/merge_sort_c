#include "generator.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

int generate_file(const char *filename, long size, long buffer_size) {
    uint32_t *io_buffer = (uint32_t *) malloc(buffer_size);
    if (io_buffer == NULL) {
        printf("cannot allocate i/o buffer\n");
        return GENERATE_FAILED;
    }

    int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        char buffer[256];
        strerror_r(errno, buffer, sizeof(buffer));
        printf("open '%s' failed: %s\n", filename, buffer);
        free(io_buffer);

        return GENERATE_FAILED;
    }

    int err = posix_fallocate(fd, 0, size);
    if (err != 0) {
        char buffer[256];
        strerror_r(err, buffer, sizeof(buffer));
        printf("posix_fallocate(%s, 0, %ld) failed: %s\n", filename, size, buffer);
        free(io_buffer);

        return GENERATE_FAILED;
    }

    long generated_size = 0;

    time_t begin_time = time(NULL);

    srand(begin_time);

    long last_percent = -1;

    printf("writing %ld bytes to '%s' with blocks of %ld...\n", size, filename, buffer_size);

    while (generated_size < size) {
        long to_write = (size - generated_size) > buffer_size ?  buffer_size : (size - generated_size);

        for (long i = 0; i < to_write / sizeof(int32_t); ++i) {
            io_buffer[i] = rand();
        }

        if (write(fd, io_buffer, to_write) == -1) {
            char buffer[256];
            strerror_r(errno, buffer, sizeof(buffer));
            printf("write failed: %s\n", buffer);

            free(io_buffer);
            close(fd);

            return GENERATE_FAILED;
        }

        generated_size += buffer_size;

        long percentage = generated_size * 100 / size;

        if (percentage != last_percent) {
            time_t current_time = time(NULL);
            printf("\r%ld%% (%.4f Mb/s)...     ",
                   percentage,
                   (float) generated_size / 1024.f / 1024.f / (float) (current_time - begin_time)
            );
            last_percent = percentage;
        }
    }

    time_t current_time = time(NULL);
    printf("\r100%% (%.4f Mb/s)                   \n", (float) generated_size / 1024.f / 1024.f / (float) (current_time - begin_time));

    close(fd);
    free(io_buffer);

    return GENERATE_SUCCESS;
}