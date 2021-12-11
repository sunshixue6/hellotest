SOURCE = $(wildcard ./*.cc)
OBJECT  = $(patsubst %cc, %o, $(SOURCE))

INCLUDES = -I .

TARGET  = target
CC      = g++
CFLAGS  = -Wall –g
#CFLAGS += -I./jpegarm/include

#LFLAGS += -ljpeg -L/home/psdcd/

LFLAGS += -lpthread -ldl

$(TARGET): $(OBJECT)
	@mkdir -p output/
	$(CC) $^ $(CFLAGES) -o output/$(TARGET) $(LFLAGS)

%.o: %.c
	$(CC) $(INCLUEDS) $(CFLAGES) -c $< -o $@

.PHONY:clean
clean:
	@rm -rf $(OBJECT) output/
