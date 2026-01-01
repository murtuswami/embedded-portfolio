
#include "timer.h"
#include <msp430.h>


#define ACLK_FREQ_HZ 32768UL
#define TIMER_ID_DIV 1UL
#define TIMER_EX_DIV 1UL
#define TIMER_TOTAL_DIV (TIMER_ID_DIV * TIMER_EX_DIV)

//use 32bit math to avoid overflow then cast back to 16 bit 
#define TIMER_CCR0_FROM_MS(ms) ((uint16_t)( ( (uint32_t) (ACLK_FREQ_HZ) * (uint32_t) (ms)) / (1000UL * TIMER_TOTAL_DIV) - 1UL))
#define TIMER0_PERIOD_MS 5UL // 5ms

#define TIMER_CCR0_VALUE TIMER_CCR0_FROM_MS(TIMER0_PERIOD_MS)

static volatile bool tick_flag = false;

void timer_init() {
    TA0CTL |= TASSEL_1; //Set Tassel 1 to select ACLK 
    TA0CTL = (TA0CTL & ~ID_3) | ID_0;   //clear input divider bits for divide by 1 
    TA0EX0 = 0;                         // clear input divider expansion bits to divide by 1 
    TA0CTL |= TACLR;
    TA0CCTL0 |= CCIE; // enable interrupts on capture &control register 0 
    TA0CCR0 = TIMER_CCR0_VALUE; 
}
void timer_on() {
    TA0CTL |= MC__UP;// start timer 
}

bool consume_tick() { 
    if(tick_flag) { 
        tick_flag = false;
        return true;
    }
    return false;

}

//Dont need to clear interrupt flag because 
//The TAxCCR0 CCIFG flag is automatically reset when the TAxCCR0 interrupt
//request is serviced.
//Special case here. Dont assuma its automatically cleared for everything
// TA0CCTL0 &= ~CCIFG; // clear interrupt flag   
#pragma vector=TIMER0_A0_VECTOR
__interrupt void timerA0Elapsed() {
    tick_flag = true; 
}


