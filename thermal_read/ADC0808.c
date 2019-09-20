#include "ADC0808.h"

/*
ADC0808_A = gpio20
ADC0808_B = gpio21
ADC0808_C = gpio22
ADC0808_ALE = gpio23
ADC0808_START = gpio24
ADC0808_EOC = gpio25
ADC0808_OE = gpio26
ADC0808_CLK = gpio27

gpio1->gpio8 for data
gpi01 to OUT1
...
gpio8 to OUT8
*/

int gpio_config_adc[8] = {ADC0808_A, ADC0808_B, ADC0808_C, ADC0808_ALE,
			 ADC0808_START, ADC0808_EOC, ADC0808_OE, ADC0808_CLK};
int gpio_adc_data[8] = {1, 2, 3, 4, 5, 6, 7, 8};

void adc_gpio_set_output(unsigned int *base_addr)
{
	int i;

	for (i = 0; i < 8; i++) {
		gpio_set_direction(base_addr, OUTPUT, gpio_config_adc[i]);
	}
}

unsigned char read_byte(unsigned int *base_addr)
{
	int bit;
	unsigned char ret = 0;
	int i;

	for (i = 0; i < 8; i++) {
		bit = get_pin_state(base_addr, gpio_adc_data[i]);
		ret |= i << bit;
	}

	return ret;
}

void temp_show(unsigned int *base_addr, unsigned char temp)
{
	goto_xy(base_addr, 0, 10);
	write_char(base_addr, (temp / 100) + 48);
	write_char(base_addr, ((temp % 100) / 10) + 48);
	write_char(base_addr, (temp % 10) + 48);
}

unsigned char ADC0808_Read(unsigned int *base_addr, unsigned char channel)
{
	unsigned char ret;

	/* Config ADC*/
	gpio_set_value(base_addr, ADC0808_A, channel & 0x01);
	gpio_set_value(base_addr, ADC0808_B, channel & 0x02);
	gpio_set_value(base_addr, ADC0808_C, channel & 0x04);

	gpio_set_value(base_addr, ADC0808_ALE, 1);
	gpio_set_value(base_addr, ADC0808_START, 1);

	gpio_set_value(base_addr, ADC0808_ALE, 0);
	gpio_set_value(base_addr, ADC0808_START, 0);
	/*End config*/
	
	while (!get_pin_state(base_addr, ADC0808_EOC));
	gpio_set_value(base_addr, ADC0808_OE, 1);
	ret = read_byte(base_addr);
	gpio_set_value(base_addr, ADC0808_OE, 0);

	return ret;
}
