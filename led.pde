/*----------------------------------------------------------------------------
**  led.pde
**      Driver for running GE Color Effects lights on an Amtel328p board.
**  
**  Features:
**     -  Can drive up to six strings by attaching them to port b (pins 8-13)
**     -  Gets commands via serial port
**
**  License:
**      GPL v 3 - See LICENSE
**
**--------------------------------------------------------------------------*/
#include "led.h"
#include "message.h"


/*----------------------------------------------------------------------------
**  Notes on Memory:
**      The 328 has 2K of RAM, 512 bytes of EEPROM, and 32 K of Flash RAM.
**  That makes the size of this ring buffer crucial, as it uses up the
**  bulk of available ram.
**--------------------------------------------------------------------------*/
uint8_t ring[SLICES_TO_SHOW_BULB * 10];
const static uint8_t *ring_fence = ring + sizeof(ring);

#define INCREMENT_RING_PTR(p) { if ((++(p)) == ring_fence) (p) = ring; }

uint8_t *readp = ring;
uint8_t *writep = ring;


/*----------------------------------------------------------------------------
**  Ring timer code.  This interrupt vector just pushes bytes from the ring
**      buffer out to Port B, thus driving the lights.  The complex bit
**      is staging the ring buffer just right.
**--------------------------------------------------------------------------*/
ISR(TIMER1_COMPA_vect)
{
    if (readp != writep)
    {
        PORTB = *readp;
        *readp = 0;
        INCREMENT_RING_PTR(readp);
    }
}

void start_timer1(void)
{
    uint8_t sreg;
    /* Save global interrupt flag */
    sreg = SREG;

    /* Disable interrupts */
    cli();

    /* Set timer mode */
    TCCR1A = 0;

    /* What count to interrupt the timer upon */
    OCR1A = CYCLES_PER_SLICE;

    TCCR1B = _BV(WGM12) | _BV(CS10);
                  /*  Turn on CTC1 mode, with no divider */

    TIMSK1 = _BV(OCIE1A);  /* Enable the interrupt */

    /* Restore global interrupt flag */
    SREG = sreg;
}

void stop_timer1(void)
{
    uint8_t sreg;
    /* Save global interrupt flag */
    sreg = SREG;
    /* Disable interrupts */
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;
    /* Restore global interrupt flag */
    SREG = sreg;
}

void set_writep(uint8_t *p)
{
    uint8_t sreg;
    /* Save global interrupt flag */
    sreg = SREG;
    /* Disable interrupts */
    cli();
    writep = p;
    /* Restore global interrupt flag */
    SREG = sreg;
}

int available(void)
{
    uint8_t sreg;
    int a;
    /* Save global interrupt flag */
    sreg = SREG;

    /* Disable interrupts */
    cli();
    if (writep == readp)
        a = sizeof(ring);
    else if (writep > readp)
        a = (readp - ring) + (ring_fence - writep);
    else
        a = readp - writep;

    /* Restore global interrupt flag */
    SREG = sreg;

    return a;
}

/*----------------------------------------------------------------------------
**  Bulb management code
**      This code will take a 4 byte expression of a bulb and
**  get it into the ring buffer in the right order.
**      There is one slight optimization:  we can write orders
**  for up to 6 strings simultaneously.  So we will cache a bulb
**  (on BULB_FLAG_COMBINE) and send them all at once if so requested.
**--------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
**  Be careful - this struct is not for general purpose use.
**   It's sole use is to provide preshifted bits to write out to the port.
**--------------------------------------------------------------------------*/
typedef struct bulb_struct
{
    uint8_t stringmask;
    uint8_t addrshift;
    uint8_t bright;
    uint8_t redshift;
    uint8_t greenshift;
    uint8_t blueshift;
} bulb;

void write_bits(uint8_t **p, uint8_t one_strings, uint8_t zero_strings)
{
    /* Always starts with low */
    **p &= ~(one_strings | zero_strings);
    INCREMENT_RING_PTR(*p);

    /* 3 slices of low for one, 3 of high for zero */
    **p &= ~one_strings;
    **p |= zero_strings;
    INCREMENT_RING_PTR(*p);

    **p &= ~one_strings;
    **p |= zero_strings;
    INCREMENT_RING_PTR(*p);

    /* Always ends with high */
    **p |= (one_strings | zero_strings);
    INCREMENT_RING_PTR(*p);

}

/* Note - the Sketch stuff generates function headers at the top of the file,
          after all the #includes, so don't use typedefs in prototypes or people
          won't be able to use Sketches :-(  */
void write_raw_bulbs(int count, struct bulb_struct *bulbs)
{
    uint8_t *p;
    int i, j;

    uint8_t all_bulbs;
    uint8_t low_bulbs;
    uint8_t high_bulbs;

    while (available() < SLICES_TO_SHOW_BULB)
        ;

    p = (uint8_t *) writep;

    all_bulbs = 0;
    for (i = 0; i < count; i++)
        all_bulbs |= bulbs[i].stringmask;

    /* start indicator high  */
    for (i = 0; i < START_SLICES; i++)
    {
        *p = all_bulbs;
        INCREMENT_RING_PTR(p);
    }

    /* Address bits */
    for (i = 6; i; i--)
    {
        high_bulbs = low_bulbs = 0;
        for (j = 0; j < count; j++)
        {
            if (bulbs[j].addrshift & 0x80)
                high_bulbs |= bulbs[j].stringmask;
            else
                low_bulbs  |= bulbs[j].stringmask;
            bulbs[j].addrshift <<= 1;
        }
        write_bits(&p, high_bulbs, low_bulbs);
    }

    /* Bright bits */
    for (i = 8; i; i--)
    {
        high_bulbs = low_bulbs = 0;
        for (j = 0; j < count; j++)
        {
            if (bulbs[j].bright & 0x80)
                high_bulbs |= bulbs[j].stringmask;
            else
                low_bulbs  |= bulbs[j].stringmask;
            bulbs[j].bright <<= 1;
        }
        write_bits(&p, high_bulbs, low_bulbs);
    }

    /* Blue bits */
    for (i = 4; i; i--)
    {
        high_bulbs = low_bulbs = 0;
        for (j = 0; j < count; j++)
        {
            if (bulbs[j].blueshift & 0x80)
                high_bulbs |= bulbs[j].stringmask;
            else
                low_bulbs  |= bulbs[j].stringmask;
            bulbs[j].blueshift <<= 1;
        }
        write_bits(&p, high_bulbs, low_bulbs);
    }

    /* Green bits */
    for (i = 4; i; i--)
    {
        high_bulbs = low_bulbs = 0;
        for (j = 0; j < count; j++)
        {
            if (bulbs[j].greenshift & 0x80)
                high_bulbs |= bulbs[j].stringmask;
            else
                low_bulbs  |= bulbs[j].stringmask;
            bulbs[j].greenshift <<= 1;
        }
        write_bits(&p, high_bulbs, low_bulbs);
    }

    /* Red */
    for (i = 4; i; i--)
    {
        high_bulbs = low_bulbs = 0;
        for (j = 0; j < count; j++)
        {
            if (bulbs[j].redshift & 0x80)
                high_bulbs |= bulbs[j].stringmask;
            else
                low_bulbs  |= bulbs[j].stringmask;
            bulbs[j].redshift <<= 1;
        }
        write_bits(&p, high_bulbs, low_bulbs);
    }

    /* stop indicator low */
    for (i = 0; i < STOP_SLICES; i++)
    {
        *p &= ~all_bulbs;
        INCREMENT_RING_PTR(p);
    }

    set_writep(p);

}

void process_bulb(const uint8_t *data)
{
    static bulb bulbs[STRING_COUNT];
    static int bulb_count = 0;

    bulb *p;

    p = &bulbs[bulb_count];

    p->addrshift = BULB_ADDRESS(data) << 2;
    p->stringmask = _BV(BULB_STRING(data));
    p->blueshift = BULB_BLUESHIFT(data);
    p->greenshift = BULB_GREENSHIFT(data);
    p->redshift = BULB_REDSHIFT(data);
    p->bright = BULB_BRIGHT(data);
    if (p->bright > MAX_BRIGHT)
        p->bright = MAX_BRIGHT;

    if (++bulb_count >= STRING_COUNT || (! IS_COMBINED(data)))
    {
        write_raw_bulbs(bulb_count, bulbs);
        bulb_count = 0;
    }
}

/*----------------------------------------------------------------------------
** Message scrolling code
**--------------------------------------------------------------------------*/
#define STRING_0_MAX_X  (10 - 1)
#define SCROLL_INTERVAL 200
uint8_t *g_scroll_pos = g_message_bits;
int g_scroll_width = (10 + 7);
unsigned long g_next_scroll = 0;
uint8_t g_scroll_blueshift = 0;
uint8_t g_scroll_redshift = 13 << 4;
uint8_t g_scroll_greenshift = 0;
uint8_t g_scroll_bright = MAX_BRIGHT;

uint8_t g_scrolling = 1;
uint8_t g_init_at_start = 1;

void map_xy_to_string_addr(int x, int y, int *string, int *addr)
{
    if (x > STRING_0_MAX_X)
    {
        *string = 1;
        x -= (STRING_0_MAX_X + 1);
    }
    else
        *string = 0;

    if (x % 2 == 0)
        *addr = (x * MESSAGE_ROWS) + y ;
    else
        *addr = (x * MESSAGE_ROWS) + (MESSAGE_ROWS - y) - 1;
}


scroll_color_map_t * current_map(int n)
{
    static scroll_color_map_t def;
    int i;
    scroll_color_map_t *ret;

    if (sizeof(g_scroll_color_map) == 0)
    {
        def.bright = g_scroll_bright;
        def.r = g_scroll_redshift;
        def.g = g_scroll_greenshift;
        def.b = g_scroll_blueshift;
        ret = &def;
    }

    for (i = 0; i < sizeof(g_scroll_color_map) / sizeof(g_scroll_color_map[0]); i++)
        if (n >= g_scroll_color_map[i].start)
            ret = &g_scroll_color_map[i];
        else
            break;

    return ret;
}

void scroll_display(void)
{
    int i;
    uint8_t *p;
    bulb pixel;
    int x, y;
    int shift;
    int string;
    int addr;
    scroll_color_map_t *map;

    p = g_scroll_pos;
    for (x = 0; x < g_scroll_width; x++)
    {
        for (y = 0; y < MESSAGE_ROWS; y++)
        {
            shift = 1 << y;
            map_xy_to_string_addr(x, y, &string, &addr);
            map = current_map(p - g_message_bits);

            if (*p & shift)
                pixel.bright = map->bright;
            else
                pixel.bright = 0;

            pixel.stringmask = _BV(string);
            pixel.addrshift = addr << 2;
            pixel.redshift = map->r;
            pixel.greenshift = map->g;
            pixel.blueshift = map->b;

            write_raw_bulbs(1, &pixel);
        }

        if (((++p) - g_message_bits) >= sizeof(g_message_bits))
            p = g_message_bits;

    }
}

void scroll_advance(int howmuch)
{
    while (howmuch-- > 0)
        if ((++g_scroll_pos) - g_message_bits >= sizeof(g_message_bits))
            g_scroll_pos = g_message_bits;
}

void check_scroll()
{
    unsigned long now = millis();

    if (now >= g_next_scroll || now <= SCROLL_INTERVAL)
    {
        g_next_scroll = now + SCROLL_INTERVAL;
        scroll_advance(1);
        scroll_display();
    }

}

/*----------------------------------------------------------------------------
**  init
**      Send the sequence to initialize the strings.
**--------------------------------------------------------------------------*/
void init(unsigned int pause)
{
    int string;
    int addr;
    uint8_t out[4];

    for (addr = 0; addr < ADDR_COUNT; addr++)
    {
        for (string = 0; string < STRING_COUNT; string++)
        {
            BULB_FLAG_ADDRESS(out) = addr;
            if (string < STRING_COUNT - 1)
                BULB_FLAG_ADDRESS(out) |= BULB_FLAG_COMBINE;
            BULB_BLUE_STRING(out) = string;
            BULB_GREEN_RED(out) = 0;
            BULB_BRIGHT(out) = 0;
            process_bulb(out);
        }
        if (pause > 0)
            delay(pause);
    }
}

/*----------------------------------------------------------------------------
**  chase
**      Send a pattern along the strings by way of testing
**--------------------------------------------------------------------------*/
void chase(void)
{
    int string;
    int addr;
    int red, green, blue;
    int bright;
    uint8_t out[4];

    for (addr = 0; addr < ADDR_COUNT; addr++)
    {
        for (bright = MAX_BRIGHT; bright >= 0; bright--)
            for (string = 0; string < STRING_COUNT; string++)
            {
                red = green = blue = 0;
                switch(string)
                {
                    case 0:  red = 13; break;
                    case 1:  green = 13; break;
                    case 2:  blue = 13; break;
                    case 3:  blue = 13; red = 13; break;
                    case 4:  green = 13; red = 13; break;
                    case 5:  blue = 13; green = 13; red = 13; break;
                }
                BULB_FLAG_ADDRESS(out) = addr;
                if (string < STRING_COUNT - 1)
                    BULB_FLAG_ADDRESS(out) |= BULB_FLAG_COMBINE;
                BULB_BLUE_STRING(out) = string | (blue << 4);
                BULB_GREEN_RED(out) = (green << 4) | red;
                BULB_BRIGHT(out) = bright;
                process_bulb(out);
            }
    }
}

/*----------------------------------------------------------------------------
**  flood
**      Flood all the strings with a single color
**--------------------------------------------------------------------------*/
void flood(void)
{
    int string;
    int addr;
    int red, green, blue;
    uint8_t out[4];

    for (addr = 0; addr < ADDR_COUNT; addr++)
    {
        for (string = 0; string < STRING_COUNT; string++)
        {
            red = green = blue = 0;
            switch(string)
            {
                case 0:  red = 13; break;
                case 1:  green = 13; break;
                case 2:  blue = 13; break;
                case 3:  blue = 13; red = 13; break;
                case 4:  green = 13; red = 13; break;
                case 5:  blue = 13; green = 13; red = 13; break;
            }
            BULB_FLAG_ADDRESS(out) = addr;
            if (string < STRING_COUNT - 1)
                BULB_FLAG_ADDRESS(out) |= BULB_FLAG_COMBINE;
            BULB_BLUE_STRING(out) = string | (blue << 4);
            BULB_GREEN_RED(out) = (green << 4) | red;
            BULB_BRIGHT(out) = MAX_BRIGHT;
            process_bulb(out);
        }
    }
}

void flush_sync(void)
{
    uint8_t c;
    do
    {
        c = Serial.read();
    }
    while (c == COMMAND_SYNC);
}


void process_command(const uint8_t *data)
{
    switch(BULB_FLAG_ADDRESS(data))
    {
        case COMMAND_ACK:
            /*----------------------------------------------------------------
            **  Command 1:  Echo.  We just return the next 3 bytes
            **--------------------------------------------------------------*/
            Serial.write(data + 1, 3);
            break;

        case COMMAND_SYNC:
            flush_sync();
            break;

        case COMMAND_INIT:
            init(10);
            break;

        case COMMAND_STATUS:
            Serial.println("AVR LED alive.");
            if (g_scrolling)
                Serial.println("  Displaying builtin message:");
            else
                Serial.println("  NOT Displaying builtin message:");
            Serial.println(MESSAGE_TEXT);
            Serial.print('\0');
            break;

        case COMMAND_CLEAR:
            init(0);
            break;

        case COMMAND_CHASE:
            chase();
            break;

        case COMMAND_FLOOD:
            flood();
            break;

        case COMMAND_SCROLL_DISPLAY_OFF:
            g_scrolling = 0;
            break;

        case COMMAND_SCROLL_DISPLAY:
            g_scrolling = 1;

            if (g_scrolling)
            {
                g_scroll_blueshift = BULB_BLUESHIFT(data);
                g_scroll_greenshift = BULB_GREENSHIFT(data);
                g_scroll_redshift = BULB_REDSHIFT(data);
                g_scroll_bright = BULB_BRIGHT(data);
                scroll_display();
                g_next_scroll = millis() + SCROLL_INTERVAL;
            }
            else
                init(0);
            break;

        default:
            Serial.print("Unknown command: ");
            Serial.println((int) BULB_FLAG_ADDRESS(data));

    }
}

/*----------------------------------------------------------------------------
**  Initial setup
**--------------------------------------------------------------------------*/
void setup()
{
    memset(ring, 0, sizeof(ring));
    Serial.begin(115200);

    /*------------------------------------------------------------------------
    ** Start our out bound processing loop.  DDRB sets the direction of the
    **  PORTB register; we want it all set to output
    **----------------------------------------------------------------------*/
    start_timer1();
    DDRB = ALL_STRINGS_MASK;

    if (g_init_at_start)
    {
        delay(2000);
        init(10);
    }
}



void loop()
{
    uint8_t data[4];

    if (g_scrolling)
        check_scroll();

    /*------------------------------------------------------------------------
    **  Pull a command from the serial port, and execute it
    **----------------------------------------------------------------------*/
    if (Serial.available() >= 4)
    {
        data[0] = Serial.read();
        data[1] = Serial.read();
        data[2] = Serial.read();
        data[3] = Serial.read();

        if (IS_COMMAND(data))
            process_command(data);
        else
            process_bulb(data);
    }
}
