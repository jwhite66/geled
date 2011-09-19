// TODO - we're getting the occasional odd color bulb - why?
// TODO - is the padding logic really needed?  Maybe not
// TODO - the safe slices stuff is a bit on the guesstimate side

#define MAX_BRIGHT          0xCC
#define ADDR_COUNT          50
#define STRING_COUNT        6
#define ALL_STRINGS_MASK    0x3F


#define SLICE_LEN_IN_MICROS 11
#define CYCLES_PER_SLICE    ((F_CPU / 1000000) * SLICE_LEN_IN_MICROS)
    /* 16,000,000 cycles / sec == 16 cyles / us */
    /*  For one interrupt every 10 us, we'd get 160 */

#define MICROS_PER_SLICE    ((CYCLES_PER_SLICE * 1000000) / F_CPU)


#define TRAILING_LOWS       3
#define SLICES_TO_SHOW_BULB (79 + TRAILING_LOWS)


/*----------------------------------------------------------------------------
**  Be careful - this struct is no for general purpose use.
**   It's sole use is to provide shift registers for getting
**   bits to write out to the port.
**--------------------------------------------------------------------------*/
typedef struct bulb_struct
{
    unsigned char string;
    unsigned char addr;
    unsigned char bright;
    unsigned char r;
    unsigned char g;
    unsigned char b;
} bulb;

#define DEFAULT_ARDUINO_SERIAL_PORT "/dev/ttyUSB0"
