.PHONY : clean

CFLAGS= -fPIC
LDFLAGS= -shared

SOURCES = $(shell echo *.c)
HEADERS = $(shell echo *.h)
OBJECTS=$(SOURCES:.c=.o)
TARGET=./liblcd_lib.so

all: $(TARGET)

clean: 
	rm -f $(OBJECTS) $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)
