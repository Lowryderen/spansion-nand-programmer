#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define BAUD B19200
#define DATABITS CS8
#define STOPBITS 0
// parity none
#define PARITYON 0
#define PARITY 0

int open_serial(char *devname)
{
    struct termios sset;
    int fd = open(devname, O_RDWR | O_NOCTTY | O_NONBLOCK);
    sset.c_cflag = BAUD | CRTSCTS | DATABITS | STOPBITS | PARITYON | PARITY | CLOCAL | CREAD;
    sset.c_iflag = IGNPAR;
    sset.c_oflag = 0;
    sset.c_lflag = 0;       //ICANON;
    sset.c_cc[VMIN]=1;
    sset.c_cc[VTIME]=0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&sset);

    return fd;
}

char read_serial(int fd)
{
    char data;

    if (read(fd, &data, 1) < 1) printf("Error occurred during serial read\n");

    return data;
}

void write_serial(int fd, char c)
{
    if (write(fd, &c, 1) < 1) printf("Error occurred during serial write\n");
}

void close_serial(int fd)
{
    close(fd);
}

