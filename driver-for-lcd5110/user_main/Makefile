
INCLUDE_DIR=../lib
CFLAGS=-I$(INCLUDE_DIR) -Wall
LDFLAGS=-L$(INCLUDE_DIR) -llcd_lib

TARGET=test
CC=gcc

all:
	#$(CC) test_lcd.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)
	$(CC) test_btn.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)
clean:
	rm $(TARGET) 

