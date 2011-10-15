CFLAGS  := -Wall -I. $(shell pkg-config --cflags xcb xcb-keysyms)
LDFLAGS := $(shell pkg-config --libs xcb xcb-keysyms) -lpthread


libledsim.a: ledsim.o
	ar r $@ $<

ledsim.o: ledsim.c ../led.h Makefile
	gcc $(CFLAGS) -c -o $@ $< $(LDFLAGS)

#LIBS    := libledsim.so
#war: war.c $(LIBS)
#	gcc $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)
