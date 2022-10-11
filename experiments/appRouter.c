#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
    int fd1;

    // FIFO file path
    char * myFifoName = "/tmp/registerPipe";

    mkfifo(myFifoName, 0666);

    char msg[255];
    while (1) {
        printf("loop iteration\n");
        fd1 = open(myFifoName,O_RDONLY);
        int bytesRead = read(fd1, msg, 12);
        close(fd1);
        printf("Read %d bytes from reg pipe:\n%s\n",bytesRead,msg);
    }

}