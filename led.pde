/*----------------------------------------------------------------------------
**  led.pde
**      Driver for running GE Color Effects lights on an Amtel328p board.
**  
**  Features:
**     -  Can drive up to six strings by attaching them to port b (pins 8-13)
**     -  Gets commands via serial port
**
**  License:
**      LGPL v3
**
**--------------------------------------------------------------------------*/
#include "led.h"


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

void write_raw_bulbs(int count, bulb *bulbs)
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

void process_command(const uint8_t *data)
{
    switch(BULB_FLAG_ADDRESS(data))
    {
        case COMMAND_ACK:
            /*----------------------------------------------------------------
            **  Command 1:  Echo.  We just return the next 3 bytes
            **--------------------------------------------------------------*/
            Serial.print(data[1]);
            Serial.print(data[2]);
            Serial.print(data[3]);
            break;

        case COMMAND_INIT:
            init(10);
            break;

        case COMMAND_STATUS:
            Serial.print("cycles per slice ");
            Serial.println((int) CYCLES_PER_SLICE);
            break;

        case COMMAND_CLEAR:
            init(0);
            break;

        case COMMAND_CHASE:
            chase();
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
}



void loop()
{
    uint8_t data[4];

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
