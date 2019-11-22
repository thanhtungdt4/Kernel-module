#include "lcdILI9341.h"

int width = ILI9341_TFTWIDTH; //By default
int height = ILI9341_TFTHEIGHT; //By default

void gpio_set_pin(int gpio, pin_state_t state)
{
	gpio_direction_output(gpio, 1);
	gpio_set_value(gpio, state);
}

void spiWrite_command(void *dev, unsigned char c)
{
	gpio_set_pin(_DC_PIN, Low);
	spi_write(dev, &c, sizeof(c));
}

void spiWrite_data(void *dev, unsigned char c)
{
	gpio_set_pin(_DC_PIN, High);
	spi_write(dev, &c, sizeof(c));
}

void lcd_reset(void)
{
	gpio_set_pin(LCD_RESET_PIN, Low);
	ndelay(2000);
	gpio_set_pin(LCD_RESET_PIN, High);
}

void lcd_begin(void *dev)
{
	if (!gpio_is_valid(_DC_PIN)) {
		pr_err("gpio pin %d is not available\n", _DC_PIN);
		return;
	}
	gpio_request(_DC_PIN, "DC pin");

	if (!gpio_is_valid(LCD_RESET_PIN)) {
		pr_err("gpio pin %d is not available\n", LCD_RESET_PIN);
		return;
	}
	gpio_request(LCD_RESET_PIN, "DC pin");

	lcd_reset();
	/* software reset */
	//spiWrite_command(dev, 0x01);
	ndelay(500);

        spiWrite_command(dev, 0xCB);
        spiWrite_data(dev, 0x39);
        spiWrite_data(dev, 0x2C);
        spiWrite_data(dev, 0x00);
        spiWrite_data(dev, 0x34);
        spiWrite_data(dev, 0x02);

	spiWrite_command(dev, 0xCF);
	spiWrite_data(dev, 0x00);
	spiWrite_data(dev, 0xC1);
	spiWrite_data(dev, 0x30);

	spiWrite_command(dev, 0xE8);
	spiWrite_data(dev, 0x85);
	spiWrite_data(dev, 0x00);
	spiWrite_data(dev, 0x78);

	spiWrite_command(dev, 0xEA);
	spiWrite_data(dev, 0x00);
	spiWrite_data(dev, 0x00);

        spiWrite_command(dev, 0xED);
        spiWrite_data(dev, 0x64);
        spiWrite_data(dev, 0x03);
        spiWrite_data(dev, 0x12);
        spiWrite_data(dev, 0x81);

        spiWrite_command(dev, 0xF7);
        spiWrite_data(dev, 0x20);

	spiWrite_command(dev, ILI9341_PWCTR1);
	spiWrite_data(dev, 0x23);

	spiWrite_command(dev, ILI9341_PWCTR2);
	spiWrite_data(dev, 0x10);

	spiWrite_command(dev, ILI9341_VMCTR1);
	spiWrite_data(dev, 0x3E);
	spiWrite_data(dev, 0x28);

	spiWrite_command(dev, ILI9341_VMCTR2);
	spiWrite_data(dev, 0x86);

	spiWrite_command(dev, ILI9341_MADCTL);
	spiWrite_data(dev, 0x48);

	spiWrite_command(dev, ILI9341_PIXFMT);
	spiWrite_data(dev, 0x55);

	spiWrite_command(dev, ILI9341_FRMCTR1);
	spiWrite_data(dev, 0x00);
	spiWrite_data(dev, 0x18);

	spiWrite_command(dev, ILI9341_DFUNCTR);
	spiWrite_data(dev, 0x08);
	spiWrite_data(dev, 0x82);
	spiWrite_data(dev, 0x27);

	spiWrite_command(dev, 0xF2);
	spiWrite_data(dev, 0x00);

	spiWrite_command(dev, ILI9341_GAMMASET);
	spiWrite_data(dev, 0x01);

	spiWrite_command(dev, ILI9341_GMCTRP1);
	spiWrite_data(dev, 0x0F);
	spiWrite_data(dev, 0x31);
	spiWrite_data(dev, 0x2B);
	spiWrite_data(dev, 0x0C);
	spiWrite_data(dev, 0x0E);
	spiWrite_data(dev, 0x08);
	spiWrite_data(dev, 0x4E);
	spiWrite_data(dev, 0xF1);
	spiWrite_data(dev, 0x37);
	spiWrite_data(dev, 0x07);
	spiWrite_data(dev, 0x10);
	spiWrite_data(dev, 0x03);
	spiWrite_data(dev, 0x0E);
	spiWrite_data(dev, 0x09);
	spiWrite_data(dev, 0x00);

	spiWrite_command(dev, ILI9341_GMCTRN1);
	spiWrite_data(dev, 0x00);
	spiWrite_data(dev, 0x0E);
	spiWrite_data(dev, 0x14);
	spiWrite_data(dev, 0x03);
	spiWrite_data(dev, 0x11);
	spiWrite_data(dev, 0x07);
	spiWrite_data(dev, 0x31);
	spiWrite_data(dev, 0xC1);
	spiWrite_data(dev, 0x48);
	spiWrite_data(dev, 0x08);
	spiWrite_data(dev, 0x0F);
	spiWrite_data(dev, 0x0C);
	spiWrite_data(dev, 0x31);
	spiWrite_data(dev, 0x36);
	spiWrite_data(dev, 0x0F);

	spiWrite_command(dev, ILI9341_SLPOUT);
	ndelay(500);
	spiWrite_command(dev, ILI9341_DISPON);

	setRotaion(dev, 4);
}

void setAddrWindow(void *dev, int x0, int y0, int x1, int y1)
{
	spiWrite_command(dev, ILI9341_CASET);
	spiWrite_data(dev, x0 >> 8);
	spiWrite_data(dev, x0 & 0xFF);
	spiWrite_data(dev, x1 >> 8);
	spiWrite_data(dev, x1 & 0xFF);

	spiWrite_command(dev, ILI9341_PASET);
	spiWrite_data(dev, y0 >> 8);
	spiWrite_data(dev, y0);
	spiWrite_data(dev, y1 >> 8);
	spiWrite_data(dev, y1);

	spiWrite_command(dev, ILI9341_RAMWR);
}

void drawPixel(void *dev, int x, int y, int color)
{
	if ((x < 0) || (x >= width) || (y < 0) || (y >= height))
		return;
	setAddrWindow(dev, x, y, x + 1, y + 1);
	spiWrite_data(dev, color >> 8);
	spiWrite_data(dev, color);
}

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

void setRotaion(void *dev, unsigned char m)
{
	unsigned char rotation;

	spiWrite_command(dev, ILI9341_MADCTL);
	rotation = m % 4;
	switch (rotation) {
	case 0:
		spiWrite_data(dev, MADCTL_MX | MADCTL_BGR);
		width = ILI9341_TFTWIDTH;
		height = ILI9341_TFTHEIGHT;
		break;
	case 1:
		spiWrite_data(dev, MADCTL_MV | MADCTL_BGR);
		width = ILI9341_TFTHEIGHT;
		height = ILI9341_TFTWIDTH;
		break;
	case 2:
		spiWrite_data(dev, MADCTL_MY | MADCTL_BGR);
		width = ILI9341_TFTWIDTH;
		height = ILI9341_TFTHEIGHT;
		break;
	case 3:
		spiWrite_data(dev, MADCTL_MX | MADCTL_MY | MADCTL_MV |
				MADCTL_BGR);
		width = ILI9341_TFTHEIGHT;
		height = ILI9341_TFTWIDTH;
		break;
	}
}

void invertDisplay(void *dev, int i)
{
	spiWrite_command(dev, i ? ILI9341_INVON : ILI9341_INVOFF);
}

void fillScreen(void *dev, int color)
{
	int x, y;

	for (x = 0; x < ILI9341_TFTWIDTH; x++) {
		for (y = 0; y < ILI9341_TFTHEIGHT; y++) {
			drawPixel(dev,x, y, color);
		}
	}
}
