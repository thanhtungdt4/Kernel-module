#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lcd_lib.h"

int main()
{
	lcd_clear_screen();
	lcd_gotoxy(13, 4);
	draw_string("Thanh Tung", Pixel_Set, FontSize_5x7);
	return 0;
}
