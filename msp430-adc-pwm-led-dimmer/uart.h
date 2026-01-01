#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_putc(char);
void uart_puts(const char*);
void uart_put_uint16(const uint16_t);


#endif
