#ifndef UART_H
#define UART_H
/******************************************************************

* Note： RXD PD0

* TXD PD1

******************************************************************/
//Band Rate
#define BAUD 9600
#define MAX_UART_READ_BUF  7

char Read_UART;
char read_uart_buffer[MAX_UART_READ_BUF];

void init_uart(void);
void send_char(char data);
void send_string(char *str);
void uart_printf(char *fmt,...);
char read_char(void);
#endif
