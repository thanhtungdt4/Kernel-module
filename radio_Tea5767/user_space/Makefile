INCLUDE_DIR=./lib
CFLAGS=-I$(INCLUDE_DIR) -Wall
LDFLAGS=-L$(INCLUDE_DIR) -lTea5767_lib

TARGET=Tea5767_user
CC=gcc

all:
	#$(CC) test_lcd.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)
	make -C ./lib
	$(CC) Tea5767_user.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)
clean:
	make -C ./lib clean
	rm $(TARGET)
