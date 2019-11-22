#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "../lcd_ioctl.h"
#include "lcdILI9341_lib.h"
#include "5x5_font.h"

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#define pgm_read_pointer(addr) ((void *)pgm_read_word(addr))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }

int width = 240;
int height = 320;

int open_file(void)
{
	int fd;

	fd = open(FILENAME, O_RDWR);
	if (fd < 0) {
		perror("open: Failed\n");
		return fd;
	}

	return fd;
}

void drawPixel(int x, int y, int color)
{
	int fd;
	pixel_t pixel;

	fd = open_file();
	pixel.x = x;
	pixel.y = y;
	pixel.color = color;

	ioctl(fd, DRAW_PIXEL, &pixel);

	close(fd);	
}

int color565(int r, int g, int b)
{
	int fd;
	color565_t color;

	fd = open_file();
	color.r = r;
	color.g = g;
	color.b = b;

	ioctl(fd, COLOR_565, &color);

	close(fd);

	return color;
}

void setRotation(unsigned char m)
{
	int fd;

	fd = open_file();
	ioctl(fd, SET_ROTATION, &m);
	close(fd);
}

void invertDisplay(int i)
{
	int fd;

	fd = open_file();
	ioctl(fd, INVERT_DISPLAY, &i);
	close(fd);
}

void lcd_reset(void)
{
	int fd;

	fd = open_file();
	ioctl(fd, LCD_RESET);
	close(fd);
}

void drawCircle(int x0, int y0, int r, int color)
{
	int f = 1 - r;
	int ddF_x = 1;
	int ddF_y = -2 * r;
	int x = 0;
	int y = r;

	drawPixel(x0, y0 + r, color);
	drawPixel(x0, y0 - r, color);
	drawPixel(x0 + r, y0, color);
	drawPixel(x0 - r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		drawPixel(x0 + x, y0 + y, color);
		drawPixel(x0 - x, y0 + y, color);
		drawPixel(x0 + x, y0 - y, color);
		drawPixel(x0 - x, y0 - y, color);
		drawPixel(x0 + y, y0 + x, color);
		drawPixel(x0 - y, y0 + x, color);
		drawPixel(x0 + y, y0 - x, color);
		drawPixel(x0 - y, y0 - x, color);
	}
}

void drawCircleHelper(int x0, int y0, int r, char cornername, int color)
{
	int f = 1 - r;
	int ddF_x = 1;
	int ddF_y = -2 * r;
	int x = 0;
	int y = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		if (cornername & 0x4) {
			drawPixel(x0 + x, y0 + y, color);
			drawPixel(x0 + y, y0 + x, color);
		}

		if (cornername & 0x2) {
			drawPixel(x0 + x, y0 - y, color);
			drawPixel(x0 + y, y0 - x, color);
		}

		if (cornername & 0x8) {
			drawPixel(x0 - y, y0 + x, color);
			drawPixel(x0 - x, y0 + y, color);
		}

		if (cornername & 0x1) {
			drawPixel(x0 - y, y0 - x, color);
			drawPixel(x0 - x, y0 - y, color);
		}
	}
}

void fillCircle(int x0, int y0, int r, int color)
{
	drawFastVLine(x0, y0 - r, 2 * r + 1, color);
	fillCircleHelper(x0, y0, r, 3, 0, color);
}

void fillCircleHelper(int x0, int y0, int r, char cornername, int delta,
			int color)
{
	int f = 1 - r;
	int ddF_x = 1;
	int ddF_y = -2 * r;
	int x = 0;
	int y = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		if (cornername & 0x1) {
			drawFastVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
			drawFastVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
		}

		if (cornername & 0x2) {
			drawFastVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
			drawFastVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
		}
	}
}

void drawLine(int x0, int y0, int x1, int y1, int color)
{
	int steep = abs(y1 - y0) > abs(x1 - x0);
	int dx, dy;

	if (steep) {
		_swap_int16_t(x0, y0);
		_swap_int16_t(x1, y1);
	}

	if (x0 > x1) {
		_swap_int16_t(x0, x1);
		_swap_int16_t(y0, y1);
	}

	dx = x1 - x0;
	dy = abs(y1 - y0);

	int err = dx / 2;
	int ystep;

	if (y0 < y1)
		ystep = 1;
	else
		ystep = -1;

	for (; x0 <= x1; x0++) {
		if (steep)
			drawPixel(y0, x0, color);
		else
			drawPixel(x0, y0, color);
		err -= dy;
		if (err < 0) {
			y0 += ystep;
			err += dx;
		}
	}
}

void drawRect(int x, int y, int w, int h, int color)
{
	drawFastHLine(x, y, w, color);
	drawFastHLine(x, y + h - 1, w, color);
	drawFastVLine(x, y, h, color);
	drawFastVLine(x + w - 1, y, h, color);
}

void drawFastVLine(int x, int y, int h, int color)
{
	drawLine(x, y, x, y + h - 1, color);
}

void drawFastHLine(int x, int y, int w, int color)
{
	drawLine(x, y, x + w - 1, y, color);
}

void fillRect(int x, int y, int w, int h, int color)
{
	int i;

	for (i = x; i < x + w; i++) {
		drawFastVLine(i, y, h, color);
	}
}

void fillScreen(int color)
{
	fillRect(0, 0, width, height, color);
}

void drawRoundRect(int x, int y, int w, int h, int r, int color)
{
	drawFastHLine(x + r, y, w - 2 * r, color);
	drawFastHLine(x + r, y + h - 1, w - 2 * r, color);
	drawFastVLine(x, y + r, h - 2 * r, color);
	drawFastVLine(x + w - 1, y + r, h - 2 * r, color);

	drawCircleHelper(x + r, y + r, r, 1, color);
	drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
	drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
	drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

void fillRoundRect(int x, int y, int w, int h, int r, int color)
{
	fillRect(x + r, y, w - 2 * r, h, color);

	fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
	fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}

void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int color)
{
	drawLine(x0, y0, x1, y1, color);
	drawLine(x1, y1, x2, y2, color);
	drawLine(x2, y2, x0, y0, color);
}

void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int color)
{
	int a, b, y, last;

	if (y0 > y1) {
		_swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
	}
	if (y1 > y2) {
		_swap_int16_t(y2, y1); _swap_int16_t(x2, x1);
	}
	if (y0 > y1) {
		_swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
	}

	if (y0 == y2) {
		a = b = x0;
		if (x1 < a) a = x1;
		else if (x1 > b) b = x1;
		if (x2 < a)      a = x2;
		else if (x2 > b) b = x2;
		drawFastHLine(a, y0, b - a + 1, color);
		return;
	}

	int 
	dx01 = x1 - x0,
	dy01 = y1 - y0,
	dx02 = x2 - x0,
	dy02 = y2 - y0,
	dx12 = x2 - x1,
	dy12 = y2 - y1;
	int
	sa = 0,
	sb = 0;

	if (y1 == y2) last = y1;
	else         last = y1 - 1;

	for(y = y0; y <= last; y++) {
		a = x0 + sa / dy01;
		b = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;

		if (a > b) _swap_int16_t(a, b);
		drawFastHLine(a, y, b - a + 1, color);
	}

	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for (; y <= y2; y++) {
		a = x1 + sa / dy12;
		b = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;
		if (a > b) _swap_int16_t(a,b);
		drawFastHLine(a, y, b - a + 1, color);
	}
}

void drawBitmap(int x, int y, const unsigned char *bitmap, int w, int h,
		int color)
{
	int i, j, byteWidth = (w + 7) / 8;
	unsigned char byte;

	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			if (i & 7) byte <<= 1;
			else byte = pgm_read_byte(bitmap + j * byteWidth + i / 8);
			if (byte & 0x80) drawPixel(x + i, y + j, color);
		}
	}
}

void drawChar(char c, int x, int y, int color, int size, int bgcolor)
{
	char character = c;
	int i, j;
	int k;

	if (character < ' ') {
		c = 0;
	} else {
		character -= 32;
	}

	char temp[CHAR_WIDTH];
	for (k = 0; k < CHAR_WIDTH; k++) {
		temp[k] = font[function_char][k];
	}

	drawRect(x, y, CHAR_WIDTH * size, CHAR_HEIGHT * size, bgcolor);

	for (j = 0; j < CHAR_WIDTH; j++) {
		for (i = 0; i < CHAR_HEIGHT; i++) {
			if (temp[j] & (1 << i)) {
				if (size == 1)
					drawPixel(x + j, y + i, color);
				else
					drawRect(x + (j * size), y + (i * size),
						size, size, color);
			}
		}
	}
}

void drawText(const char *text, int x, int y, int color, int size, int bgcolor)
{
	while (*text) {
		drawChar(*text, x, y, color, size, bgcolor);
		x += CHAR_WIDTH * size;
	}
}
