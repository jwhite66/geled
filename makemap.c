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

#ifdef NOCLUE
printf("flags %lx\n", face->face_flags);
printf("style flags %lx\n", face->style_flags);
printf("fixed sizes%d\n", face->num_fixed_sizes);
printf("units_per_EM %d\n", face->units_per_EM);
if (!bs)
    printf("No bitmap sizes available.\n");
else
{
printf("bitmap_size height %d\n", bs->height);
printf("bitmap_size width %d\n", bs->width);
printf("bitmap_size size %ld\n", bs->size);
printf("bitmap_size x_ppem %ld\n", bs->x_ppem);
printf("bitmap_size y_ppem %ld\n", bs->y_ppem);
}
printf("height %d\n", face->height);
printf("ascender %d\n", face->ascender);
printf("descender %d\n", face->descender);
printf("max advance width %d\n", face->max_advance_width);
printf("max advance height %d\n", face->max_advance_height);
printf("bbox.xMin %ld, xMax %ld\n", face->bbox.xMin, face->bbox.xMax);
printf("bbox.yMin %ld, yMax %ld\n", face->bbox.yMin, face->bbox.yMax);
printf("size.metrics.x_ppem %d\n", face->size->metrics.x_ppem);
printf("size.metrics.x_scale %ld\n", face->size->metrics.x_scale);
printf("size.metrics.y_ppem %d\n", face->size->metrics.y_ppem);
printf("size.metrics.y_scale %ld\n", face->size->metrics.y_scale);
printf("size.metrics.height %ld\n", face->size->metrics.height);
printf("size.metrics.ascender %ld\n", face->size->metrics.ascender);
printf("size.metrics.descender %ld\n", face->size->metrics.descender);
printf("size.metrics.max_advance %ld\n", face->size->metrics.max_advance);
#endif

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

    printf("%d: bm rows %d, bm width %d, bm left %d, bm top %d, ", n, face->glyph->bitmap.rows, face->glyph->bitmap.width, face->glyph->bitmap_left, face->glyph->bitmap_top);
#if defined(NOCLUE)
    printf("horibearingx: %ld, ", face->glyph->metrics.horiBearingX);
    printf("horibearingy: %ld, ", face->glyph->metrics.horiBearingY);
    printf("horiadvance: %ld, ", face->glyph->metrics.horiAdvance);
    printf("vertbearingx: %ld, ", face->glyph->metrics.vertBearingX);
    printf("vertbearingy: %ld, ", face->glyph->metrics.vertBearingY);
    printf("vertadvance: %ld\n ", face->glyph->metrics.vertAdvance);

        if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
            printf("ERROR: wrong format?\n");

#endif
        for (y = 0; y < bs->height; y++)
        {
            commentp[y] = comments[y] + x;
        }
        bitp = bits + x;

#if defined(NOCLUE)
    printf("bitmap rows %d, width %d, pitch %d, num_grays %d, pixel_mode %d, palette_mode %d\n", 
            face->glyph->bitmap.rows,
            face->glyph->bitmap.width,
            face->glyph->bitmap.pitch,
            face->glyph->bitmap.num_grays,
            face->glyph->bitmap.pixel_mode,
            face->glyph->bitmap.palette_mode);
#endif
        draw_bitmap(&face->glyph->bitmap, commentp, bitp);

        x += face->glyph->bitmap.width;

        /* Deliberately add 1 column of spaces */
        x++;
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
