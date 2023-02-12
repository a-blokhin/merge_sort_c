#ifndef MERGE_SORT_C_CHECK_H
#define MERGE_SORT_C_CHECK_H

#define CHECK_RESULT_SORTED 0
#define CHECK_RESULT_NOT_SORTED 1
#define CHECK_FAILED 2

int check_sorted(const char *filename, long io_buffer_size);

#endif //MERGE_SORT_C_CHECK_H
