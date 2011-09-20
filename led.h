// TODO - we're getting the occasional odd color bulb - why?

#define MAX_BRIGHT          0xCC
#define ADDR_COUNT          50
#define STRING_COUNT        6
#define ALL_STRINGS_MASK    0x3F


#define SLICE_LEN_IN_MICROS 10
#define CYCLES_PER_SLICE    ((F_CPU / 1000000) * SLICE_LEN_IN_MICROS)
    /* 16,000,000 cycles / sec == 16 cyles / us */
    /*  For one interrupt every 10 us, we'd get 160 */

#define MICROS_PER_SLICE    ((CYCLES_PER_SLICE * 1000000) / F_CPU)


#define START_SLICES        1
#define STOP_SLICES         3
#define SLICES_PER_BIT      3
#define BITS_PER_BULB       26
#define SLICES_TO_SHOW_BULB (START_SLICES + (SLICES_PER_BIT * BITS_PER_BULB) + STOP_SLICES)


#define DEFAULT_ARDUINO_SERIAL_PORT "/dev/ttyUSB0"
