/**
 * @file main.c
 * @brief MSP430 demo: PWM LED brightness controlled by potentiometer via ADC.
 *
 * Overview:
 * - PWM output on P1.2 (Timer_A0 CCR1, ACLK) drives an LED with variable brightness.
 * - A potentiometer on A0 (P6.0 → ADC12 A0) is sampled continuously.
 * - The ADC ISR publishes a new value only when the change exceeds a threshold,
 *   and wakes the CPU from LPM0.
 * - The main loop:
 *     - sleeps in LPM0,
 *     - wakes when the ADC ISR signals a new value,
 *     - maps the 12‑bit ADC value (0–4095) to a 0.0–1.0 duty cycle,
 *     - updates the PWM,
 *     - prints the raw ADC value over UART.
 */


#include <msp430.h> 
#include <pwm.h>
#include <stdint.h>
#include <uart.h>
#include <adc.h>


/**
 * @brief Application entry point.
 *
 * Initialization sequence:
 * - Stops the watchdog.
 * - Initializes:
 *     - PWM output (pin + Timer_A0),
 *     - UART for debug output,
 *     - ADC sampling on A0 with interrupt + threshold logic.
 * - Enables global interrupts.
 *
 * Runtime behavior:
 * - Main loop:
 *     - Enters LPM0 with interrupts enabled.
 *     - Wakes when the ADC ISR calls `__bic_SR_register_on_exit(LPM0_bits)`.
 *     - Uses `poll_adc_value` to retrieve a new ADC sample if available.
 *     - Converts the 12‑bit raw value to a fractional duty cycle
 *       $d = \frac{\text{adc\_value}}{4095}$.
 *     - Updates the PWM duty cycle and prints the ADC value.
 */

void main (void)
{
    /* Stop the watchdog timer */
    WDTCTL = WDTPW | WDTHOLD;   
    pin_init();                 /* Configure P1.2 as Timer_A PWM output */
    timer_init();               /* Configure Timer_A0 for PWM on CCR1 */
    set_duty_cycle(0.9f);       /* Start with 90% duty cycle (LED mostly on) */
    uart_init();                 /* Initialize UART for debug prints */
    adc_init();                  /* Initialize ADC on A0 with ISR + threshold */
    uart_puts("Initialized\n");
    __bis_SR_register(GIE);

    
    while(1) { 
        /* Enter LPM0; CPU sleeps until an interrupt (e.g. ADC) wakes it */
        _bis_SR_register(LPM0_bits | GIE);
        uint16_t adc_value; 
        /* Check if ADC module has published a new value */

        if(poll_adc_value(&adc_value)){
            /* Map 12‑bit value (0..4095) to duty cycle fraction (0.0..1.0) */
            float duty_cycle = (float) adc_value / 4095.0f ; 
            set_duty_cycle(duty_cycle);

            uart_put_uint16(adc_value);
            uart_puts("\n");
        }
    }
    

}
