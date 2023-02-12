#include "sort.h"

#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <queue>
#include <utility>
#include <memory>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int get_file_size(const char *filename, long *out) {
    struct stat st;

    if (stat(filename, &st) != 0) {
        return 1;
    }

    *out = st.st_size;

    return 0;
}

int read_file_content(const char *filename, uint32_t *output, long io_buffer_size) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        char buffer[256];
        strerror_r(errno, buffer, sizeof(buffer));

        printf("cant open '%s' for reading: %s", filename, buffer);

        return 1;
    }

    uint32_t *dst = output;

    ssize_t r_t = 0UL;

    while (1) {
        ssize_t r = read(fd, dst, io_buffer_size * sizeof(uint32_t));

        if (r == -1) {
            char buffer[256];
            strerror_r(errno, buffer, sizeof(buffer));

            printf("read failed: %s (%d)", buffer, errno);

            return 1;
        }

        r_t += r;

        if (r == 0) {
            break;
        }

        if (r % sizeof(uint32_t) != 0) {
            printf("error: read bytes (%zu) is not aligned by sizeof(uint32_t)\n", r);
            close(fd);

            return 1;
        }

        dst += r / sizeof(uint32_t);
    }

    close(fd);

    return 0;
}

int write_content_to_file(const char *filename, uint32_t *content, long size, long write_count) {
    int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        char buffer[256];
        strerror_r(errno, buffer, sizeof(buffer));

        printf("cant open '%s' for writing: %s", filename, buffer);

        return 1;
    }

    long idx = 0;

    while (idx < size) {
        long to_write = size - idx > write_count ? write_count : size - idx;

        ssize_t written = write(fd, content, to_write * sizeof(uint32_t));
        if (written == -1) {
            char buffer[256];
            strerror_r(errno, buffer, sizeof(buffer));

            printf("write failed: %s", buffer);

            close(fd);

            return 1;
        }

        content += to_write;
        idx += to_write;
    }

    close(fd);

    return 0;
}

int cmp(const void *a, const void *b) {
    return *((uint32_t *) a) < *((uint32_t *) b);
}

int
sort_file_in_memory_internal(const char *filename, const char *output_filename, uint32_t *mem, long io_buffer_size) {
    long file_size;
    if (get_file_size(filename, &file_size) != 0) {
        char buffer[256];
        strerror_r(errno, buffer, sizeof(buffer));

        printf("cant stat '%s': %s", filename, buffer);

        return SORT_FAILED;
    }

    std::cout << "sorting " << filename << "..." << std::endl;

    long count = file_size / sizeof(uint32_t);

    if (read_file_content(filename, mem, io_buffer_size / sizeof(uint32_t)) != 0) {
        return SORT_FAILED;
    }

    qsort(mem, count, sizeof(uint32_t), cmp);

    if (write_content_to_file(output_filename, mem, count, io_buffer_size / sizeof(uint32_t)) != 0) {
        return SORT_FAILED;
    }

    return SORT_SUCCESS;
}

std::vector <std::filesystem::path> split_file(const char *filename,
                                               std::filesystem::path temp_directory_path,
                                               uint32_t *block,
                                               std::size_t part_size) {
    std::cout << "partitioning of file \'" << filename << "\'..." << std::endl;

    std::ifstream file(filename, std::ios::in | std::ios::binary);

    if (!file.is_open()) {
        perror("cant open file");

        return std::vector<std::filesystem::path>();
    }

    std::vector <std::filesystem::path> files;
    int file_num = 0;

    if (part_size % sizeof(uint32_t)) {
        part_size += sizeof(uint32_t) - part_size % sizeof(uint32_t);
    }

    while (true) {
        file.read(reinterpret_cast<char*>(block), part_size);
        auto read_len = file.gcount();
        if (read_len == 0) {
            break;
        }
        std::filesystem::path part_path = temp_directory_path / ("part" + std::to_string(file_num) + ".data");
        std::ofstream part(part_path, std::ios::out | std::ios::trunc | std::ios::binary);

        if (!part.is_open()) {
            std::cout << "cant create part file #" << file_num << std::endl;
            return std::vector<std::filesystem::path>();
        }

        part.write(reinterpret_cast<char *>(block), read_len);

        part.close();

        std::cout << "created file # " << file_num << std::endl;

        files.push_back(part_path);

        file_num++;
    }

    return files;
}


struct stream_reader {
    mutable std::ifstream stream;
    std::ifstream::pos_type end_of_file_pos;
    int io_buffer_size;
    mutable size_t current_buffer_size = 0;
    mutable size_t index = 0;
    mutable std::vector <uint32_t> buffer;

    stream_reader() = default;

    stream_reader(const stream_reader &) = delete;

    bool empty() const {
        return index >= current_buffer_size && stream.tellg() >= end_of_file_pos;
    }

    uint32_t read() {
        if (index >= current_buffer_size) {
            read_next_chunk();
        }

        if (empty()) {
            throw std::out_of_range("read from empty stream reader");
        }

        return buffer[index++];
    }

    uint32_t current() const {
        if (index >= current_buffer_size) {
            read_next_chunk();
        }

        if (empty()) {
            throw std::out_of_range("current on empty stream reader");
        }

        return buffer[index];
    }

private:
    void read_next_chunk() const {
        buffer.resize(io_buffer_size / sizeof(uint32_t));

        size_t read_bytes = stream.read(reinterpret_cast<char *>(buffer.data()), buffer.size()).gcount();

        current_buffer_size = read_bytes / sizeof(uint32_t);
        index = 0;
    }
};

using SP = std::shared_ptr<stream_reader>;

struct stream_reader_cmp {
    bool operator()(const SP &l, const SP &r) const {
        return l->current() < r->current();
    }
};

int
merge_files(const std::vector <std::filesystem::path> &files, const std::filesystem::path &out, long io_buffer_size) {
    std::priority_queue <SP, std::vector<SP>, stream_reader_cmp> readers_queue;

    std::cout << "merging..." << std::endl;

    std::ofstream out_stream(out, std::ios::trunc | std::ios::binary | std::ios::out);
    if (!out_stream) {
        std::cout << "cant open " << out << std::endl;
        return 1;
    }

    for (const auto &path: files) {
        std::ifstream stream(path, std::ios::binary | std::ios::in);
        if (!stream) {
            std::cout << "cant open " << path << std::endl;
            return 1;
        }

        auto s = std::make_shared<stream_reader>();

        s->stream = std::move(stream);
        s->io_buffer_size = io_buffer_size;
        s->index = 0;
        s->current_buffer_size = 0;
        s->buffer.resize(io_buffer_size / sizeof(uint32_t));
        s->stream.seekg(0, std::ios_base::end);
        s->end_of_file_pos = s->stream.tellg();
        s->stream.seekg(0);

        readers_queue.push(std::move(s));
    }

    std::vector <uint32_t> flush_buffer;
    flush_buffer.resize(io_buffer_size / sizeof(uint32_t));
    size_t flush_buffer_idx = 0;

    while (!readers_queue.empty()) {
        auto s = readers_queue.top();
        readers_queue.pop();

        uint32_t val = s->read();

        flush_buffer[flush_buffer_idx++] = val;

        if (flush_buffer_idx >= flush_buffer.size()) {
            out_stream.write(reinterpret_cast<const char *>(flush_buffer.data()), io_buffer_size);
            flush_buffer_idx = 0;
        }

        if (!s->empty()) {
            readers_queue.push(s);
        }
    }

    if (flush_buffer_idx != 0) {
        out_stream.write(reinterpret_cast<const char *>(flush_buffer.data()), flush_buffer_idx * sizeof(uint32_t));
    }

    return 0;
}

int sort(options_t *options) {
    std::vector <uint32_t> mem(options->ram_size / sizeof(uint32_t));

    std::vector <std::filesystem::path> parts = split_file(options->filename,
                                                           std::filesystem::path(options->temp_directory_path),
                                                           mem.data(),
                                                           options->ram_size);

    for (const std::filesystem::path &part_file: parts) {
        if (sort_file_in_memory_internal(part_file.c_str(), part_file.c_str(), mem.data(), options->io_buffer_size) !=
            0) {
            std::cout << "sorting of " << part_file << " failed" << std::endl;
            return SORT_FAILED;
        }
    }

    if (merge_files(parts, std::filesystem::path(options->output_filename), options->io_buffer_size) != 0) {
        std::cout << "merging failed" << std::endl;
        return SORT_FAILED;
    }

    std::cout << "cleanup..." << std::endl;
    for (const auto &path: parts) {
        std::filesystem::remove(path);
    }

    return SORT_SUCCESS;
}
