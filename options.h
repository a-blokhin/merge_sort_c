#ifndef MERGE_SORT_C_OPTIONS_H
#define MERGE_SORT_C_OPTIONS_H

#include <stdio.h>

#define MODE_SORT 0
#define MODE_GENERATE 1
#define MODE_CHECK_SORTED 2
#define MODE_UNSPECIFIED 3

typedef struct {
    int mode;
    long io_buffer_size;
    long ram_size;
    long generate_size;
    char filename[FILENAME_MAX];
    char output_filename[FILENAME_MAX];
    char temp_directory_path[FILENAME_MAX];
} options_t;

#define OPTIONS_PARSE_SUCCESS 0
#define OPTIONS_PARSE_FAILED 1
#define OPTIONS_PARSE_SHOW_HELP 2

int parse_arguments(options_t *options, int argc, char **argv);

#endif //MERGE_SORT_C_OPTIONS_H
