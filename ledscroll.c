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

void draw_bitmap(FT_Bitmap*  bitmap, int originx, int originy, bits_t bits, pixel_t *pixel)
{
    int x, y;
    static pixel_t blank_pixel = { 0, 0, 0, 0 };

    for (x = 0; x < bitmap->width; x++)
        for (y = 0; y < bitmap->rows; y++)
            if (bitmap->buffer[y * bitmap->width + x])
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
    int rows = 0;
    int x, y;
    int skip;

    int originx;
    int max_x;
    int max_y = 0;
    int base_y = 0;
    int wide, high;

    LED_HANDLE_T h;

    if ( argc != 3 )
    {
        fprintf ( stderr, "usage: %s font sample-text\n", argv[0] );
        exit( 1 );
    }

    error = FT_Init_FreeType(&library);
    error = FT_New_Face(library, argv[1], 0, &face);

    /* 1024 works for the 5x7 font.  Go figure */
    //if (memcmp(argv[1], "elegant", 7) == 0)
        error = FT_Set_Char_Size( face, 512, 0, 72, 0 );
    //else
        //error = FT_Set_Char_Size( face, 1024, 0, 72, 0 );

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
          continue;                 /* ignore errors */

        if (rows <= 0)
            rows = face->glyph->bitmap.rows;

        if (rows > max_y)
            max_y = rows;


        assert(x + face->glyph->bitmap.width < MAX_COLS);

        draw_bitmap(&face->glyph->bitmap, x, y, bits, &pixel);

        x += face->glyph->bitmap.width;

        /* Deliberately add 1 column of spaces */
        x++;
    }

    max_x = x;

    FT_Done_Face    ( face );
    FT_Done_FreeType( library );

    h = led_init();
    if (h)
    {
        led_get_size(h, &wide, &high);

        if (max_y < high)
            base_y = (high - max_y) / 2;

        originx = 0;
        while (1)
        {
            printf("tick\n");
            if (originx >= max_x)
                originx = 0;

            for (x = 0; x < wide; x++)
                for (y = 0; y < max_y; y++)
                {
                    pixel_t *pixel = &bits[(originx + x) % max_x][y];
                    led_set_pixel(h, x, y + base_y, pixel->bright, pixel->r, pixel->g, pixel->b);
                }

            originx++;

            usleep(200000);
        }
    }
    return 0;
}

/* EOF */
