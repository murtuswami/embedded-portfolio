
/**
 * @file adc.c
 * @brief ADC12 setup on A0 with change‑threshold publishing and ISR wakeup.
 *
 * - Uses ADC12 on channel A0 (P6.0).
 * - Continuously samples in repeat‑single‑channel mode.
 * - When the raw value changes by more than ADC_CHANGE_THRESHHOLD from the last
 *   published value, the ISR:
 *     - stores the new sample,
 *     - marks it as needing publication,
 *     - and exits low‑power mode (LPM0) so main can process it.
 *
 * The `poll_adc_value` API lets main retrieve each “interesting” sample exactly
 * once.
 */

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief ADC change threshold in raw counts.
 *
 * A new sample is published only when
 * $|raw - last\_published| > ADC\_CHANGE\_THRESHHOLD$.
 *
 * For a 12‑bit ADC (range $0 \dots 4095$), 100 ≈ 2.4 % of full scale.
 */

#define ADC_CHANGE_THRESHHOLD 100

/**
 * @brief Publication state:
 *
 * - published == false  → there is a new value in publish_value that
 *                         main has not yet consumed.
 * - published == true   → no unread value is pending.
 *
 */


static volatile bool published = false; 
static volatile uint16_t publish_value = 0; 

/**
 * @brief Initialize ADC12 to continuously sample A0 (P6.0) with interrupts.
 *
 * Configuration:
 * - Input: A0 on P6.0 (analog).
 * - Mode: repeat single channel (ADC12CONSEQ_2).
 * - Clock: SMCLK, divider /1.
 * - Resolution: 12‑bit.
 * - Reference: AVCC/AVSS (board supply rails).
 * - Interrupt: ADC12MEM0 interrupt enabled.
 *
 * Conversions start immediately and run continuously after init.
 */
void adc_init() {

    /* Route P6.0 to ADC input A0 */
    P6SEL |= BIT0;

    /* Disable ADC to allow configuration changes */
    ADC12CTL0 &= ~ADC12ENC;

    /* Sample‑and‑hold and multiple sample mode */
    ADC12CTL0 &= ~ADC12MSC;           /* clear first: single conversion */
    ADC12CTL0 |= ADC12SHT0_6;         /* 128 ADC clocks sample time on SHT0 */
    ADC12CTL0 |= ADC12MSC;            /* enable repeated conversions */

    /* Use external AVCC/AVSS reference (on‑chip ref off) */
    ADC12CTL0 &= ~ADC12REFON;

    /* Start address: ADC12MEM0 (CSTARTADD = 0) */
    ADC12CTL1 &= ~(ADC12CSTARTADD0 | ADC12CSTARTADD1 |
                   ADC12CSTARTADD2 | ADC12CSTARTADD3);

    /* Trigger source: software (SHS = 0), sampling timer (SHP = 1) */
    ADC12CTL1 &= ~(ADC12SHS0 | ADC12SHS1); /* software trigger */
    ADC12CTL1 |= ADC12SHP;                 /* use sampling timer */

    /* ADC clock: SMCLK /1 */
    ADC12CTL1 &= ~(ADC12DIV0 | ADC12DIV1 | ADC12DIV2); /* divider /1 */
    ADC12CTL1 |= (ADC12SSEL0 | ADC12SSEL1);            /* SMCLK source */

    /* Conversion mode: repeat‑single‑channel */
    ADC12CTL1 |= ADC12CONSEQ_2;

    /* ADC12CTL2: 12‑bit resolution, no additional pre‑divider */
    ADC12CTL2 &= ~ADC12PDIV;      /* pre‑divider /1 */
    ADC12CTL2 |= ADC12RES_2;      /* 12‑bit resolution */

    /* Enable interrupt for ADC12MEM0 (corresponds to channel A0 here) */
    ADC12IE |= ADC12IE0;

    /* Memory control for ADC12MEM0:
     * - EOS: end of sequence at MEM0 (only one channel).
     * - SREF: AVCC/AVSS.
     * - INCH: A0.
     */
    ADC12MCTL0 |= ADC12EOS;   /* end of sequence at MEM0 */
    ADC12MCTL0 &= ~(ADC12SREF0 | ADC12SREF1 | ADC12SREF2); /* AVCC/AVSS */
    ADC12MCTL0 &= ~(ADC12INCH0 | ADC12INCH1 | ADC12INCH2 | ADC12INCH3); /* A0 */

    /* Turn ADC on, enable conversions, and start conversion loop */
    ADC12CTL0 |= ADC12ON;
    ADC12CTL0 |= ADC12ENC;
    ADC12CTL0 |= ADC12SC;


}

/**
 * @brief Try to retrieve the most recent “interesting” ADC value.
 *
 * @param[out] external_value  Pointer where the ADC value will be stored
 *                             if a new sample is available.
 *
 * @return true  if a new value was copied into *external_value and marked as
 *               published (consumed).
 * @return false if there is no unread value pending.
 *
 * Semantics :
 * - ISR sets `publish_value` and `published = false` when a new value should
 *   be processed by main.
 * - The first call to this function after that will:
 *     - copy `publish_value` to *external_value,
 *     - set `published = true`, and
 *     - return true.
 * - Subsequent calls will return false until the ISR publishes again.
 */
bool poll_adc_value(uint16_t *external_value){
    if(!published) {
         *external_value = publish_value;
        published = true;   /* mark value as consumed */
        return true; 
    }
    return false;
}

/**
 * @brief ADC12 interrupt service routine.
 *
 * Triggered when ADC12MEM0 has a new conversion result.
 *
 * Logic:
 * - Compute $|raw - last\_published|$.
 * - If the difference exceeds ADC_CHANGE_THRESHHOLD:
 *     - update last_published,
 *     - store raw in publish_value,
 *     - set published = false (main needs to consume),
 *     - request exit from LPM0 on ISR return.
 */
#pragma vector=ADC12_VECTOR
__interrupt void ADC12_interrupt(void) {
    if (ADC12IV == ADC12IV_ADC12IFG0) { 
        static uint16_t last_published = 0;
        uint16_t raw  = ADC12MEM0; 
        uint16_t diff = (raw > last_published) ? (raw - last_published) : (last_published - raw);
        if(diff >  ADC_CHANGE_THRESHHOLD ) {
            last_published = raw; /* update reference point */
            publish_value = raw;  /* value to be consumed by main */
            published = false; /* mark as needing publication */
            
            /* Wake main from LPM0 so it can process the new value */
            __bic_SR_register_on_exit(LPM0_bits);  
        }
        /* Else: ignore small jitter; do not update or wake main. */
    }
    /* No other ADC12IV cases are expected: only MEM0 interrupt is enabled. */

}



