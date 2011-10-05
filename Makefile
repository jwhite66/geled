# Configure here
ARDUINO_DIR = /home/led/geled/arduino-0022/

# You can set MESSAGE in the environment or just use the default
ifndef MESSAGE
MESSAGE=HACK FACTORY
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
		for STTYF in 'stty --file' 'stty -f' 'stty <' ; \
		  do $$STTYF /dev/tty >/dev/null 2>&1 && break ; \
		done ; \
		$$STTYF $(ARD_PORT)  hupcl ; \
		(sleep 0.1 2>/dev/null || sleep 1) ; \
		$$STTYF $(ARD_PORT) -hupcl 

clean:
	$(RM) $(OBJECTS) $(TARGET_HEX) $(OUTDIR)/led.elf $(OUTDIR)/generated_led.cpp $(OUTDIR)/drive $(OUTDIR)/makemap $(OUTDIR)/message.h
