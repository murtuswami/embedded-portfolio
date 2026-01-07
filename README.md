# MSP430F5529 Bare-Metal Tick Scheduler (LPM0) + PWM + ADC12 + UART Command Console

Bare-metal firmware for the MSP430F5529 LaunchPad implemented with direct register access (no RTOS, no HAL, no driver library). The project implements a tick-driven cooperative scheduler, Timer_A hardware PWM, ADC12 continuous sampling with change-threshold publishing, a UART line-based command console, and LPM0 idle between ticks.

---


## Demos

Place media in `assets/` and reference them below.

### 1) Board / wiring photo
![Board wiring](assets/board_wiring_2.jpeg)


### 2) ADC → PWM → LED brightness (GIF)
![Potentiometer changes LED brightness](assets/pwm.gif)


### 3) Button short press behavior (GIF)
![Short press toggles P1.0](assets/short_press.gif)


### 4) Button long press behavior (GIF)
![Long press toggles P4.7](assets/long_press.gif)



---

## Features

- Cooperative tick scheduler with wraparound-safe timing checks
- Periodic tick from Timer1_A CCR0 on ACLK (default period: 5 ms)
- LPM0 idle between ticks; ticker ISR exits LPM0 on ISR return
- Hardware PWM on P1.2 using Timer0_A CCR1 output mode (reset/set)
- ADC12 continuous sampling on A0 with change-threshold publishing to reduce jitter
- UART command console with double-buffered line RX and a table-driven command dispatcher
- Button debounce and short/long press events driving onboard LEDs

---

## Hardware & Pinout

Board: MSP430F5529 LaunchPad

- PWM output: P1.2 (TA0.1)
- ADC input: P6.0 (A0)
- UART: P4.4 (TXD), P4.5 (RXD) via USCI_A1
- LEDs: P1.0, P4.7
- Button: P1.1 (pull-up enabled)

---

## Build & Flash

Toolchain: Code Composer Studio (CCS) or equivalent MSP430 toolchain.

UART serial settings:
- Baud: 115200
- Format: 8N1
- Flow control: none

---

## Usage

### Button
- Short press: toggle LED on P1.0
- Long press: toggle LED on P4.7

### PWM
- PWM output on P1.2.
- Duty cycle updated from ADC: `duty = adc_value / 4095.0`.

### UART commands
Examples:
```text
LED P1 ON
LED P1 OFF
LED P4 ON
LED P4 OFF
SET DUTY 0.50
```
---

### Control flow

```text
Timer1_A (ACLK) tick ISR
   -> sets tick_flag
   -> exits LPM0 on ISR return
Main loop:
   -> sleeps in LPM0
   -> wakes on tick
   -> scheduler_run(tasks, now)
   -> re-enters LPM0

```
---

###  Source layout 
```text
adc.c / adc.h              # ADC12 A0 continuous sampling + threshold publishing
button.c / button.h        # debounce + short/long press state machine
cmd_led.c / cmd_led.h      # LED command handlers (P1, P4)
cmd_set.c / cmd_set.h      # SET DUTY handler
cmd_log.c / cmd_log.h      # LOG handlers (ADC stub)
command.c / command.h      # tokenize + dispatch + routing table
led.c / led.h              # onboard LED helpers (P1.0, P4.7)
main.c                     # init + main loop (sleep/wake + scheduler)
pwm.c / pwm.h              # Timer0_A PWM on P1.2 (TA0.1)
scheduler.c / scheduler.h  # cooperative scheduler + wraparound-safe timing
tasks.c / tasks.h          # task implementations (ADC->PWM, button, UART RX)
ticker.c / ticker.h        # Timer1_A CCR0 periodic tick + LPM0 wake
uart.c / uart.h            # UART + double-buffered RX line input
```
---
### References 

- MSP430x5xx/6xx Family User’s Guide (Timer_A, ADC12, USCI UART, low-power modes)
- MSP430F5529 datasheet