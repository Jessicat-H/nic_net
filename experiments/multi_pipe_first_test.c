#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Opens pipe, blocks and asks for user input then writes
 */
int main(){
    // pipe file path
    char* pipe_path = "/tmp/test_pipe";
    // make the pipe with rw-rw-rw- permissions
    mkfifo(pipe_path, 0666);

    // Open pipe for write only
    int fd = open(pipe_path, O_WRONLY);

    char arr1[80];
    // Grab input from user, keeping file open until then;
    // potentially blocking access to pipe
    fgets(arr1, 80, stdin);

    // Write the input to the pipe and close
    write(fd, arr1, strlen(arr1)+1);
    close(fd);
    return 0;
}