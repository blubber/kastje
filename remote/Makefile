ARDUINO_DIR = /usr/share/arduino
MCU = atmega328p
F_CPU = 8000000
ARDUINO_DIR = /usr/share/arduino
ARDUINO_PORT = /dev/ttyUSB0


CC 		= avr-gcc
CXX 		= avr-g++
AR 		= avr-ar
AVRDUDE		= avrdude
CPPFLAGS      	= -mmcu=$(MCU) -DF_CPU=$(F_CPU) \
			-I. -Iarduino-core/ \
			$(SYS_INCLUDES) -g -Os -w -Wall \
			-ffunction-sections -fdata-sections
CFLAGS        	= -std=gnu99
CXXFLAGS      	= -fno-exceptions
ASFLAGS       	= -mmcu=$(MCU) -I. -x assembler-with-cpp
LDFLAGS       	= -mmcu=$(MCU) -L/usr/lib/avr/lib -lm -Wl,--gc-sections -Os

CORE_C_SRCS     = $(wildcard arduino-core/*.c)
CORE_CPP_SRCS   = $(wildcard arduino-core/*.cpp)
CORE_OBJ_FILES  = $(CORE_C_SRCS:.c=.o) $(CORE_CPP_SRCS:.cpp=.o)

SRC_CPP_SRCS = $(wildcard *.cc)
SRC_OBJ_FILES = $(SRC_CPP_SRCS:.cc=.o)

all:  applet.hex

clean:
	rm -rf applet.hex applet.elf *.o *.a arduino-core/*.o

dist-clean: clean
	rm -rf arduino-core


%.o: %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

applet.elf: $(SRC_OBJ_FILES)
	$(CC) -Os -Wl,--gc-sections -mmcu=$(MCU) -o applet.elf \
		-L . -lm $(SRC_OBJ_FILES)

applet.hex: applet.elf
	avr-objcopy -O ihex applet.elf applet.hex

upload:	raw_upload

raw_upload: applet.hex
	avrdude -C /etc/avrdude.conf -c avrispmkII \
		-P usb  -p atmega328p \
		-U flash:w:applet.hex:i

reset:
	stty --file $(ARDUINO_PORT) hupcl; \
	(sleep 0.1 2>/dev/null || sleep 1) ; \
	stty --file $(ARDUINO_PORT) -hupcl 

.PHONY: all reset upload raw_upload
