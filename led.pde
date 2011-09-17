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


void write_bulb(int string, unsigned char addr, unsigned char bright, unsigned char r, unsigned char g, unsigned char b)
{
    unsigned char *p;
    unsigned int rgb;
    int i;

    unsigned char *start;

    start = get_ringp();

    /*------------------------------------------------------------------------
    **  Get a point far enough out that we'll finish writing this
    **   before it comes around.
    **----------------------------------------------------------------------*/
    p = start + SAFE_SLICES_AHEAD;
    if (p >= RING_TOP)
        p = ring + (p - RING_TOP);

    /* 10 us high */
    *p |= _BV(string);
    INC_P(p);

    addr <<= 2;
    if (bright > MAX_BRIGHT)
        bright = MAX_BRIGHT;
    rgb = (b << 12) | (g << 8) | (r << 4);
    for (i = 6; i; i--)
    {
        write_bits(&p, addr & 0x80 ? _BV(string) : 0, addr & 0x80 ? 0 : _BV(string));
        addr <<= 1;
    }
    for (i = 8; i; i--)
    {
        write_bits(&p, bright & 0x80 ? _BV(string) : 0, bright & 0x80 ? 0 : _BV(string));
        bright <<= 1;
    }
    for (i = 12; i; i--)
    {
        write_bits(&p, rgb & 0x8000 ? _BV(string) : 0, rgb & 0x8000 ? 0 : _BV(string));
        rgb <<= 1;
    }

    /* 30 us low */
    for (i = 0; i < 3; i++)
    {
        *p &= (~_BV(string)); 
        INC_P(p);
    }

#if defined(MEASURE_TIMING)
    {
    unsigned char *end;
    end = get_ringp();
    Serial.print("Wrote bulb ");
    Serial.print(addr);
    Serial.print(" in ");
    if (end > start)
        Serial.print(end - start);
    else
        Serial.print(end - ring + RING_TOP - start);
    Serial.println(" slices.");
    }
#endif

}

void establishContact()
{
    while (Serial.available() <= 0)
    {
        Serial.print('A', BYTE);   // send a capital A
        delay(300);
    }
    Serial.read();
    Serial.println("Ready for commands");
}

void setup()
{
    memset(ring, 0, sizeof(ring));
    Serial.begin(115200);
    start_timer1();
    DDRB = 0x3F;
    //establishContact();  // send a byte to establish contact until receiver responds
}


void loop()
{
    unsigned char b;
    int last_bulb = 1;
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
            last_bulb = 0;

        p->addr = b << 2;
        p->bright = Serial.read();

        if (++bulb_count >= MAX_STRINGS || last_bulb)
        {
            write_raw_bulbs(bulb_count, bulbs);
            bulb_count = 0;
        }
    }
}

void old_loop()
{
    int rc;
    static int target_bulb = 0;
    int i;

    if (Serial.available() > 0)
    {
        int inByte = 0;

        inByte = Serial.read();

        if (inByte == 's')
        {
            Serial.println("Issuing setup sequence");
            for (i = 0; i < 36; i++)
            {
                write_bulb(0, i, MAX_BRIGHT, 0, 0, 0);
                delay(100);
            }
            Serial.println("Done.");
        }
        else if (inByte == 'b')
        {
            Serial.println("blue");
            write_bulb(0, target_bulb, MAX_BRIGHT, 0, 0, 15);
        }
        else if (inByte == 'r')
        {
            Serial.println("red");
            write_bulb(1, target_bulb, MAX_BRIGHT, 15, 0, 0);
        }
        else if (inByte == 'g')
        {
            Serial.println("green");
            write_bulb(0, target_bulb, MAX_BRIGHT, 0, 15, 0);
        }
        else if (inByte == 'o')
        {
            Serial.println("off");
            write_bulb(0, target_bulb, MAX_BRIGHT, 0, 0, 0);
        }
        else if (inByte == 'd')
        {
            Serial.println("dim");
            write_bulb(0, target_bulb, MAX_BRIGHT / 2, 15, 15, 15);
        }
        else if (inByte == 'a')
        {
            bulb testme[3];
            Serial.println("testing");
            testme[0].string = _BV(2);
            testme[0].addr = target_bulb << 2;
            testme[0].bright = MAX_BRIGHT;
            testme[0].r = 15 << 4;
            testme[0].b = 15 << 4;
            testme[0].g = 0;
            testme[1].string = _BV(1);
            testme[1].addr = target_bulb << 2;
            testme[1].bright = MAX_BRIGHT;
            testme[1].r = 0;
            testme[1].b = 15 << 4;
            testme[1].g = 15 << 4;;
            testme[2].string = _BV(0);
            testme[2].addr = target_bulb << 2;
            testme[2].bright = MAX_BRIGHT;
            testme[2].r = 0;
            testme[2].b = 15 << 4;
            testme[2].g = 15 << 4;;
            write_raw_bulbs(3, testme);
        }
        else if (inByte >= '0' && inByte <= '9')
        {
            target_bulb = (inByte - '0');
            Serial.print("target bulb ");
            Serial.println(target_bulb);
        }
        else
        {
            Serial.print("lighting bulb ");
            Serial.println(target_bulb);
            write_bulb(0, target_bulb, MAX_BRIGHT, 15, 15, 15);
        }
    }
}
