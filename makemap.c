#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "led.h"

int bit_set(unsigned char *buffer, int x, int y, int pitch)
{
    buffer += (y * pitch);
    buffer += (x / 8);
    return *buffer & (1 << (8 - (x % 8)));
}

void draw_bitmap(FT_Bitmap*  bitmap, char **comments, char *bits)
{
    int x, y;

    assert(bitmap->pixel_mode == FT_PIXEL_MODE_MONO);

    for (x = 0; x < bitmap->width; x++)
        for (y = 0; y < bitmap->rows; y++)
            if (bit_set(bitmap->buffer, x, y, bitmap->pitch))
            {
                bits[x] |= (1 << (bitmap->rows - y - 1));
                comments[y][x] = 'X';
            }

}


/* [bright|red|green|blue] */
int get_color_command(const char *p, int n, char *colors)
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

    sprintf(colors + strlen(colors), "    { %d, 0x%x, 0x%x, 0x%x, 0x%x },\n", n, bright, red << 4, green << 4, blue << 4);
    q = strchr(p, ']');
    if (q)
        return q - p + 1;

    return 0;
}

int main(int argc, char *argv[])
{
    FT_Library    library;
    FT_Face       face;

    FT_Error      error;

    char*         filename;
    char*         text;

    int           n, num_chars;

    char comments[20][2048];
    char color_map_buf[8096];
    char bits[2048];

    int x, y;

    FT_Bitmap_Size *bs;
    char *commentp[20];
    char *bitp;

    int skip;

    if ( argc != 3 )
    {
        fprintf ( stderr, "usage: %s font sample-text\n", argv[0] );
        exit( 1 );
    }

    filename      = argv[1];                           /* first argument     */
    text          = argv[2];                           /* second argument    */
    num_chars     = strlen( text );

    error = FT_Init_FreeType( &library );              /* initialize library */
    if (error)
    {
        fprintf(stderr, "Error initializing freetype\n");
        exit (1);
    }

    error = FT_New_Face( library, argv[1], 0, &face ); /* create face object */
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


    memset(comments, ' ', sizeof(comments));
    memset(bits, 0, sizeof(bits));
    memset(color_map_buf, 0, sizeof(color_map_buf));

    for ( n = 0, x= 0; n < num_chars; n++ )
    {
        skip = get_color_command(&text[n], x, color_map_buf);
        if (skip > 0)
        {
            n += skip - 1;
            continue;
        }

        /* load glyph image into the slot (erase previous one) */
        error = FT_Load_Char( face, text[n], 0);
        if ( error )
        {
            fprintf(stderr, "Couldn't load '%c'\n", text[n]);
            continue;                 /* ignore errors */
        }

        for (y = 0; y < bs->height; y++)
        {
            commentp[y] = comments[y] + x;
        }
        bitp = bits + x;

        draw_bitmap(&face->glyph->bitmap, commentp, bitp);

        x += face->glyph->bitmap.width;

#if defined(HACK_THIS_OUT_FOR_NOW)
        /* Deliberately add 1 column of spaces */
        /*  Seems like BDF fonts make provision for spaces */
        x++;
#endif
    }

    printf("#define MESSAGE_ROWS %d\n", bs->height);
    printf("#define MESSAGE_TEXT \"%s\"\n", text);

    printf("unsigned char g_message_bits[] = { ");
    for (n = 0; n < x; n++)
        printf("0x%02x%c", (int) bits[n], n == x - 1 ? ' ' : ',');
    printf("};\n");
    for (y = 0; y < bs->height; y++)
    {
        comments[y][x] = 0;
        printf("//%s\n", comments[y]);
    }

    printf("scroll_color_map_t g_scroll_color_map[] = \n{\n%s};\n", color_map_buf);


    FT_Done_Face    ( face );
    FT_Done_FreeType( library );

    return 0;
}

/* EOF */
