SOURCE = $(wildcard ./*.c)
OBJECT  = $(patsubst %c, %o, $(SOURCE))

INCLUDES = -I .

TARGET  = paseLog
CC      = gcc
CFLAGS  = -Wall –g
#CXXFLAGS = -Wall -Wl,-Map,signal.map -g
CXXFLAGS = -Wall -g

#-Wl,-Map,helloworld.map
#CFLAGS += -I./jpegarm/include

#LPATH += -ljpeg -L/home/psdcd/

LFLAGS += -lpthread
#LFLAGS += -luuid

$(TARGET): $(OBJECT)
	$(CC) $^ $(CXXFLAGS) -o $(TARGET) $(LFLAGS)

%.o: %.c
	$(CC) $(INCLUEDS) -c $< -o $@
#	$(CC) $(INCLUEDS) $(CFLAGES) -c $< -o $@

.PHONY:clean
clean:
	@rm -rf $(OBJECT) *.o $(TARGET) *.map