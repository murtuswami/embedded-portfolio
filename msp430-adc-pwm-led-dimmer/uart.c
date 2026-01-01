/**
 * @file uart.c
 * @brief Simple UART driver for MSP430 using USCI_A1.
 *
 * Provides:
 * - `uart_init()`   : configure USCI_A1 for 115200 baud over the USB‑UART.
 * - `uart_putc()`   : transmit a single character.
 * - `uart_puts()`   : transmit a null‑terminated string.
 * - `uart_put_uint16()` : transmit a 16‑bit integer as decimal ASCII.
 *
 * USCI_A1 is connected to the USB interface on the MSP430 development board.
 */


#include <msp430.h>
#include "uart.h"
#include "string.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


#define UART_BUFFER_SIZE 64 

/* 
 * Double‑buffer storage for received data / command parsing.
 * - `isr_buffer` is filled by the UART RX ISR.
 * - `main_buffer` is used by main context for processing.
 *
 * Note: `isr_buffer`, `isr_index`, and `command_ready` are volatile because
 * they are written from interrupt context and read from main context.
 */
 
static char buffer_a[UART_BUFFER_SIZE];
static char buffer_b[UART_BUFFER_SIZE];

static  char * main_buffer = buffer_a;
static  char * volatile isr_buffer = buffer_b;

static volatile int isr_index = 0;
static volatile bool command_ready = false;

/**
 * @brief Initialize USCI_A1 for UART operation at 115200 baud using SMCLK.
 *
 * Configuration:
 * - Module: USCI_A1.
 * - Pins:  P4.4 (TXD), P4.5 (RXD) set to peripheral function.
 * - Clock: SMCLK at 1 MHz.
 * - Baud:  115200 baud (BR0 = 9, BR1 = 0, UCBRS_1).
 * - RX interrupt enabled.
 */
 
void uart_init(){
  UCA1CTL1 |= UCSWRST;              /* Put USCI in reset for configuration */
  UCA1CTL1 |= UCSSEL__SMCLK;        /* Use SMCLK as clock source */
  P4SEL |= BIT4 + BIT5;             /* Select peripheral function for P4.4, P4.5 */
  P4DIR |= BIT4;                    /* P4.4 (TX) as output */
  /* 1 MHz / 115200 baud settings (see device user’s guide) */
  UCA1BR0 = 9;                      /* Baud rate divider low byte */
  UCA1BR1 = 0;                      /* Baud rate divider high byte */
  UCA1MCTL = UCBRS_1 + UCBRF_0;     /* Modulation pattern */
  UCA1CTL1 &= ~UCSWRST;             /* Release USCI from reset */
}

/**
 * @brief Transmit a single character over UART.
 *
 * Blocks until the transmit buffer is ready, then writes the character.
 *
 * @param c Character to transmit.
 */
void uart_putc(char c){
  while (!(UCA1IFG&UCTXIFG));         /* wait for TX buffer to be ready */
  UCA1TXBUF = c;  
}
/**
 * @brief Transmit a null‑terminated string over UART.
 *
 * @param c Pointer to a C‑string to send. Must be null‑terminated.
 */
void uart_puts(const char* c) {
  while(*c != '\0'){ 
    uart_putc(*c);
    ++c;
  }

}
/**
 * @brief Transmit a 16‑bit unsigned integer in decimal format.
 *
 * Converts the value to ASCII using `snprintf` and sends it with `uart_puts`.
 *
 * @param v Unsigned 16‑bit value to print.
 */
void uart_put_uint16(const uint16_t v){
  char buf[6];
  snprintf(buf, sizeof buf, "%d", v);
  uart_puts(buf);

}


