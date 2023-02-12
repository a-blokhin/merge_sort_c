#include "options.h"

#include <stdlib.h>
#include <string.h>

#include <unistd.h>

int parse_size_number(const char *s, long *count) {
    char *suffix_pointer = NULL;
    *count = strtoll(s, &suffix_pointer, 10);

    switch (*suffix_pointer) {
        case 'b':
            *count *= 1UL;
            break;
        case 'k':
            *count *= 1024UL;
            break;
        case 'm':
            *count *= 1024UL * 1024UL;
            break;
        case 'g':
            *count *= 1024UL * 1024UL * 1024UL;
            break;
        case 0:
            break;
        default:
            return 1;
    }

    return 0;
}

int parse_arguments(options_t *options, int argc, char **argv) {
    memset(options->filename, 0, FILENAME_MAX);
    memset(options->temp_directory_path, 0, FILENAME_MAX);
    memset(options->output_filename, 0, FILENAME_MAX);

    options->io_buffer_size = 4 * 1024;
    options->ram_size = 1 * 1024 * 1024 * 1024;

    options->mode = MODE_UNSPECIFIED;

    int opt;

    while ((opt = getopt(argc, argv, "f:t:r:g:b:o:mhcs")) != -1) {
        switch (opt) {
            case 'c':
                if (options->mode != MODE_UNSPECIFIED) {
                    printf("only one mode can be selected\n");
                    return OPTIONS_PARSE_FAILED;
                }
                options->mode = MODE_CHECK_SORTED;
                break;

            case 's':
                if (options->mode != MODE_UNSPECIFIED) {
                    printf("only one mode can be selected\n");
                    return OPTIONS_PARSE_FAILED;
                }
                options->mode = MODE_SORT;
                break;

            case 'g':
                if (options->mode != MODE_UNSPECIFIED) {
                    printf("only one mode can be selected\n");
                    return OPTIONS_PARSE_FAILED;
                }
                options->mode = MODE_GENERATE;

                if (parse_size_number(optarg, &options->generate_size) != 0) {
                    printf("cant parse generate size\n");
                    return OPTIONS_PARSE_FAILED;
                }
                break;

            case 'f':
                if (strlen(optarg) >= FILENAME_MAX) {
                    printf("too long filename\n");
                    return OPTIONS_PARSE_FAILED;
                }
                strcpy(options->filename, optarg);
                break;

            case 't':
                if (strlen(optarg) >= FILENAME_MAX) {
                    printf("too long temp directory path\n");
                    return OPTIONS_PARSE_FAILED;
                }
                strcpy(options->temp_directory_path, optarg);
                break;

            case 'o':
                if (strlen(optarg) >= FILENAME_MAX) {
                    printf("output filename too log\n");
                    return OPTIONS_PARSE_FAILED;
                }
                strcpy(options->output_filename, optarg);
                break;

            case 'r':
                if (parse_size_number(optarg, &options->ram_size) != 0) {
                    printf("cant parse ram usage quota\n");
                    return OPTIONS_PARSE_FAILED;
                }
                break;

            case 'b':
                if (parse_size_number(optarg, &options->io_buffer_size) != 0) {
                    printf("cant parse i/o buffer size\n");
                    return OPTIONS_PARSE_FAILED;
                }
                break;

            case 'h':
                printf("external memory merge sort utility\n"
                       "usage: %s -f <filename> -g <size>|-c|-s|-m -r <size>\n"
                       "use following options:\n"
                       " -h        : see this help and exit\n"
                       " -f <path> : specify file for sorting, checking ang generating\n"
                       " -c        : check if specified file sorted\n"
                       " -g <size> : generate file with given <size>\n"
                       " -s        : sort specified file with external memory if needed\n"
                       " -r <size> : ram size quota (default 1Gb)\n"
                       " -o <path> : output file for generation and sorting\n"
                       " -t <path> : temp directory path for sorting\n"
                       " -b <size> : i/o buffer size (not strict, default 4Kb)\n"
                       "\n"
                       "all size values support suffixes b|k|m|g\n",
                       argv[0]);
                return OPTIONS_PARSE_SHOW_HELP;

            case '?':
            default:
                return OPTIONS_PARSE_FAILED;
        }
    }

    if (options->mode == MODE_UNSPECIFIED) {
        printf("mode not selected. see -h for more info\n");
        return OPTIONS_PARSE_FAILED;
    }

    if (options->mode == MODE_GENERATE) {
        if (strlen(options->output_filename) == 0) {
            printf("expected -o file path, see -h for more\n");
            return OPTIONS_PARSE_FAILED;
        }
    }

    if (options->mode == MODE_CHECK_SORTED) {
        if (strlen(options->filename) == 0) {
            printf("expected -f file path, see -h for more\n");
            return OPTIONS_PARSE_FAILED;
        }
    }

    if (options->mode == MODE_SORT) {
        if (strlen(options->filename) == 0) {
            printf("expected -f file path to sort, see -h for more\n");
            return OPTIONS_PARSE_FAILED;
        }

        if (strlen(options->output_filename) == 0) {
            printf("expected -o output file path, see -h for more\n");
            return OPTIONS_PARSE_FAILED;
        }

        if (strlen(options->temp_directory_path) == 0) {
            printf("expected -t directory path, see -h for more\n");
            return OPTIONS_PARSE_FAILED;
        }
    }

    return OPTIONS_PARSE_SUCCESS;
}
