#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Opens a pipe and constantly reads it
 */ 
int main(){
    // pipe file path
    char* pipe_path = "/tmp/test_pipe";
    // make the pipe with rw-rw-rw- permissions
    mkfifo(pipe_path, 0666);

    // Open pipe for write only
    int fd = open(pipe_path, O_RDONLY);

    char strng[80];
    // Open in read only
    int fd1 = open(pipe_path,O_RDONLY);
    while (1)
    {
        // read and print
        read(fd1, strng, 80);
        printf("Read: %s\n", strng);
    }
}