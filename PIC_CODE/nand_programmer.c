//#include "pic18f2685.h"
#include "pic16f876a.h"
// PIC #1: controls address lines A0 - A22

#include <string.h>

//#define DEBUG
//#include "stdio.h"
//#include "usart.h"

// Configurations
/*
code char at __CONFIG1H conf1 = _OSC_IRCIO67_1H;
code char at __CONFIG2L conf2 = _PWRT_ON_2L;
code char at __CONFIG2H conf3 = _WDT_OFF_2H;                    // Disable WDT
code char at __CONFIG4L conf4 = _LVP_OFF_4L;                    // Disable LVP
*/
/* Setup chip configuration */
typedef unsigned int config;
config at 0x2007 __CONFIG = _CP_OFF & 
 _WDT_OFF & 
 _BODEN_OFF & 
 _PWRTE_OFF & 
 _HS_OSC & 
 _LVP_OFF;

// shift register pins
#define PIN_SER RC3
//PORTA_bits.RA0
#define PIN_CLK RC5
//PORTA_bits.RA1
#define PIN_RCLK RC4
//PORTA_bits.RA2

#define PIN_WE RA3
//PORTA_bits.RA2
#define PIN_OE RA4
//PORTA_bits.RA3
#define PIN_CE RA5
//PORTA_bits.RA5
#define PIN_BSY RC0
//PORTA_bits.RA4
#define PIN_RST RC1
//PORTA_bits.RA6
#define PIN_TST RC2

// 9600 baud rate with 20 mhz clock
//#define BAUD_9600_20 129
// 9600 baud rate with 16MHz clock
#define BAUD_9600 25
// 19200 baud rate with 16MHz clock
#define BAUD_19200 12

// output
#define PORTA_CONFIG 0x00
#define PORTB_CONFIG 0x00
#define PORTC_CONFIG 0x81
// initial values
#define PORTA_INIT 0x00
#define PORTB_INIT 0x00
#define PORTC_INIT 0x00

#define WRITE_BIT(x,n,v) (x=v?(x|1<<(n)):(x&~(1<<(n))))
#define SET_BIT(x,n) ((x)|=1<<(n))
#define CLR_BIT(x,n) ((x)&=~(1<<(n)))
#define GET_BIT(x,n) (x&(1<<n))

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

/*
#ifndef TXIF
#define TXIF                 PIR1bits.TXIF
#endif

#ifndef RCIF
#define RCIF                 PIR1bits.RCIF
#endif

#define SERIAL_MODE (USART_TX_INT_ON & USART_RX_INT_ON & \
USART_BRGH_HIGH & USART_ASYNCH_MODE & USART_EIGHT_BIT)
#define SERIAL_BAUD 10

typedef union {
        struct {
                unsigned s0         : 1;
                unsigned s1         : 1;
                unsigned s2         : 1;
                unsigned s3         : 1;
                unsigned dmode      : 1;
                unsigned            : 1;
                unsigned            : 1;
                unsigned            : 1;
        };
} __ADDR_STATE;
volatile __ADDR_STATE addr_state;

//unsigned char rx_serial_data;
unsigned char read_data;

// general interrupt handler
//static void isr(void) interrupt 0x08 {

static void isr(void) interrupt 0 {
    if (RCIF) {
        RCIE = 0;
        //rx_serial_data = RCREG;

        if (addr_state.s0)
        {
            // drive OE and CE to VIL
            PIN_OE = 0;
            PIN_CE = 0;

            addr_state.s0 = 0;
            addr_state.s1 = 0;

            // verify write command
            if (RCREG == CMD_ADDR)
            {
                addr_state.s1 = 1;
                addr_state.dmode = 0;
            }
            else if (RCREG == CMD_DATA)
            {
                addr_state.s1 = 1;
                addr_state.dmode = 1;
            }
            else if (RCREG == CMD_READ)
            {
                // send data read from NAND back over serial
                TXREG = PORTB;
                //TXIF = 1;
            }
            else if (RCREG == CMD_STATUS)
            {
            }
            else if (RCREG == CMD_TEST)
            {
                PIN_TST2 = PIN_TST2 ^ 1;
            }
            else if (RCREG == CMD_TESTSER)
            {
                // for testing serial I/O is working
                //TXEN = 1;
                TXREG = 0xBA;
                //TXIF = 1;
            }
            else
            {
                // invalid command
            }
        } else if (addr_state.s1)
        {
            if (addr_state.dmode)
            {
                // write D0 - D7 to PORTB
                addr_state.s0 = 1;
                PORTB = RCREG;
                // Data is output from the falling edge of OE
                PIN_OE = 1;
            }
            else
            { 
                // write A0 - A7 to shift register
                addr_state.s2 = 1;
                //PORTB = RCREG;
                write_register(RCREG);
            }

            addr_state.s1 = 0;

        } else if (addr_state.s2)
        {
            // write A8 - A15 to shift register
            addr_state.s2 = 0;
            addr_state.s3 = 1;
            //PORTB = RCREG;
            write_register(RCREG);
        } else if (addr_state.s3)
        {
            // write A16 - A22,A-1 to shift register
            //PORTC = RCREG;
            write_register(RCREG);

            // All addresses are latched on the falling edge of CE
            PIN_CE = 1;
            addr_state.s3 = 0;
            addr_state.s0 = 1;
        }
        RCIF = 0;
        RCIE = 1;
    }
    else if (TXIF) {
        TXIE = 0;
        TXREG = read_data;
        TXIE = 1;
        TXIF = 0;
    }
}
*/

void delay_ms(int ms)
{
    int i;

    while (ms--)
        for (i=0; i < 330; i++)
            ;
}

#define SHORT_DELAY { unsigned char i; for (i=0; i < 64; i++); }

void write_register(unsigned char dat)
{
    char j;

    PIN_CLK = 1; 

    SHORT_DELAY;

    for(j=7; j>=0; j--)
    {
        PIN_CLK = 0;
        //delay_ms(1);
        SHORT_DELAY;
        if (j == 0) {
            PIN_SER = dat&0x01;
        } else {
            PIN_SER = (dat>>j)&0x01;
        }
        //delay_ms(1);
        SHORT_DELAY;
        PIN_CLK = 1;
        //delay_ms(1);
        SHORT_DELAY;
    }
}

unsigned char read_val;

/*
void timer_intr() interrupt 0 {
  if(TXIF) { // serial interrupt
    // Most interrupts must be cleared in software:
    // TXIF = 0; // not doable :)
    //TXREG = 'n';
  } else if (RCIF) {
    read_val = RCREG;
    RCIF = 0;
  } 
}
*/

// enable serial tx/rx on RC6/RC7
void init_serial(void)
{
    //usart_open(SERIAL_MODE, SERIAL_BAUD);
    // below baud configuration assumes 16Mhz clock
    BRGH = 0;
    SPBRG = BAUD_19200;
    // SYNC - USART Mode select Bit
    SYNC = 0; // (0 = asynchronous)
    SPEN = 1; // (1 = serial port enabled)
    // enable serial tx/rc interrupt
    RCIE = 0; // (1 = enabled)
    TXIE = 0; // (1 = enabled)
    TX9 = 0; // (0 = 8-bit transmit)
    RX9 = 0; // (0 = 8-bit reception)
    TXEN = 1;  // enable transmit
    CREN = 1; // enable receive

    GIE = 0;  // (1 = Enable all unmasked interrupts)
    PEIE = 0; // (1 = Enable all unmasked peripheral)
}

/*
unsigned char memcmp(char *str1, const char str2[], unsigned char len)
{
    while(len-- > 0) {
        if (str1[len] != str2[len]) return 1;
    }
    return 0;
}
*/

#ifdef DEBUG
void puts(const char *str)
{
    unsigned char i = 0;
    while (str[i] != '\0')
    {
        while(!TXIF);
        TXREG = str[i];
        i++;
    } 
}

unsigned char compare_data(char *str1, char *str2, unsigned char len)
{
    unsigned char i = 0;

    for(i = 0; i < len; i++)
    {
        if (str1[i] != str2[i]) return 1;
    }
    return 0;
}

unsigned char convert_hex2bin(char h)
{
    if (h >= '0' && h <= '9')
    {
        return (h-'0');
    }
    else if (h >= 'A' && h <= 'F')
    {
        return (h-'A'+0x0A);
    }
    else if (h >= 'a' && h <= 'f')
    {
        return (h-'a'+0x0A);
    }

    return 0;
}

char convert_bin2hex(unsigned char h)
{
    if (h <= 0x09)
    {
        return (h+'0');
    }
    else if (h >= 0x0A && h <= 0x0F)
    {
        return (h+-0x0A+'A');
    }

    return 0;
}

void gets_cmd()
{
    unsigned char i = 0;
    char buf[14];
    const char cmd1[] =  "addr";
    const char cmd2[] =  "write";
    const char cmd3[] = "pin";
    const char cmd4[] = "read";
    const char cmd5[] = "tris";
    //const char cmd6[] = "mode";
    const char cmd6[] = "shift";

    while (1)
    {
        while (!RCIF);
        buf[i] = RCREG;
        if (buf[i] == '\n') {
            buf[i] = '\0';
            break;
        }
        i++;
    }

    if (!compare_data(buf, cmd1, 4))
    {
        // write A22 - A16
        unsigned char addr = convert_hex2bin(buf[9])<<4;
        addr |= convert_hex2bin(buf[10]);
        write_register(addr);

        // A15 - A8
        addr = convert_hex2bin(buf[7])<<4;
        addr |= convert_hex2bin(buf[8]);
        write_register(addr);

        // A7 - A0
        addr = convert_hex2bin(buf[5])<<4;
        addr |= convert_hex2bin(buf[6]);
        write_register(addr);

        puts(buf+5);
    }
    else if (!compare_data(buf, cmd2, 4))
    {
        unsigned char dat = convert_hex2bin(buf[5])<<4;
        dat |= convert_hex2bin(buf[6]);
        // write to data bus
	TRISB = 0x00;
        PORTB = dat;
        puts(buf+5);
    }
    else if (!compare_data(buf, cmd3, 3))
    {
        if (buf[3] == 'A')
        {
            WRITE_BIT(PORTA, (buf[4] - '0'), buf[5]-'0');
        }
        else if (buf[3] == 'B')
        {
            WRITE_BIT(PORTB, (buf[4] - '0'), buf[5]-'0');
        }
        else if (buf[3] == 'C')
        {
            WRITE_BIT(PORTC, (buf[4] - '0'), buf[5]-'0');
        }
    }
    else if (!compare_data(buf, cmd4, 4))
    {
        unsigned char dat;
        TRISB = 0xFF;
        dat = PORTB;
        buf[5] = convert_bin2hex(buf[5]>>4);
        buf[6] = convert_bin2hex(buf[6]&0x0F);
        buf[7] = '\0';
        puts(buf+5);
    }
    else if (!compare_data(buf, cmd5, 4))
    {
        if (buf[3] == 'A')
        {
            WRITE_BIT(TRISA, (buf[4] - '0'), buf[5]-'0');
        }
        else if (buf[3] == 'B')
        {
            WRITE_BIT(TRISB, (buf[4] - '0'), buf[5]-'0');
        }
        else if (buf[3] == 'C')
        {
            WRITE_BIT(TRISC, (buf[4] - '0'), buf[5]-'0');
        }
    }
    else if (!compare_data(buf, cmd6, 5))
    {
        // test shift register
        unsigned char addr = convert_hex2bin(buf[5])<<4;
        addr |= convert_hex2bin(buf[6]);
        write_register(addr);

        // test converting back
        buf[5] = convert_bin2hex(addr>>4);
        buf[6] = convert_bin2hex(addr&0x0F);
        buf[7] = '\0';
        delay_ms(50);
        puts(buf);
    }

    //else if (!compare_data(buf, cmd6, 4))
    //{
        // set data read/write mode
    //}

    delay_ms(100);
}
#endif

void write_tx(unsigned char d)
{
    while(!TXIF);
    TXREG = d;
}

unsigned char read_rx()
{
    while(!RCIF);
    return RCREG;
}

void main(void) {
    unsigned char recvdat = 0;
    TRISA = PORTA_CONFIG;
    TRISB = PORTB_CONFIG;
    TRISC = PORTC_CONFIG;
    PORTA = PORTA_INIT;
    PORTB = PORTB_INIT;
    PORTC = PORTC_INIT;

    init_serial();

    PIN_TST = 1;

    while(1) {
#ifdef DEBUG
            if (!recvdat)
            {
                if (RCIF)
                {
                    read_val = RCREG;
                    if (read_val == '\n' || read_val == ' ')
                    {
                        recvdat = 1;
                        PIN_TST = 0;
                    }
                }
                else
                {
                    while(!TXIF);
                    TXREG = '.';
                    delay_ms(200);
                    PIN_TST = 0;
                    delay_ms(200);
                    PIN_TST = 1;
                }
            }
            else
            {
                puts("prompt> ");
                gets_cmd();
            }
#else
        if (RCIF)
        {
            // actual code
            read_val = RCREG;
            //read_val = read_rx();
            switch(read_val) {
                case CMD_INIT:
                    PIN_TST = 0;
                    write_tx(CMD_ACK);
                    break;
                case CMD_STATUS:
                    write_tx(CMD_ACK);
                    break;
                case CMD_RESET:
                    PIN_RST = 0;
                    delay_ms(1);
                    PIN_RST = 1;
                    write_tx(CMD_ACK);
                    break;
                case CMD_MODE:
                    read_val = read_rx(); 
                    PIN_WE = (read_val & CMD_MODE_WE)&0x1;
                    PIN_CE = ((read_val & CMD_MODE_CE)>>1)&0x1;
                    PIN_OE = ((read_val & CMD_MODE_OE)>>2)&0x1;
                    write_tx(CMD_ACK);
                    break;
                case CMD_RDY:
                    write_tx(!PIN_BSY);
                    break;
                case CMD_ADDR:
                    PIN_RCLK = 0;
                    read_val = read_rx();
                    write_register(read_val);
                    read_val = read_rx();
                    write_register(read_val);
                    read_val = read_rx();
                    write_register(read_val);
                    SHORT_DELAY;
                    PIN_RCLK = 1;
                    write_tx(CMD_ACK);
                    break;
                case CMD_DATAWR:
                    TRISB = 0x00;
                    PORTB = read_rx();
                    write_tx(CMD_ACK);
                    break;
                case CMD_DATARD:
                    TRISB = 0xFF;
                    write_tx(PORTB);
                    break;
                }
            }
#endif
        }
}

