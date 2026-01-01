/**
 * @file pwm.c
 * @brief PWM output on P1.2 using Timer_A0 CCR1 (ACLK as source).
 *
 * - Uses TA0.1 (CCR1) configured in reset/set mode (OUTMOD_7).
 * - Output pin: P1.2 configured for peripheral (TA0.1) output.
 * - Timer clock: ACLK ( 32768 Hz).
 * - Period: TA0CCR0 = 320 → PWM frequency ≈ 32768 / 320 ≈ 102 Hz.
 */
 
#include <msp430.h>
#include <stdint.h>
#include <pwm.h>
#define TIMER_CCR0_VALUE ((uint16_t)320)


/**
 * @brief Configure P1.2 as Timer_A0 CCR1 output.
 *
 * P1.2 is set as output and connected to the TA0.1 peripheral function.
 */
void pin_init(void)
{
    P1DIR |= BIT2;  /* P1.2 as output */
    P1SEL |= BIT2;  /* P1.2 function select: TA0.1 (Timer_A CCR1 output) */
}


/**
 * @brief Initialize Timer_A0 for PWM on CCR1 using ACLK.
 *
 * - Clock source: ACLK (TASSEL_1).
 * - Input divider: /1 (ID_0, TA0EX0 = 0).
 * - Mode: Up mode (MC__UP).
 * - CCR0 sets the PWM period.
 * - CCR1 controls the duty cycle (reset/set mode, OUTMOD_7).
 *
 * Interrupts are disabled; the timer runs autonomously to generate PWM.
 */
void timer_init(void)
{
    /* Select ACLK as timer clock source, clear input divider to /1 */
    TA0CTL = TASSEL_1 | MC_0;          /* ACLK, stop mode for configuration */
    TA0CTL = (TA0CTL & ~ID_3) | ID_0;  /* Input divider /1 */
    TA0EX0 = 0;                        /* Extended divider /1 */

    TA0CTL |= TACLR;                   /* Clear timer to start from 0 */

    TA0CCR0 = TIMER_CCR0_VALUE;        /* Set PWM period */

    TA0CTL &= ~TAIE;                   /* Disable Timer_A overflow interrupt */

    TA0CCTL1 &= ~CAP;                  /* Compare mode (not capture) */
    TA0CCTL1 = (TA0CCTL1 & ~OUTMOD_7) | OUTMOD_7; /* Reset/Set PWM mode */

    TA0CCTL1 &= ~CCIE;                 /* Disable CCR1 interrupt */

    TA0CCR1 = 0;                       /* Start with 0% duty cycle */

    TA0CTL |= MC__UP;                  /* Start timer in Up mode */
}

/**
 * @brief Set PWM duty cycle on P1.2.
 *
 * @param duty_cycle Fraction between 0.0 (0%) and 1.0 (100%).
 *
 * Values outside [0.0, 1.0] are clamped.
 */
void set_duty_cycle(const float duty_cycle)
{
    float d = duty_cycle;

    if (d < 0.0f) d = 0.0f;
    if (d > 1.0f) d = 1.0f;

    TA0CCR1 = (uint16_t)(TIMER_CCR0_VALUE * d);
}            



