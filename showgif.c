#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <gif_lib.h>


#if defined(SIMULATOR)
#include <X11/keysym.h>
#endif

#include "led.h"


int g_quit = 0;

int g_wide = 0;
int g_high = 0;


#if defined(SIMULATOR)
void my_callback(LED_HANDLE_T h, unsigned long key)
{
    if (key == 'q')
        g_quit++;
}
#endif


void usage(char *argv0)
{
    fprintf(stderr, "%s fname [repeat_count]\n", argv0);
    fprintf(stderr, "  Where fname is the name of .gif file with optional animations and\n");
    fprintf(stderr, "  the optional repeat_count specifies the number of times to show it, with 0 being infinite.\n");
    fprintf(stderr, "Note that the display will truncate at %d high, and %d wide.\n", g_high, g_wide);
}

int find_delay(SavedImage *img)
{
    int i;
    int delay = 0;

    for (i = 0; i < img->ExtensionBlockCount; i++)
    {
        ExtensionBlock *e = &img->ExtensionBlocks[i];
        if (e->Function == GRAPHICS_EXT_FUNC_CODE)
        {
            unsigned char *d = (unsigned char *) e->Bytes + 1;
            return *d + ((*(d+1)) * 256);
        }
    }

    return delay;
}

void show_one_image(LED_HANDLE_T p, ColorMapObject *colors, SavedImage *img)
{
    int x;
    int y;
    unsigned char *bits;
    int delay;

    bits = img->RasterBits;
    for (y = 0; y < img->ImageDesc.Height; y++)
        for (x = 0; x < img->ImageDesc.Width; x++)
        {
            GifColorType *ctype;
            int r, g, b;

            ctype = &colors->Colors[*bits++];
            r = ctype->Red;
            g = ctype->Green;
            b = ctype->Blue;

            if (x < g_wide && y < g_high)
                led_set_pixel(p, x, y, MAX_BRIGHT, r, g, b);
        }

    delay = find_delay(img);
    usleep(delay * 10000);
}

void show_gif(LED_HANDLE_T p, GifFileType *f)
{
    int i;
    for (i = 0; i < f->ImageCount; i++)
    {
        show_one_image(p, f->SavedImages[i].ImageDesc.ColorMap ? 
                            f->SavedImages[i].ImageDesc.ColorMap : f->SColorMap,
                            &f->SavedImages[i]);
    }
}


int main (int argc, char *argv[])
{
    LED_HANDLE_T p;
    GifFileType *f;
    int repeat_count = 0;
    int count = 0;


    p = led_init();
#if defined(SIMULATOR)
    ledsim_set_x_callback(p, my_callback);
#endif

    led_get_size(p, &g_wide, &g_high);

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, "Error:  supply the name of a .gif file to display.\n");
        usage(argv[0]);
        g_quit++;
    }

    if (argc == 3)
        repeat_count = atoi(argv[2]);

    f = DGifOpenFileName(argv[1]);
    if (! f)
    {
        fprintf(stderr, "Error opening %s\n", argv[1]);
        usage(argv[0]);
        g_quit++;
    }
    else
        if (DGifSlurp(f) != GIF_OK)
        {
            fprintf(stderr, "Error reading rest of %s\n", argv[1]);
            g_quit++;
        }

    while (! g_quit && (repeat_count == 0 || count < repeat_count))
    {
        show_gif(p, f);
        count++;
    }

    led_term(p);

    return 0;
}
