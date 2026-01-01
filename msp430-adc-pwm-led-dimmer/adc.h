#pragma once
#include <stdbool.h> 
void adc_init();
bool poll_adc_value(uint16_t *external_value);

