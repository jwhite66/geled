#include <unistd.h>
#include <stdio.h>

#include "led.h"

int main(int argc, char *argv[])
{
    LED_HANDLE_T h;
    int x, y, i, j;

    h = led_init();
    if (h)
    {
        led_get_size(h, &x, &y);
        printf("Size is %d x %d\n", x, y);

        for (i = 0; i < x; i++)
            for (j = 0; j < y; j++)
            {
                led_set_pixel(h, i, j, MAX_BRIGHT, 0xf, 0, 0);
                usleep(100000);
                led_set_pixel(h, i, j, 0, 0, 0, 0);
            }

        led_term(h);
    }

    return 0;
}
