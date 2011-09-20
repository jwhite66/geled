#include "led.h"

unsigned char ring[SLICES_TO_SHOW_BULB * 10];
const static unsigned char *ring_fence = ring + sizeof(ring);

#define INCREMENT_RING_PTR(p) { if ((++(p)) == ring_fence) (p) = ring; }

unsigned char *readp = ring;
unsigned char *writep = ring;


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
    unsigned char sreg;
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
    unsigned char sreg;
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

void *set_writep(unsigned char *p)
{
    unsigned char sreg;
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
    unsigned char sreg;
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

void write_bits(unsigned char **p, unsigned char one_strings, unsigned char zero_strings)
{
    /* Always starts with low */
    **p &= ~(one_strings | zero_strings);
    INCREMENT_RING_PTR(*p);

    /* 2 slices of low for one, 2 of high for off */
    **p &= ~one_strings;
    **p |= zero_strings;

    INCREMENT_RING_PTR(*p);

    /* Always ends with high */
    **p |= (one_strings | zero_strings);
    INCREMENT_RING_PTR(*p);

}

void write_raw_bulbs(int count, bulb *bulbs)
{
    unsigned char *p;
    int i, j;
    int a;

    unsigned char all_bulbs;
    unsigned char low_bulbs;
    unsigned char high_bulbs;

    unsigned char *start;

    while (available() < SLICES_TO_SHOW_BULB)
        ;

    p = (unsigned char *) writep;

    all_bulbs = 0;
    for (i = 0; i < count; i++)
        all_bulbs |= bulbs[i].string;

    /* start indicator high  */
    for (i = 0; i < START_SLICES; i++)
    {
        *p |= all_bulbs;
        INCREMENT_RING_PTR(p);
    }

    for (i = 6; i; i--)
    {
        high_bulbs = low_bulbs = 0;
        for (j = 0; j < count; j++)
        {
            if (bulbs[j].addr & 0x80)
                high_bulbs |= bulbs[j].string;
            else
                low_bulbs  |= bulbs[j].string;
            bulbs[j].addr <<= 1;
        }
        write_bits(&p, high_bulbs, low_bulbs);
    }
    for (i = 8; i; i--)
    {
        high_bulbs = low_bulbs = 0;
        for (j = 0; j < count; j++)
        {
            if (bulbs[j].bright & 0x80)
                high_bulbs |= bulbs[j].string;
            else
                low_bulbs  |= bulbs[j].string;
            bulbs[j].bright <<= 1;
        }
        write_bits(&p, high_bulbs, low_bulbs);
    }

    for (i = 4; i; i--)
    {
        high_bulbs = low_bulbs = 0;
        for (j = 0; j < count; j++)
        {
            if (bulbs[j].b & 0x80)
                high_bulbs |= bulbs[j].string;
            else
                low_bulbs  |= bulbs[j].string;
            bulbs[j].b <<= 1;
        }
        write_bits(&p, high_bulbs, low_bulbs);
    }

    for (i = 4; i; i--)
    {
        high_bulbs = low_bulbs = 0;
        for (j = 0; j < count; j++)
        {
            if (bulbs[j].g & 0x80)
                high_bulbs |= bulbs[j].string;
            else
                low_bulbs  |= bulbs[j].string;
            bulbs[j].g <<= 1;
        }
        write_bits(&p, high_bulbs, low_bulbs);
    }

    for (i = 4; i; i--)
    {
        high_bulbs = low_bulbs = 0;
        for (j = 0; j < count; j++)
        {
            if (bulbs[j].r & 0x80)
                high_bulbs |= bulbs[j].string;
            else
                low_bulbs  |= bulbs[j].string;
            bulbs[j].r <<= 1;
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

static bulb bulbs[STRING_COUNT];
static int bulb_count = 0;

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

/*--------------------------------------------------------------------
**  All other commands are now giving us a bulb to light.
**      The general structure is:
**   cmd str(3) blue(4)  |  g(4) r(4)  |  more_bulbs unused addr(6) | bright
**  Note that we store all values left shifted, so they can
**    just be shifted out for high performance.
**------------------------------------------------------------------*/
void process_bulb(unsigned char data[4])
{
    int more_bulbs = 0;
    bulb *p;
    int a;

    p = &bulbs[bulb_count];

    p->b = data[0] << 4;   /* Grab blue */

    data[0] >>=4;          /* Grab string # */
    p->string = _BV(data[0]);

    /*  Green on the high, red on the low */
    p->g = data[1] & 0xF0;
    p->r = data[1] << 4;

    /*--------------------------------------------------------------------
    **  We can set up to STRING_COUNT worth of bulbs to write for each
    **      time, although note that you cannot write the same string
    **      twice; it'll just fail.
    **------------------------------------------------------------------*/
    if (data[2] & 0x80)
        more_bulbs = 1;

    p->addr = data[2] << 2;

    /* Bright is nice and simple, but let's protect things */
    p->bright = data[3];
    if (p->bright > MAX_BRIGHT)
        p->bright = MAX_BRIGHT;

    if (++bulb_count >= STRING_COUNT || (! more_bulbs))
    {
        write_raw_bulbs(bulb_count, bulbs);
        bulb_count = 0;
    }
}


void loop()
{
    unsigned char data[4];
    int i;

    /*------------------------------------------------------------------------
    **  Pull a command from the serial port, and execute it
    **----------------------------------------------------------------------*/
    if (Serial.available() >= 4)
    {
        data[0] = Serial.read();
        data[1] = Serial.read();
        data[2] = Serial.read();
        data[3] = Serial.read();

        /*--------------------------------------------------------------------
        **  Command 1:  Echo.  We just return the next 3 bytes
        **------------------------------------------------------------------*/
        if (data[0] == 0x80)
        {
            Serial.print(data[1]);
            Serial.print(data[2]);
            Serial.print(data[3]);
            return;
        }

        else if (data[0] == 0x81)
        {
            Serial.print("cycles per slice ");
            Serial.println((int) CYCLES_PER_SLICE);
            return;
        }

        else if (data[0] == 0x82)
        {
            unsigned char fakedata[4];
            int i, r, g, b;
            for (i = 0; i < 36; i++)
            {
                fakedata[0] = 13;
                fakedata[1] = 0;
                fakedata[2] = i % 36;
                fakedata[3] = 0xcc;
                process_bulb(fakedata);

                for (r = 0; r <= 13; r++)
                    for (g = 0; g <= 13; g++)
                        for (b = 0; b <= 13; b++)
                        {
                            fakedata[0] = b;
                            fakedata[1] = g << 4 | r;
                            fakedata[2] = i % 36;
                            fakedata[3] = 0xcc;
                            process_bulb(fakedata);
                        }
            }
        }

        else if (data[0] == 0x83)
        {
            unsigned char fakedata[4];
            for (i = 0; i < 36; i++)
            {
                fakedata[0] = 0;
                fakedata[1] = 0;
                fakedata[2] = i % 36;
                fakedata[3] = 0;
                process_bulb(fakedata);
            }
            while (available() < sizeof(ring))
                ;

            Serial.println("Wrote zero");
        }

        else if (data[0] == 0x84)
        {
            unsigned char fakedata[4];
            fakedata[0] = 0;
            fakedata[1] = 13;
            fakedata[2] = 35;
            fakedata[3] = MAX_BRIGHT;
            for (i = 0; i < 6000; i++)
            {
                process_bulb(fakedata);
            }

            while (available() < sizeof(ring))
                ;

            Serial.println("Wrote 6000 red");
        }

        else
            process_bulb(data);
    }
}
