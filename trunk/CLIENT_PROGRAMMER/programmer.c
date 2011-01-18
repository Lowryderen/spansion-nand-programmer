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
        printf("usage: programmer <ip address>\n");
        return -1;
    }

    printf("Opening serial port to %s\n", argv[1]);
    int fd = open_serial_net(argv[1], SERIAL_NET_PORT);

#ifdef DEBUG
    printf("Waiting for serial connection\n");
    while (read_serial_net(fd) == '.') {
        // write newline character to open connection
        write_serial_net(fd, '\n');
    }
    printf("Connected to serial network port\n");

    while (1) {
        gets_serial_net(fd, buf, 255);
        if (!strcmp(buf, "prompt>"))
        {
            printf("%s ", buf);
            fgets(buf, 255, stdin);
            strcat(buf, "\n");
            puts_serial_net(fd, buf, strlen(buf)-1);
        }
        else
        {
            printf("%s\n", buf);
        }
    }
#else
    // initialize communication with the PIC
    printf("Connecting to PIC serial...\n");
    write_serial_net(fd, CMD_INIT);
    while(read_serial_net(fd) != CMD_ACK);
    printf("done\n");

    printf("testind NAND read byte\n");
    data = read_byte_nand(fd, 0x0000);
    printf("read(%X) = %.2X\n", 0x0000, (unsigned int)data);

/*
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
*/
/*
    irintf("Initializing NAND...");
    // put NAND chip into known state (reset)
    init_nand(fd);
    printf("done\n");

    printf("Reading NAND device info...");
    // verify nand device info
    verify_nand_info(fd);
    printf("done\n");
*/

#endif

    close(fd);
    return 0;
}

