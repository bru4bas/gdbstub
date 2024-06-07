
#pragma once
#include <stdint.h>

void uart_init(void);
void uart_putc(uint8_t c);
void uart_puts(char *s);
uint8_t uart_getc(void);

void uart_break_enable(void);
void uart_break_disable(void);
