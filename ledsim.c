#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

#include <pthread.h>

#define PIXELS_WIDE     20
#define PIXELS_HIGH     10
#define PIXELS_PER_PIXEL    40

typedef struct
{
    int bright;
    int r;
    int g;
    int b;
} pixel_t;

typedef void (*ledsim_x_callback)(void *h, unsigned long key);
typedef struct xinfo_struct
{
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_window_t window;
    xcb_key_symbols_t *syms;
    ledsim_x_callback x_callback;
    pthread_t thread;

    pixel_t pixels[PIXELS_WIDE][PIXELS_HIGH];
} xinfo_t;


#define LED_HANDLE_T    xinfo_t *

#include "led.h"

static unsigned long map_pixel_to_x_color(xinfo_t *xinfo, pixel_t *pixel)
{
    return
        (((pixel->r << 4) << 16) +
        ((pixel->g << 4) << 8) +
        (pixel->b << 4)) << ((256 - pixel->bright) / 64);
}

static void draw_pixel(xinfo_t *xinfo, int x, int y)
{
    xcb_rectangle_t rectangle =
        { x * PIXELS_PER_PIXEL, y * PIXELS_PER_PIXEL,
            PIXELS_PER_PIXEL, PIXELS_PER_PIXEL};

    xcb_gcontext_t  foreground = xcb_generate_id (xinfo->connection);
    uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t values[2] = { map_pixel_to_x_color(xinfo, &xinfo->pixels[x][y]), 0};

    xcb_create_gc (xinfo->connection, foreground, xinfo->window, mask, values);

    xcb_poly_fill_rectangle (xinfo->connection, xinfo->window, foreground, 1, &rectangle);
    xcb_flush (xinfo->connection);

    xcb_free_gc (xinfo->connection, foreground);
}

static void process_event_thread(xinfo_t *xinfo)
{
    int x, y;
    xcb_generic_event_t *event;
    xcb_key_press_event_t *k;
    xcb_keysym_t sym;

    while ((event = xcb_wait_for_event (xinfo->connection)))
    {
        switch (event->response_type & ~0x80)
        {
            case XCB_EXPOSE:
                for (x = 0; x < PIXELS_WIDE; x++)
                    for (y = 0; y < PIXELS_HIGH; y++)
                        draw_pixel(xinfo, x, y);
                break;

            case XCB_KEY_PRESS:
                if (xinfo->x_callback)
                {
                    k = (xcb_key_press_event_t *)event;
                    sym = xcb_key_press_lookup_keysym(xinfo->syms, k, 0);
                    (*xinfo->x_callback)(xinfo, sym);
                }
                break;

            default:
                break;
        }
    }
}


LED_HANDLE_T led_init(void)
{
    pthread_attr_t attr;
    xinfo_t *xinfo = malloc(sizeof(*xinfo));
    memset(xinfo, 0, sizeof(*xinfo));

    /* Open the connection to the X server */
    xinfo->connection = xcb_connect (NULL, NULL);

    /* Get the first screen */
    const xcb_setup_t      *setup  = xcb_get_setup (xinfo->connection);
    xcb_screen_iterator_t   iter   = xcb_setup_roots_iterator (setup);
    xinfo->screen = iter.data;

    /* Create the window */
    xinfo->window = xcb_generate_id (xinfo->connection);
    uint32_t        mask       = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t        values[2]  = { xinfo->screen->white_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS };

    xcb_create_window (xinfo->connection,             /* Connection          */
                       XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
                       xinfo->window,                 /* window Id           */
                       xinfo->screen->root,           /* parent window       */
                       0, 0,                          /* x, y                */
                       PIXELS_WIDE * PIXELS_PER_PIXEL,
                       PIXELS_HIGH * PIXELS_PER_PIXEL,
                       10,                            /* border_width        */
                       XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
                       xinfo->screen->root_visual,    /* visual              */
                       mask, values);

    /* Map the window on the screen */
    xcb_map_window (xinfo->connection, xinfo->window);

    xcb_flush (xinfo->connection);

    xinfo->syms = xcb_key_symbols_alloc(xinfo->connection);

    pthread_attr_init(&attr);
    pthread_create(&xinfo->thread, &attr, (void * (*)(void *))process_event_thread, xinfo);

    return xinfo;
}

void led_term(LED_HANDLE_T h)
{
    pthread_cancel(h->thread);
    xcb_key_symbols_free(h->syms);
    xcb_disconnect (h->connection);
    free(h);
}

void led_set_pixel(LED_HANDLE_T h, int x, int y, int bright, int r, int g, int b)
{
    pixel_t pixel = { bright, r, g, b };
    if (x < 0 || y < 0 || x >= PIXELS_WIDE ||y >= PIXELS_HIGH)
    {
        fprintf(stderr, "Error:  attempt made to set an invalid pixel spot %d, %d\n", x, y);
        return;
    }

    h->pixels[x][y] = pixel;
    draw_pixel(h, x, y);
}

void led_get_size(LED_HANDLE_T h, int *wide, int *high)
{
    *wide = PIXELS_WIDE;
    *high = PIXELS_HIGH;
}

void ledsim_set_x_callback(LED_HANDLE_T h, ledsim_x_callback x_callback)
{
    h->x_callback = x_callback;
}

void ledsim_join(LED_HANDLE_T h)
{
    pthread_join(h->thread, NULL);
}
