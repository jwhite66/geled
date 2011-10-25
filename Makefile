# Configure here
ARDUINO_DIR = /home/led/geled/arduino-0022/

# You can set MESSAGE in the environment or just use the default
ifndef MESSAGE
MESSAGE=[red]TC[green] MAKER [orange]@ [yellow]HACK[purple] FACTORY
endif

# You can set MESSAGE in the environment or just use the default
ifndef OUTDIR
OUTDIR=.
endif



CC      = avr-gcc
RM      = rm
CXX     = avr-g++
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

TARGET_HEX = $(OUTDIR)/led.hex

ARDUINO_LIB_PATH  = $(ARDUINO_DIR)/libraries
ARDUINO_CORE_PATH = $(ARDUINO_DIR)/hardware/arduino/cores/arduino

MCU          = atmega328
MCUPART      = m328p
F_CPU        = 16000000
ARDUINO_PORT = /dev/ttyUSB*
# Expand and pick the first port
ARD_PORT      = $(firstword $(wildcard $(ARDUINO_PORT)))

AVRDUDE_ARD_PROGRAMMER = arduino
AVRDUDE_ARD_BAUDRATE   = 57600
AVRDUDE_COM_OPTS = -q -V -p $(MCUPART)
AVRDUDE_ARD_OPTS = -c $(AVRDUDE_ARD_PROGRAMMER) -b $(AVRDUDE_ARD_BAUDRATE) -P $(ARD_PORT) $(AVRDUDE_ARD_EXTRAOPTS)

CPPFLAGS      = -mmcu=$(MCU) -DF_CPU=$(F_CPU) \
			-I. -I$(OUTDIR) -I$(ARDUINO_CORE_PATH) \
			-g -Os -w -Wall \
			-ffunction-sections -fdata-sections
CFLAGS        = -std=gnu99
CXXFLAGS      = -fno-exceptions
LDFLAGS       = -mmcu=$(MCU) -lm -Wl,--gc-sections -Os

LIBCONFIGCFLAGS  := $(shell pkg-config --cflags libconfig)
LIBCONFIGLDFLAGS := $(shell pkg-config --libs libconfig)

SIMCFLAGS  := $(shell pkg-config --cflags xcb xcb-keysyms)
SIMLDFLAGS := $(shell pkg-config --libs xcb xcb-keysyms) -lpthread -L $(OUTDIR) -lledsim

FTCFLAGS  := $(shell pkg-config --cflags freetype2)
FTLDFLAGS := $(shell pkg-config --libs freetype2)

$(OUTDIR)/%.o: $(ARDUINO_CORE_PATH)/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(OUTDIR)/%.o: $(ARDUINO_CORE_PATH)/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(OUTDIR)/%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(OUTDIR)/%.hex: $(OUTDIR)/%.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

OBJECTS=$(OUTDIR)/generated_led.o $(OUTDIR)/Print.o $(OUTDIR)/HardwareSerial.o $(OUTDIR)/wiring.o $(OUTDIR)/main.o

all: $(TARGET_HEX) $(OUTDIR)/drive $(OUTDIR)/makemap

$(OUTDIR)/message.h: Makefile $(OUTDIR)/makemap
	$(OUTDIR)/makemap elegante_pixel.ttf "$(MESSAGE)" > $(OUTDIR)/message.h

$(OUTDIR)/generated_led.cpp: led.pde led.h $(OUTDIR)/message.h
	@echo '#include <WProgram.h>' > $@
	@cat $< >>$@

$(OUTDIR)/generated_led.o: $(OUTDIR)/generated_led.cpp

$(OUTDIR)/drive: drive.c led.h
	gcc -Wall -I. -o $@ $<

$(OUTDIR)/libled.a: $(OUTDIR)/libled.o
	ar r $@ $<

$(OUTDIR)/libled.o: libled.c led.h
	gcc -Wall -c -I. $(LIBLEDCFLAGS) -o $@ $< $(LIBLEDLDFLAGS)

$(OUTDIR)/libledsim.a: $(OUTDIR)/ledsim.o
	ar r $@ $<

$(OUTDIR)/ledsim.o: ledsim.c led.h Makefile
	gcc $(SIMCFLAGS) -c -o $@ $<

$(OUTDIR)/testlib: testlib.c led.h $(OUTDIR)/libled.a
	gcc -Wall -I. -o $@ $< $(OUTDIR)/libled.a $(LIBCONFIGLDFLAGS)

$(OUTDIR)/testlibsim: testlib.c led.h $(OUTDIR)/libledsim.a
	gcc -Wall -I. -DSIMULATOR $(SIMCFLAGS) -o $@ $< $(SIMLDFLAGS)

$(OUTDIR)/ledscroll: ledscroll.c led.h $(OUTDIR)/libled.a
	gcc -Wall -I. $(FTCFLAGS) -o $@ $< $(OUTDIR)/libled.a $(LIBCONFIGLDFLAGS) $(FTLDFLAGS)

$(OUTDIR)/ledscrollsim: ledscroll.c led.h $(OUTDIR)/libledsim.a
	gcc -Wall -I. -DSIMULATOR $(FTCFLAGS) $(SIMCFLAGS) -o $@ $< $(SIMLDFLAGS) $(FTLDFLAGS)

$(OUTDIR)/warsim: war.c led.h $(OUTDIR)/libledsim.a
	gcc -Wall -I. -DSIMULATOR $(SIMCFLAGS) -o $@ $< $(SIMLDFLAGS) -lm
$(OUTDIR)/war: war.c led.h $(OUTDIR)/libled.a
	gcc -Wall -I. $(LIBLEDCFLAGS) -o $@ $<  $(LIBLEDLDFLAGS) -L$(OUTDIR) -lled -l pthread $(LIBCONFIGLDFLAGS) -lm

$(OUTDIR)/makemap: makemap.c led.h
	gcc -Wall -o $@ -I. -I $(OUTDIR) -I /usr/include/freetype2 $< -lfreetype -lm


$(OUTDIR)/led.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

$(OUTDIR)/led.hex: $(OUTDIR)/led.elf

$(OUTDIR)/Print.o: $(ARDUINO_CORE_PATH)/Print.cpp
$(OUTDIR)/HardwareSerial.o: $(ARDUINO_CORE_PATH)/HardwareSerial.cpp
$(OUTDIR)/wiring.o: $(ARDUINO_CORE_PATH)/wiring.c
$(OUTDIR)/main.o: $(ARDUINO_CORE_PATH)/main.cpp

upload:		reset raw_upload

raw_upload:	$(TARGET_HEX)
		$(AVRDUDE) $(AVRDUDE_COM_OPTS) $(AVRDUDE_ARD_OPTS) \
			-U flash:w:$(TARGET_HEX):i

# BSD stty likes -F, but GNU stty likes -f/--file.  Redirecting
# stdin/out appears to work but generates a spurious error on MacOS at
# least. Perhaps it would be better to just do it in perl ?
reset:		
	stty --file $(ARD_PORT)  hupcl ; \
	(sleep 0.1 2>/dev/null || sleep 1) ; \
	stty --file $(ARD_PORT) -hupcl 

clean:
	$(RM) $(OBJECTS) $(TARGET_HEX) $(OUTDIR)/led.elf $(OUTDIR)/generated_led.cpp $(OUTDIR)/drive $(OUTDIR)/makemap $(OUTDIR)/message.h
