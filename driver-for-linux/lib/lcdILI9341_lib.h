#ifndef _ILI9431LIB_
#define _ILI9431LIB_

//#include "gfxfont.h"

#define FILENAME		"/dev/lcdILI9341"
void drawPixel(int x, int y, int color);
void color565(int r, int g, int b);
void setRotation(unsigned char m);
void invertDisplay(int i);
void lcd_reset(void);
void drawCircle(int x0, int y0, int r, int color);
void drawCircleHelper(int x0, int y0, int r, char cornername, int color);
void fillCircle(int x0, int y0, int r, int color);
void fillCircleHelper(int x0, int y0, int r, char cornername, int delta,
			int color);
void drawLine(int x0, int y0, int x1, int y1, int color);
void drawRect(int x, int y, int w, int h, int color);
void drawFastVLine(int x, int y, int h, int color);
void drawFastHLine(int x, int y, int w, int color);
void fillRect(int x, int y, int w, int h, int color);
void fillScreen(int color);
void drawRoundRect(int x, int y, int w, int h, int r, int color);
void fillRoundRect(int x, int y, int w, int h, int r, int color);
void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int color);
void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int color);
void drawBitmap(int x, int y, const unsigned char *bitmap, int w, int h,
		int color);

#endif
