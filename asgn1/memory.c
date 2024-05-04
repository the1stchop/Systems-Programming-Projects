#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <limits.h>

// reads to_read number of bytes from fd into buffer. Returns number of bytes read. If error, return -1.
ssize_t read_bytes(int fd, char *buffer, ssize_t to_read) {
    ssize_t bytes_read = 0;
    ssize_t curr_read = read(fd, buffer, to_read);
    while (to_read != 0 && curr_read > 0) {
        bytes_read += curr_read;
        to_read -= curr_read;
        curr_read = read(fd, buffer + bytes_read, to_read);
    }
    if (curr_read == -1) {
        return -1;
    }
    return bytes_read;
}

// writes to_write number of bytes from buffer to fd. Returns number of bytes written. If error, return -1.
ssize_t write_bytes(int fd, char *buffer, ssize_t to_write) {
    ssize_t bytes_written = 0;
    ssize_t curr_written = write(fd, buffer, to_write);
    while (to_write != 0 && curr_written > 0) {
        bytes_written += curr_written;
        to_write -= curr_written;
        curr_written = write(fd, buffer + bytes_written, to_write);
    }
    if (curr_written == -1) {
        return -1;
    }
    return bytes_written;
}

// get command, takes open file, reads it to buffer, then writes to stdout
void get(int fd) {
    // create buffer to write to
    char file_buff[4096];

    while (1) {
        ssize_t read = read_bytes(fd, file_buff, 4096);
        if (read == -1) {
            fprintf(stderr, "Invalid Command\n");
            exit(EXIT_FAILURE);
        }
        ssize_t write = write_bytes(STDOUT_FILENO, file_buff, read);
        if (write == -1) {
            fprintf(stderr, "Invalid Command\n");
            exit(EXIT_FAILURE);
        }

        if (read <= 0) {
            break;
        }
    }
}

void set(int fd, unsigned long long content_length) {
    // create buffer to write to
    char file_buff[4096];
    unsigned long long content_read = 0;
    ssize_t read = 1;
    unsigned long long size = sizeof(file_buff);
    if (content_length < sizeof(file_buff)) {
        size = content_length;
    }

    while (content_length > content_read && read > 0) {
        read = read_bytes(STDIN_FILENO, file_buff, size);
        if (read == -1) {
            fprintf(stderr, "Invalid Command pa\n");
            exit(EXIT_FAILURE);
        }
        content_read += read;
        ssize_t write = write_bytes(fd, file_buff, read);
        if (write == -1) {
            fprintf(stderr, "Invalid Command ma\n");
            exit(EXIT_FAILURE);
        }
    }
}

int main(void) {
    //read command (get or set) from stdin
    //create 4 byte buffer for command
    char cmd_buff[4];

    // checks for error in reading
    if (read_bytes(STDIN_FILENO, cmd_buff, 4) == -1) {
        fprintf(stderr, "invalid input: trouble reading input.\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(cmd_buff, "get\n") == 0) {
        // allocating mem for file name
        char get_buff[PATH_MAX] = "";

        // checks for error in reading
        int read = read_bytes(STDIN_FILENO, get_buff, PATH_MAX);
        if (read == -1) {
            fprintf(stderr, "Invalid Command\n");
            exit(EXIT_FAILURE);
        }

        // using regex to match valid file name
        static const char *const re = "^([a-zA-Z0-9_.]+)\n$";
        regex_t regex;
        regmatch_t pmatch[2];

        // compiling regex
        if (regcomp(&regex, re, REG_NEWLINE | REG_EXTENDED)) {
            fprintf(stderr, "you fucked up\n");
            exit(EXIT_FAILURE);
        }

        // if there are no matches, meaning shit broke
        if (regexec(&regex, get_buff, 2, pmatch, 0)) {
            fprintf(stderr, "Invalid Command\n");
            regfree(&regex);
            exit(EXIT_FAILURE);
        }

        char file_buff[PATH_MAX] = "";
        memcpy(file_buff, get_buff, pmatch[1].rm_eo - pmatch[1].rm_so);

        // attempt to open file
        int fd = open(file_buff, O_RDONLY, 0);
        if (fd == -1) {
            fprintf(stderr, "Invalid Command\n");
            exit(EXIT_FAILURE);
        }

        // call get()
        get(fd);

        // freeing regex and file
        regfree(&regex);
        close(fd);
        return 0;
    } else if (strcmp(cmd_buff, "set\n") == 0) {
        // using regex to match valid file name
        static const char *const re = "^([a-zA-Z0-9_.]+)\n$";
        regex_t regex;
        regmatch_t pmatch[2];

        // compiling regex
        if (regcomp(&regex, re, REG_NEWLINE | REG_EXTENDED)) {
            fprintf(stderr, "you fucked up\n");
            exit(EXIT_FAILURE);
        }

        // buffer for file_name
        // read in byte by byte
        char file_buff[PATH_MAX] = "";
        ssize_t total_read = 0;
        while (1) {
            ssize_t read = read_bytes(STDIN_FILENO, file_buff + total_read, 1);
            if (read == -1) {
                fprintf(stderr, "Invalid Command: couldnt read bytes\n");
                exit(EXIT_FAILURE);
            }
            total_read += 1;
            if (read <= 0 || regexec(&regex, file_buff, 2, pmatch, 0) == 0) {
                break;
            }
        }

        // check if match
        if (regexec(&regex, file_buff, 2, pmatch, 0)) {
            fprintf(stderr, "Invalid Command: no match\n");
            regfree(&regex);
            exit(EXIT_FAILURE);
        }

        // freeing regex
        regfree(&regex);

        file_buff[strlen(file_buff) - 1] = '\0';
        // attempt to open file
        int fd = open(file_buff, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            fprintf(stderr, "Invalid Command: couldnt open file\n");
            exit(EXIT_FAILURE);
        }

        // using regex to get content_length
        static const char *const re1 = "^([0-9]+)\n$";

        // compiling regex
        if (regcomp(&regex, re1, REG_NEWLINE | REG_EXTENDED)) {
            fprintf(stderr, "you fucked up\n");
            exit(EXIT_FAILURE);
        }

        // buffer for content_length
        // read in byte by byte
        char content_length[36] = "";
        total_read = 0;
        while (1) {
            ssize_t read = read_bytes(STDIN_FILENO, content_length + total_read, 1);
            if (read == -1) {
                fprintf(stderr, "Invalid Command: couldnt read bytes\n");
                exit(EXIT_FAILURE);
            }
            total_read += read;
            if (read <= 0 || regexec(&regex, content_length, 2, pmatch, 0) == 0
                || total_read == 36) {
                break;
            }
        }

        // check if match
        if (regexec(&regex, content_length, 2, pmatch, 0)) {
            fprintf(stderr, "Invalid Command: no matched\n");
            regfree(&regex);
            exit(EXIT_FAILURE);
        }

        // change to integer
        unsigned long long length = strtoull(content_length, NULL, 10);

        //free regex
        regfree(&regex);

        // read rest of stdin and place into file calling set
        set(fd, length);

        close(fd);

        printf("OK\n");
        return 0;
    } else {
        fprintf(stderr, "Invalid Command\n");
        exit(EXIT_FAILURE);
    }
}
