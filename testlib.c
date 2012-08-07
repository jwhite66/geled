#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "led.h"

int main(int argc, char *argv[])
{
    LED_HANDLE_T h;
    int x, y, i, j;
    long count = 0;

    if (argc <= 1)
    {
        fprintf(stderr, "Specify number of bulbs to set\n");
        return 1;
    }

    h = led_init();
    if (h)
    {
        led_get_size(h, &x, &y);
        printf("Size is %d x %d\n", x, y);

        do
        {
            for (i = 0; i < x; i++)
                for (j = 0; j < y; j++)
                {
                    led_set_pixel(h, i, j, MAX_BRIGHT, 0xf, 0, 0);
                    led_set_pixel(h, i, j, 0, 0, 0, 0);
                    count += 2;
                }
        }
        while (count < atol(argv[1]));

        led_term(h);
    }

    return 0;
}
