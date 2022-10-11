#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * tries to open the pipe and write to it immediately
 */
int main(){
    // pipe file path
    char* pipe_path = "/tmp/test_pipe";
    // make the pipe with rw-rw-rw- permissions
    mkfifo(pipe_path, 0666);

    // Open pipe for write only
    int fd = open(pipe_path, O_WRONLY);

    // write to the pipe and close
    char arr1[]="This is a test";
    write(fd, arr1, strlen(arr1));
    close(fd);

    return 0;
}