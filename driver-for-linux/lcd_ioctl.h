#ifndef __LCD_IOCTL
#define __LCD_IOCTL

#define MAGIC_NO		100

typedef struct pixel {
	int x;
	int y;
	int color;
} pixel_t;
#define DRAW_PIXEL		_IOW(MAGIC_NO, 0, pixel_t)

#define SET_ROTATION		_IOW(MAGIC_NO, 1, unsigned char)
#define INVERT_DISPLAY		_IOW(MAGIC_NO, 2, int)

#define LCD_RESET		_IO(MAGIC_NO, 3)

#endif
