# ADC Controlled PWM LED Dimmer

Timer driven LED brightness control on MSP430F5529 demonstrating:

- Potentiometer input via ADC12 on channel A0
- PWM brightness control of an LED using Timer A0
- UART debug output of raw ADC values
- Threshold based ADC change detection with low power sleep and wake

## Hardware

- MSP430F5529 LaunchPad
- LED on P1.2 (driven by Timer A0 CCR1)
- Potentiometer connected to A0 (P6.0, ADC12 channel A0)
- USB UART interface (USCI A1 on P4.4 and P4.5)

## Key Concepts

- Register level configuration of:
  - Timer A0 for PWM generation on CCR1
  - ADC12 in repeat single channel mode on A0
  - USCI A1 UART at 115200 baud
- Threshold based filtering of analog input:
  - Only treat changes larger than a set number of ADC counts as meaningful
- Simple interrupt to main communication:
  - ADC interrupt stores the latest value and a flag
  - Main loop polls once per update with `poll_adc_value`
- Low power operation using LPM0:
  - Main sleeps in LPM0 and is woken by the ADC interrupt when work is needed

## Module Overview

- `main.c`  
  Initializes peripherals (PWM, UART, ADC), enters LPM0, and on wake:
  - Retrieves new ADC values via `poll_adc_value`
  - Maps the 12 bit ADC value to a duty cycle
  - Updates the PWM duty cycle
  - Prints the raw ADC value over UART

- `pwm.c` / `pwm.h`  
  Configures Timer A0 to generate PWM on P1.2 and provides:
  - `pin_init` to set up the PWM output pin
  - `timer_init` to configure Timer A0
  - `set_duty_cycle` to set the PWM duty cycle as a float in the range 0.0 to 1.0

- `adc.c` / `adc.h`  
  Configures ADC12 on A0 and provides:
  - `adc_init` to start continuous sampling on channel A0
  - `poll_adc_value` to retrieve new samples once per significant change  
  The ADC interrupt:
  - Compares each sample to the last published value
  - Publishes only when the difference exceeds a configurable threshold
  - Requests exit from LPM0 so main can process the new value

- `uart.c` / `uart.h`  
  Configures USCI A1 for UART and provides:
  - `uart_init` for 115200 baud on P4.4 and P4.5 using SMCLK at 1 MHz
  - `uart_putc` to send a character
  - `uart_puts` to send a C string
  - `uart_put_uint16` to send a 16 bit integer in decimal

## Dependencies

This project uses TI provided device support files:

| File                  | Source                 | Purpose                      |
|-----------------------|------------------------|------------------------------|
| `msp430f5529.h`       | TI (included with CCS) | Register and bit definitions |
| `lnk_msp430f5529.cmd` | TI (included with CCS) | Default linker script        |

All peripheral configuration and application logic are written at the register level using the MSP430x5xx Family User Guide (SLAU208) as reference.