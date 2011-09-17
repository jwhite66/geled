// TODO:  Prevent overdrive; writing over back end of prior instructions
// TODO:  Make drive use select to get the ack ASAP

#define MAX_BRIGHT 0xCC
#define MAX_STRINGS 6
#define SAFE_SLICES_AHEAD   50      /* The raw code takes about 380 us, + ~ 100 us per bulb */
#define SLICES_PER_BULB     10

unsigned char ring[100 * 10];  /* 100 10us slices makes 1 ms, let's hold enough for 10 ms */
#define RING_TOP (ring + sizeof(ring))
unsigned char *ringp = ring;

//#define MEASURE_TIMING

ISR(TIMER1_COMPA_vect)
{
    PORTB = *ringp;
    *ringp++ = 0;
    if (ringp >= RING_TOP)
        ringp = ring;
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
    OCR1A = 160;  /* 16,000,000 cycles / sec == 16 cyles / us */
                  /*  We crave one interrupt every 10 us, so 160 */

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

unsigned char *get_ringp(void)
{
    unsigned char sreg;
    unsigned char *p;
    /* Save global interrupt flag */
    sreg = SREG;
    /* Disable interrupts */
    cli();
    p = ringp;
    /* Restore global interrupt flag */
    SREG = sreg;

    return p;
}

#define INC_P(in_p)   { if (++(in_p) == RING_TOP) (in_p) = ring; }

void write_bits(unsigned char **p, unsigned char one_strings, unsigned char zero_strings)
{
    /* Always starts with low */
    **p &= ~(one_strings | zero_strings);
    INC_P(*p);

    /* 20 us of low for one, 20 us of high for off */
    **p &= ~one_strings;
    **p |= zero_strings;

    INC_P(*p);

    /* Always ends with high */
    **p |= (one_strings | zero_strings);
    INC_P(*p);

}

typedef struct bulb_struct
{
    unsigned char string;
    unsigned char addr;
    unsigned char bright;
    unsigned char r;
    unsigned char g;
    unsigned char b;
} bulb;

void write_raw_bulbs(int count, bulb *bulbs)
{
    unsigned char *p;
    int i, j;

    unsigned char all_bulbs;
    unsigned char low_bulbs;
    unsigned char high_bulbs;

    unsigned char *start;

    start = get_ringp();

#if defined(DEBUG)
    {
        char buf[245];
        int k;

        for (k = 0; k < count; k++)
        {
            sprintf(buf, "%02x: str 0x%x|addr 0x%x|bright 0x%x|r 0x%x|g 0x%x|b 0x%x",
                k, bulbs[k].string, bulbs[k].addr, bulbs[k].bright, bulbs[k].r,
                bulbs[k].g, bulbs[k].b);
            Serial.println(buf);
        }
    }
#endif

    /*------------------------------------------------------------------------
    **  Get a point far enough out that we'll finish writing this
    **   before it comes around.
    **----------------------------------------------------------------------*/
    p = start + SAFE_SLICES_AHEAD + (count * SLICES_PER_BULB);
    if (p >= RING_TOP)
        p = ring + (p - RING_TOP);


    all_bulbs = 0;
    for (i = 0; i < count; i++)
        all_bulbs |= bulbs[i].string;

    /* 10 us high */
    *p |= all_bulbs;
    INC_P(p);

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

    /* 30 us low */
    for (i = 0; i < 3; i++)
    {
        *p &= all_bulbs;
        INC_P(p);
    }

#if defined(MEASURE_TIMING)
    {
    unsigned char *end;
    end = get_ringp();
    Serial.print("Wrote bulb in");
    if (end > start)
        Serial.print(end - start);
    else
        Serial.print(end - ring + RING_TOP - start);
    Serial.println(" slices.");
    }
#endif

}

void setup()
{
    memset(ring, 0, sizeof(ring));
    Serial.begin(115200);
    start_timer1();
    DDRB = 0x3F;
}


void loop()
{
    unsigned char b;
    int more_bulbs = 0;
    static bulb bulbs[MAX_STRINGS], *p;
    static int bulb_count = 0;

    if (Serial.available() >= 4)
    {
        p = &bulbs[bulb_count];

        b = Serial.read();
        if (b & 0x80)
        {
            Serial.print((char) Serial.read());
            Serial.print((char) Serial.read());
            Serial.print((char) Serial.read());
            return;
        }
        p->b = b << 4;
        
        b >>=4;
        p->string = _BV(b);
        
        b = Serial.read();
        p->g = b & 0xF0;
        p->r = b << 4;

        b = Serial.read();
        if (b & 0x80)
            more_bulbs = 1;

        p->addr = b << 2;
        p->bright = Serial.read();

        if (++bulb_count >= MAX_STRINGS || (! more_bulbs))
        {
            write_raw_bulbs(bulb_count, bulbs);
            bulb_count = 0;
        }
    }
}
