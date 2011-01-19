#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "programmer.h"

int main(int argc, char **argv)
{
    unsigned char data;
    char buf[255];

    if (argc < 2)
    {
        printf("usage: programmer <serial device>\n");
        return -1;
    }

    printf("Opening serial port to %s\n", argv[1]);
    int fd = open_pic(argv[1]);

    // initialize communication with the PIC
    printf("Connecting to PIC serial...\n");
    write_pic(fd, CMD_INIT);
    while(read_pic(fd) != CMD_ACK);
    printf("done\n");

/*
    printf("Initializing NAND...\n");
    // put NAND chip into known state (reset)
    init_nand(fd);
    printf("done\n");

    printf("testind NAND read byte\n");
    data = read_byte_nand(fd, 0x0000);
    printf("read(%X) = %.2X\n", 0x0000, (unsigned int)data);
*/

    while(1) {
        write_addr(fd, 0x000000);
        sleep(1);
        write_addr(fd, 0x010101);
        sleep(1);
        write_addr(fd, 0x101010);
        sleep(1);
        write_addr(fd, 0x000001);
        sleep(1);
        write_addr(fd, 0x100000);
        sleep(1);
        write_addr(fd, 0x000011);
        sleep(1);
        write_addr(fd, 0x001100);
        sleep(1);
    }

/*
    printf("Reading NAND device info...");
    // verify nand device info
    verify_nand_info(fd);
    printf("done\n");
*/

    close_pic(fd);
    return 0;
}

