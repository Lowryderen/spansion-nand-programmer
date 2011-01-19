#include <stdio.h>
#include <unistd.h>
#include "programmer.h"

int open_pic(char *str)
{
    return open_serial_net(str, SERIAL_NET_PORT);
}

unsigned char read_pic(int fd)
{
    return read_serial_net(fd);
}

void write_pic(int fd, unsigned char data)
{
    write_serial_net(fd, data);
}

void close_pic(int fd)
{
    close_serial_net(fd);
}

// write data to the data bus on the PIC
void write_data(int fd, unsigned char d)
{
    write_pic(fd, CMD_DATAWR);
    write_pic(fd, d);
    while(read_pic(fd) != CMD_ACK);
}

// read data from the data bus on the pic
unsigned char read_data(int fd)
{
    write_pic(fd, CMD_DATARD);
    return read_pic(fd);
}

// write 24 bit address to array of latches connected to pic
void write_addr(int fd, unsigned int addr)
{
    write_pic(fd, CMD_ADDR);
    //while(read_pic(fd) != CMD_ACK);
    // write A22 - A16
    write_pic(fd, (unsigned char)((addr>>16)&0xFF));
    //while(read_pic(fd) != CMD_ACK);
    // write A15 - A8
    write_pic(fd, (unsigned char)((addr>>8)&0xFF));
    //while(read_pic(fd) != CMD_ACK);
    // write A7 - A0
    write_pic(fd, (unsigned char)(addr&0xFF));
    while(read_pic(fd) != CMD_ACK);
}

void set_mode(int fd, unsigned char mode)
{
    write_pic(fd, CMD_MODE);
    //while(read_pic(fd) != CMD_ACK);
    write_pic(fd, mode);
    while(read_pic(fd) != CMD_ACK);
}

void init_nand(int fd)
{
    set_mode(fd, CMD_MODE_WE); // WE=1, OE=0, CE=0
    write_addr(fd, 0x0000);
    write_pic(fd, CMD_RESET);
    while(read_pic(fd) != CMD_ACK);
}

unsigned char read_byte_nand(int fd, unsigned int addr)
{
    unsigned char d;
    set_mode(fd, CMD_MODE_WE); // WE=1, OE=0, CE=0
    write_addr(fd, addr);
    // latch addresses (on falling edge of CE)
    set_mode(fd, CMD_MODE_WE|CMD_MODE_CE); // WE=1, OE=0, CE=1
    usleep(100);
    set_mode(fd, CMD_MODE_WE); // WE=1, OE=0, CE=0
    // falling edge of OE triggers data output from nand on bus
    set_mode(fd, CMD_MODE_WE|CMD_MODE_OE); // WE=1, OE=1, CE=0
    usleep(100);
    set_mode(fd, CMD_MODE_WE); // WE=1, OE=0, CE=0

    d = read_data(fd);
}

void write_byte_nand(int fd, unsigned int addr, unsigned char d)
{
    set_mode(fd, CMD_MODE_WE); // WE=1, OE=0, CE=0
    write_addr(fd, addr);
    // latch addresses (on falling edge of CE)
    set_mode(fd, CMD_MODE_WE|CMD_MODE_CE); // WE=1, OE=0, CE=1
    usleep(100);
    set_mode(fd, CMD_MODE_WE); // WE=1, OE=0, CE=0
    write_data(fd, d);
    // falling edge of WE triggers data write
    set_mode(fd, CMD_MODE_WE); // WE=1, OE=1, CE=0
    usleep(100);
    set_mode(fd, 0x00); // WE=0, OE=0, CE=0
}

// wait until NAND is in a ready state
void wait_rdy_nand(int fd)
{
    do
    {
        write_pic(fd, CMD_RDY);
    }
    while(!read_pic(fd));
}

void unlock_nand(int fd)
{
    write_byte_nand(fd, UNLOCK_ADDR1, UNLOCK_DATA1);
    write_byte_nand(fd, UNLOCK_ADDR2, UNLOCK_DATA2);
}

void program_setup_nand(int fd)
{
    write_byte_nand(fd, PRGMSETUP_ADDR, PRGMSETUP_DATA);
}

void program_byte_nand(int fd, unsigned int addr, unsigned char d)
{
    unlock_nand(fd);
    program_setup_nand(fd);
    write_byte_nand(fd, addr, d);
    wait_rdy_nand(fd);
}

void dump_nand_memory(int fd)
{
    unsigned char data;
    unsigned int addr;

    for(addr=0; addr<NAND_BYTE_MEMORY_SIZE; addr++)
    {
        data = read_byte_nand(fd, addr);        
        printf("read(%X) = %.2X\n", addr, (unsigned int)data);
        sleep(1);
    }
}

// completely erase nand device
void erase_nand_device(int fd)
{
    unlock_nand(fd);
    write_byte_nand(fd, PRGMSETUP_ADDR, PRGM_SETUPCMD);
    unlock_nand(fd);
    write_byte_nand(fd, PRGMSETUP_ADDR, PRGM_ERASECMD);
    printf("Erasing nand chip...\n");
    wait_rdy_nand(fd);
    printf("Erase complete!\n");
}

// erase a nand sector
void erase_nand_sector(int fd, unsigned int sector)
{
    unlock_nand(fd);
    write_byte_nand(fd, PRGMSETUP_ADDR, PRGM_SETUPCMD);
    unlock_nand(fd);
    write_byte_nand(fd, sector, PRGM_ERASE_SECTOR_CMD);
    printf("Erasing nand sector %X...\n", sector);
    wait_rdy_nand(fd);
    printf("Erase complete!\n");
}

void verify_nand_info(int fd)
{
    unsigned char data;
    unlock_nand(fd);
    // set autoselect mode
    write_byte_nand(fd, 0x0AAA, 0x90);
    // verify Manufacturer ID
    data = read_byte_nand(fd, 0x0000);
    if (data != 0x01)
    {
        printf("Error expected data 0x01 at address 0x0000, instead 0x%.2X\n", data);
        return;
    }
    printf("Verified Manufacturer ID\n");

    // read device ID byte 1
    data = read_byte_nand(fd, 0x0001);
    if (data != 0x7E)
    {
        printf("Error expected data 0x7E at address 0x0001, instead 0x%.2X\n", data);
        return;
    }

    // read device ID byte 2
    data = read_byte_nand(fd, 0x000E);
    if (data != 0x21) // for GL128P
    {
        printf("Error expected data 0x21 at address 0x000E, instead 0x%.2X\n", data);
        return;
    }

    // read device id byte 3
    data = read_byte_nand(fd, 0x000F);
    if (data != 0x01)
    {
        printf("Error expected data 0x01 at address 0x000F, instead 0x%.2X\n", data);
        return;
    }
    printf("Verified Device ID\n");

    // read Secure Device Verify
    //data = read_byte_nand(fd, 0x0003);
    // read Sector Protect Verify

    // clear autoselect mode
    write_byte_nand(fd, 0x0000, 0xF0);
}

