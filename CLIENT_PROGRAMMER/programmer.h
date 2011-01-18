#ifndef _PROGRAMMER_INCLUDE_
#define _PROGRAMMER_INCLUDE_

//#define DEBUG

#define CMD_INIT 0x01
#define CMD_STATUS 0x02
// reset the NAND
#define CMD_RESET 0x04
#define CMD_MODE 0x08
#define CMD_RDY 0x10
#define CMD_ADDR 0xAD
#define CMD_DATAWR 0xDA
#define CMD_DATARD 0xDB
#define CMD_ACK 0xFE

// modes for CMD_MODE (note that modes must be OR'ed together to set the pin high)
#define CMD_MODE_WE 0x01
#define CMD_MODE_CE 0x02
#define CMD_MODE_OE 0x04

// in byte mode
#define UNLOCK_ADDR1 0x0AAA
#define UNLOCK_ADDR2 0x0555
#define UNLOCK_DATA1 0x00AA
#define UNLOCK_DATA2 0x0055
// set programming mode
#define PRGMSETUP_ADDR 0x0AAA
#define PRGMSETUP_DATA 0x00A0

#define PRGM_SETUPCMD 0x80
#define PRGM_ERASECMD 0x10
#define PRGM_ERASE_SECTOR_CMD 0x30

// 16 Mb
#define NAND_BYTE_MEMORY_SIZE 0x1000000

#define SERIAL_NET_PORT 2001

// sernetlib functions
int open_serial_net(char *hostname, int portno);
unsigned char read_serial_net(int fd);
void gets_serial_net(int fd, char *buf, int max_len);
void write_serial_net(int fd, unsigned char c);
void puts_serial_net(int fd, char *buf, int len);
void close_serial_net(int fd);

// nandlib functions
void write_data(int fd, unsigned char d);
unsigned char read_data(int fd);
void write_addr(int fd, unsigned int addr);
void set_mode(int fd, unsigned char mode);
unsigned char read_byte_nand(int fd, unsigned int addr);
void write_byte_nand(int fd, unsigned int addr, unsigned char d);
void wait_rdy_nand(int fd);
void unlock_nand(int fd);
void program_setup_nand(int fd);
void verify_nand_info(int fd);

#endif
