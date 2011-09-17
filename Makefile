CC      = avr-gcc
RM      = rm
CXX     = avr-g++
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

TARGET_HEX = led.hex

ARDUINO_DIR = /home/jwhite/w/led/arduino-0022/
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
			-I. -I$(ARDUINO_CORE_PATH) \
			-g -Os -w -Wall \
			-ffunction-sections -fdata-sections
CFLAGS        = -std=gnu99
CXXFLAGS      = -fno-exceptions
LDFLAGS       = -mmcu=$(MCU) -lm -Wl,--gc-sections -Os


%.o: $(ARDUINO_CORE_PATH)/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.o: %.acpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.o: $(ARDUINO_CORE_PATH)/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

OBJECTS=generated_led.o Print.o HardwareSerial.o wiring.o main.o

all: $(TARGET_HEX) drive

generated_led.cpp: led.pde
	@echo '#include <WProgram.h>' > $@
	@cat $< >>$@

drive: drive.c
	gcc -Wall -o $@ $<


led.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

led.hex: led.elf

Print.o: $(ARDUINO_CORE_PATH)/Print.cpp
HardwareSerial.o: $(ARDUINO_CORE_PATH)/HardwareSerial.cpp
wiring.o: $(ARDUINO_CORE_PATH)/wiring.c
main.o: $(ARDUINO_CORE_PATH)/main.cpp
led.o: led.cpp

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
	$(RM) $(OBJECTS) $(TARGET_HEX) led.elf generated_led.cpp drive
