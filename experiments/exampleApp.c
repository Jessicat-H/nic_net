#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    // FIFO file path
    char * myFifoName = "/tmp/registerPipe";
    while (1) {
        int fd1 = open(myFifoName,O_WRONLY);
        write(fd1, "............", 12);
        close(fd1);
    }
}