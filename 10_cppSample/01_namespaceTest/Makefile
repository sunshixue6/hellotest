SOURCE = $(wildcard ./*.cc)
OBJECT  = $(patsubst %cc, %o, $(SOURCE))

INCLUDES = -I .

TARGET  = target
CC      = g++
CFLAGS  = -Wall –g
#CXXFLAGS = -Wall -Wl,-Map,signal.map -g
CXXFLAGS = -Wall -g

#-Wl,-Map,helloworld.map
#CFLAGS += -I./jpegarm/include

#LFLAGS += -ljpeg -L/home/psdcd/

#LFLAGS += -lpthread -ldl
LFLAGS += -luuid

$(TARGET): $(OBJECT)
	@mkdir -p output/
	$(CC) $^ $(CXXFLAGS) -o output/$(TARGET) $(LFLAGS)

%.o: %.c
	$(CC) $(INCLUEDS) -c $< -o $@
#	$(CC) $(INCLUEDS) $(CFLAGES) -c $< -o $@

.PHONY:clean
clean:
	@rm -rf $(OBJECT) output/ *.map
