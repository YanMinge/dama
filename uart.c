//Crystal frequency
#ifndef F_CPU 
#define F_CPU 8000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "uart.h"

char Read_UART = 0;
/*****************************************************************
** Name：void init_uart(void)

** Function：atmega328p UART initialization

** Entrance parameters: None

** Outlet parameters: None

** Instructions for use: send & read, interrupt reception, 8 data bits, 1 stop bit, 57600 baud
*****************************************************************/

void init_uart(void)
{
  UCSRB = 0x00;
  UCSRA = 0x00;
  UCSRC = 3 << UCSZ0;
  //Configure the baud rate
  UBRRH = (F_CPU / BAUD / 16 - 1) / 256;
  UBRRL = (F_CPU / BAUD / 16 - 1) % 256;

  //Enabled receive interrupt, receiver, transmit
  UCSRB |= (1 << RXCIE) | (1 << RXEN) | (1 << TXEN);

  DDRD |= 0x02;
}
/*****************************************************************
** Name：void send_Char(char data)

** Function：atmega328p UART send character

** Entrance parameters：the character to be sent

** Outlet parameters：None

** Instructions for use：Send single character

*****************************************************************/
void send_char(char data)

{
  while(!(UCSRA & _BV(UDRE)));
  UDR = data;
} 

/************************************************************
** Name：void send_string(char *str)

** Function：atmega328p UART send string

** Entrance parameters：the point of string to be sent

** Outlet parameters：None

** Instructions for use：Send string
************************************************************/
void send_string(char *str)
{
  while(*str)
  {
    send_char(*str++);
  }
}

/************************************************************
** Name：void uart_printf(char *str)

** Function：atmega328p UART send string with parameters

** Entrance parameters：the point of string to be sent

** Outlet parameters：None

** Instructions for use：Send string
************************************************************/
void uart_printf(char *fmt,...)
{
    va_list ap;
    unsigned char string[256];

    va_start(ap,fmt);
    vsprintf(string,fmt,ap);
    send_string(string);
    va_end(ap);
}

/*****************************************************************
** Name：void read_char(char data)

** Function：atmega328p UART read character

** Entrance parameters：None

** Outlet parameters：the character read from RX

** Instructions for use：Read single character

*****************************************************************/
char read_char(void)
{
  while(!(UCSRA & (1<<RXC)) );
  Read_UART = UDR;
  send_char(Read_UART);
  return Read_UART;
}

