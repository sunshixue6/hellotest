SOURCE = $(wildcard ./src/*.c)
OBJECT  = $(patsubst %c, %o, $(SOURCE))

INCLUDES = -I ./inc

TARGET  = target
CC      = gcc
CFLAGS  = -Wall –g
CFLAGS += -I./jpegarm/include

LFLAGS += -ljpeg -L/home/psdcd/

$(TARGET): $(OBJECT)
        @mkdir -p output/
        $(CC) $^ $(CFLAGES) -o output/$(TARGET) $(LFLAGS)

%.o: %.c
        $(CC) $(INCLUEDS) $(CFLAGES) -c $< -o $@

.PHONY:clean
clean:
        @rm -rf $(OBJECT) output/
