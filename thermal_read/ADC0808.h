#ifndef _ADC0808_
#define _ADC0808_

#include "lcd.h"

enum adc_config {
	ADC0808_A = 20,
	ADC0808_B,
	ADC0808_C,
	ADC0808_ALE,
	ADC0808_START,
	ADC0808_EOC,
	ADC0808_OE,
	ADC0808_CLK
};

void adc_gpio_set_output(unsigned int *base_addr);
unsigned char read_byte(unsigned int *base_addr);
void temp_show(unsigned int *base_addr, unsigned char temp);
unsigned char ADC0808_Read(unsigned int *base_addr, unsigned char channel);
#endif
