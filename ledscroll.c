#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "led.h"

#define     MAX_ROWS    30
#define     MAX_COLS    (1024 * 8)

typedef struct
{
    int bright;
    int r;
    int g;
    int b;
} pixel_t;

typedef pixel_t bits_t[MAX_COLS][MAX_ROWS];

int bit_set(unsigned char *buffer, int x, int y, int pitch)
{
    int rc;
    buffer += (y * pitch);
    buffer += (x / 8);
    rc = *buffer & (1 << (8 - (x % 8)));
printf("checking x %d, y %d, pitch %d, buffer 0x%x: %d\n", x, y, pitch, *buffer, rc);
    return rc;
}

void draw_bitmap(FT_Bitmap*  bitmap, int originx, int originy, bits_t bits, pixel_t *pixel)
{
    int x, y;
    static pixel_t blank_pixel = { 0, 0, 0, 0 };

    for (x = 0; x < bitmap->width; x++)
        for (y = 0; y < bitmap->rows; y++)
            if (bit_set(bitmap->buffer, x, y, bitmap->pitch))
                bits[x + originx][y + originy] = *pixel;
            else
                bits[x + originx][y + originy] = blank_pixel;
}


/* [bright|red|green|blue] */
int get_color_command(const char *p, pixel_t *pixel)
{
    int bright = MAX_BRIGHT;
    int red = 0;
    int green = 0;
    int blue = 0;
    char *q;

    if (sscanf(p, "[%x|%x|%x|%x]", &bright, &red, &green, &blue) == 4)
        ;
    else if (strncasecmp(p, "[red]", 5) == 0)
    {
        bright = MAX_BRIGHT;
        red = 0xf;
    }
    else if (strncasecmp(p, "[green]", 7) == 0)
    {
        bright = MAX_BRIGHT;
        green = 0xf;
    }
    else if (strncasecmp(p, "[blue]", 6) == 0)
    {
        bright = MAX_BRIGHT;
        blue = 0xf;
    }
    else if (strncasecmp(p, "[purple]", 8) == 0)
    {
        bright = MAX_BRIGHT;
        red = 13;
        blue = 13;
    }
    else if (strncasecmp(p, "[orange]", 8) == 0)
    {
        bright = MAX_BRIGHT;
        red = 13;
        green = 1;
    }
    else if (strncasecmp(p, "[yellow]", 8) == 0)
    {
        bright = MAX_BRIGHT;
        red = 13;
        green = 6;
    }
    else if (*p == '[')
        fprintf(stderr, "Huh? [%s]\n", p);
    else
        return 0;

    q = strchr(p, ']');
    if (q)
    {
        pixel->bright = bright;
        pixel->r = red;
        pixel->g = green;
        pixel->b = blue;

        return q - p + 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    FT_Library    library;
    FT_Face       face;

    FT_Error      error;

    char*         p;

    pixel_t       pixel = { MAX_BRIGHT, MAX_RGB, 0, 0 };
    bits_t bits;
    int x, y;
    int skip;

    int originx;
    int max_x;
    int base_y = 0;
    int wide, high;

    FT_Bitmap_Size *bs;

    LED_HANDLE_T h;

    if ( argc != 3 )
    {
        fprintf ( stderr, "usage: %s font sample-text\n", argv[0] );
        exit( 1 );
    }

    error = FT_Init_FreeType(&library);
    if (error)
    {
        fprintf(stderr, "Error initializing freetype\n");
        exit (1);
    }
    error = FT_New_Face(library, argv[1], 0, &face);
    if (error)
    {
        fprintf(stderr, "Error making face out of %s\n", argv[1]);
        exit (2);
    }
    bs = face->available_sizes;
    if (! bs)
    {
        fprintf(stderr, "Error:  this only works with bitmap fonts.  Try to find a .bdf file.\n");
        exit (1);
    }

    memset(bits, 0, sizeof(bits));

    for ( p = argv[2], x = 0, y = 0; *p; p++)
    {
        skip = get_color_command(p, &pixel);
        if (skip > 0)
        {
            p += (skip - 1);
            continue;
        }

        /* load glyph image into the slot (erase previous one) */
        error = FT_Load_Char( face, *p, FT_LOAD_RENDER);
        if ( error )
        {
            fprintf(stderr, "Couldn't load '%c'\n", *p);
            continue;                 /* ignore errors */
        }

        draw_bitmap(&face->glyph->bitmap, x, y, bits, &pixel);

        x += face->glyph->bitmap.width;

#if defined(HACK_THIS_OUT_FOR_NOW)
        /* Deliberately add 1 column of spaces */
        /*  Seems like BDF fonts make provision for spaces */
        x++;
#endif
    }

    max_x = x;

    h = led_init();
    if (h)
    {
        led_get_size(h, &wide, &high);

        if (bs->height < high)
            base_y = (high - bs->height) / 2;

        originx = 0;
        while (1)
        {
            printf("tick\n");
            if (originx >= max_x)
                originx = 0;

            for (x = 0; x < wide; x++)
                for (y = 0; y < bs->height; y++)
                {
                    pixel_t *pixel = &bits[(originx + x) % max_x][y];
                    led_set_pixel(h, x, y + base_y, pixel->bright, pixel->r, pixel->g, pixel->b);
                }

            originx++;

            usleep(200000);
        }
    }

    FT_Done_Face    ( face );
    FT_Done_FreeType( library );

    return 0;
}

/* EOF */
