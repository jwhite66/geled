#define LED_CFG_FILE        "led.cfg"

#define MAX_BRIGHT          0xCC
#define MAX_RGB             0xF
#define ADDR_COUNT          50
#define STRING_COUNT        6
#define ALL_STRINGS_MASK    0x3F


#define SLICE_LEN_IN_MICROS 7
#define CYCLES_PER_SLICE    ((F_CPU / 1000000) * SLICE_LEN_IN_MICROS)
    /* 16,000,000 cycles / sec == 16 cyles / us */
    /*  For one interrupt every 10 us, we'd get 160 */

#define MICROS_PER_SLICE    ((CYCLES_PER_SLICE * 1000000) / F_CPU)


#define START_SLICES        2
#define STOP_SLICES         4
#define SLICES_PER_BIT      4
#define BITS_PER_BULB       26
#define SLICES_TO_SHOW_BULB (START_SLICES + (SLICES_PER_BIT * BITS_PER_BULB) + STOP_SLICES)


#define DEFAULT_ARDUINO_SERIAL_PORT "/dev/ttyUSB0"

/*----------------------------------------------------------------------------
**  Define the structure of a 4 byte on wire bulb sequence.
**    It's generally:
**      flags address | blue string | green red | bright
**  where flags are 2 bits, address 6, blue/string/green/red are
**  each 4 bits each.
**--------------------------------------------------------------------------*/
#define BULB_FLAG_COMMAND   0x80
#define BULB_FLAG_COMBINE   0x40

#define BULB_FLAG_ADDRESS(b)((b)[0])
#define BULB_BLUE_STRING(b) ((b)[1])
#define BULB_GREEN_RED(b)   ((b)[2])
#define BULB_BRIGHT(b)      ((b)[3])

#define IS_COMMAND(b)       (((b)[0] & BULB_FLAG_COMMAND) == BULB_FLAG_COMMAND)
#define IS_COMBINED(b)      (((b)[0] & BULB_FLAG_COMBINE) == BULB_FLAG_COMBINE)

#define BULB_ADDRESS(b)     (BULB_FLAG_ADDRESS(b) & 0x3F)
#define BULB_STRING(b)      (BULB_BLUE_STRING(b)  & 0xF)
#define BULB_BLUESHIFT(b)   (BULB_BLUE_STRING(b)  & 0xF0)
#define BULB_GREENSHIFT(b)  (BULB_GREEN_RED(b) & 0xF0)
#define BULB_REDSHIFT(b)    (BULB_GREEN_RED(b) << 4)

/*----------------------------------------------------------------------------
**  Command list
**--------------------------------------------------------------------------*/
#define COMMAND_ACK             (BULB_FLAG_COMMAND | 1)
#define COMMAND_INIT            (BULB_FLAG_COMMAND | 2)
#define COMMAND_STATUS          (BULB_FLAG_COMMAND | 3)
#define COMMAND_CLEAR           (BULB_FLAG_COMMAND | 4)
#define COMMAND_CHASE           (BULB_FLAG_COMMAND | 5)
#define COMMAND_SCROLL_DISPLAY  (BULB_FLAG_COMMAND | 6)

#define COMMAND_SYNC            0xFF


/*----------------------------------------------------------------------------
**  Structure definitions
**--------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"{
#endif
typedef struct {
    int start;
    unsigned char bright;
    unsigned char r;
    unsigned char g;
    unsigned char b;
} scroll_color_map_t;
#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
**  Function prototypes; should work for ledsim.so and for led.so
**--------------------------------------------------------------------------*/
#if ! defined(LED_HANDLE_T)
#define LED_HANDLE_T void *
typedef void (*ledsim_x_callback)(LED_HANDLE_T h, unsigned long key);
#endif

LED_HANDLE_T led_init(void);
void led_get_size(LED_HANDLE_T h, int *wide, int *high);
void led_set_pixel(LED_HANDLE_T, int x, int y, int bright, int r, int g, int b);
void led_term(LED_HANDLE_T h);

/*  Function only used by the simulator to report X events */
#if defined(SIMULATOR)
void ledsim_set_x_callback(LED_HANDLE_T h, ledsim_x_callback x_callback);
void ledsim_join(LED_HANDLE_T h);
#endif
