#include <iostream>

#include <cstdio>

#include <unistd.h>

#include "sort.h"
#include "check.h"
#include "generator.h"

long align_by_page_size(long size, long page_size) {
    return size + size % page_size;
}

void align_by_page_size_inplace(long *value, long page_size, const char *name) {
    long aligned = align_by_page_size(*value, page_size);

    if (*value != aligned) {
        printf("%s aligned by page size (%ld) from %ld to %ld\n", name, page_size, *value, aligned);
        *value = aligned;
    }
}

int main(int argc, char **argv) {
    std::cout << "pid = " << getpid() << std::endl;

    options_t options;
    int parse_status = parse_arguments(&options, argc, argv);
    if (parse_status == OPTIONS_PARSE_SHOW_HELP) {
        return 0;
    }

    if (parse_status == OPTIONS_PARSE_FAILED) {
        return 1;
    }

    if (options.io_buffer_size % sizeof(uint32_t) != 0) {
        options.io_buffer_size += (sizeof(uint32_t) - options.io_buffer_size % sizeof(uint32_t));
    }

    if (options.ram_size % sizeof(uint32_t) != 0) {
        options.ram_size += (sizeof(uint32_t) - options.ram_size % sizeof(uint32_t));
    }

    if (options.mode == MODE_GENERATE) {
        if (generate_file(options.output_filename, options.generate_size, options.io_buffer_size) != GENERATE_FAILED) {
            return 1;
        }
    }

    if (options.mode == MODE_CHECK_SORTED) {
        if (check_sorted(options.filename, options.io_buffer_size) == CHECK_FAILED) {
            printf("check failed\n");
            return 1;
        }
    }

    if (options.mode == MODE_SORT) {
        if (sort(&options) != SORT_SUCCESS) {
            printf("sort failed\n");
            return 1;
        }
    }

    return 0;
}
