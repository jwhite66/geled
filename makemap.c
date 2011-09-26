#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <ft2build.h>
#include FT_FREETYPE_H

void draw_bitmap(FT_Bitmap*  bitmap, char **comments, char *bits)
{
    int x, y;

    for (x = 0; x < bitmap->width; x++)
        for (y = 0; y < bitmap->rows; y++)
            if (bitmap->buffer[y * bitmap->width + x])
            {
                bits[x] |= (1 << (bitmap->rows - y - 1));
                comments[y][x] = 'X';
            }

}


int
main( int     argc,
      char**  argv )
{
  FT_Library    library;
  FT_Face       face;

  FT_Error      error;

  char*         filename;
  char*         text;

  int           n, num_chars;

    char comments[20][2048];
    char bits[2048];
    int rows = 0;

    int x, y;

    char *commentp[20];
    char *bitp;

  if ( argc != 3 )
  {
    fprintf ( stderr, "usage: %s font sample-text\n", argv[0] );
    exit( 1 );
  }

  filename      = argv[1];                           /* first argument     */
  text          = argv[2];                           /* second argument    */
  num_chars     = strlen( text );

  error = FT_Init_FreeType( &library );              /* initialize library */
  /* error handling omitted */

  error = FT_New_Face( library, argv[1], 0, &face ); /* create face object */
  /* error handling omitted */

#ifdef NOCLUE
printf("fixed sizes%d\n", face->num_fixed_sizes);
printf("units_per_EM %d\n", face->units_per_EM);
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
#endif

/* 1024 works for the 5x7 font.  Go figure */
  error = FT_Set_Char_Size( face, 512, 0, 72, 0 );                /* set character size */

    memset(comments, ' ', sizeof(comments));
    memset(bits, 0, sizeof(bits));

  for ( n = 0, x= 0; n < num_chars; n++ )
  {
    /* load glyph image into the slot (erase previous one) */
    error = FT_Load_Char( face, text[n], FT_LOAD_RENDER);
    if ( error )
      continue;                 /* ignore errors */

//printf("%d: max %d, current %d\n", n, rows, face->glyph->bitmap.rows);
    if (n > 0)
        assert(rows >= face->glyph->bitmap.rows);
    else
        rows = face->glyph->bitmap.rows;

    assert(x + face->glyph->bitmap.width < sizeof(bits));
    for (y = 0; y < rows; y++)
    {
        commentp[y] = comments[y] + x;
    }
    bitp = bits + x;

    draw_bitmap(&face->glyph->bitmap, commentp, bitp);

    x += face->glyph->bitmap.width;

    /* Deliberately add 1 column of spaces */
    x++;
  }

    printf("#define MESSAGE_ROWS %d\n", rows);
    printf("//%s\n", text);

    printf("unsigned char g_message_bits[] = { ");
    for (n = 0; n < x; n++)
        printf("0x%02x%c", (int) bits[n], n == x - 1 ? ' ' : ',');
    printf("};\n");
    for (y = 0; y < rows; y++)
    {
        comments[y][x] = 0;
        printf("//%s\n", comments[y]);
    }

  FT_Done_Face    ( face );
  FT_Done_FreeType( library );

  return 0;
}

/* EOF */
