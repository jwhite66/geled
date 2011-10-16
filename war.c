#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <pthread.h>
#include <errno.h>


#include <X11/keysym.h>

#include "led.h"


typedef struct
{
    int bright;
    int r;
    int g;
    int b;
} pixel_t;

#define ORIENT_NORTH   0
#define ORIENT_EAST    1
#define ORIENT_SOUTH   2
#define ORIENT_WEST    3

#define X_ORIENT(o) ((o) == ORIENT_EAST ? 1 : (o) == ORIENT_WEST ? -1 : 0)
#define Y_ORIENT(o) ((o) == ORIENT_SOUTH ? 1 : (o) == ORIENT_NORTH ? -1 : 0)

typedef struct
{
    double x;
    double y;
    int orientation;
    double dx;
    double dy;
    int id;
    int hits;
} dude_t;

typedef struct
{
    double x;
    double y;
    double dx;
    double dy;
    int id;
} bullet_t;

typedef struct
{
    int x;
    int y;
    int type;
    int tick;
} explosion_t;

#define BLUE_DUDE   0
#define RED_DUDE    1

dude_t g_dudes[2];

int g_quit = 0;

#define MAX_BULLETS 10
#define BULLET_SPEED 0.2

bullet_t g_bullets[MAX_BULLETS];
int g_bullet_count = 0;

#define MAX_EXPLOSIONS 10
#define EXPLOSION_RADIUS 1
#define EXPLOSION_LIFE_CYCLE 10
explosion_t g_explosions[MAX_EXPLOSIONS];
int g_explosion_count = 0;

int g_wide = 0;
int g_high = 0;

double wrap_x(double x, double dx)
{
    x += dx;
    while (((int) x) >= g_wide)
        x -= g_wide;
    while ( x < 0.0)
        x += g_wide;

    return x;
}

double wrap_y(double y, double dy)
{
    y += dy;
    while (((int) y) >= g_high)
        y -= g_high;
    while (y < 0.0)
        y += g_high;

    return y;
}


pixel_t *get_dude_color(dude_t *dude)
{
    static pixel_t colors[2][3] =
    {
        {
            { 0xc0, 0, 0, 0xf },
            { 0xc0, 5, 0, 0xf },
            { 0xc0, 0xf, 0, 5 }
        },
        {
            { 0xc0, 0xf, 0, 0 },
            { 0xc0, 0xf, 0, 5 },
            { 0xc0, 5, 0, 0xf }
        }
    };

    return (&colors[dude->id][dude->hits]);
}

pixel_t *get_bullet_color(bullet_t *bullet)
{
    static pixel_t colors[2] =
    {
        { 0xc0, 0, 0, 0xf },
        { 0xc0, 0xf, 0, 0 }
    };

    return (&colors[bullet->id]);
}

void erase_dude(LED_HANDLE_T h, dude_t *dude)
{
    led_set_pixel(h, (int) dude->x, (int) dude->y, 0, 0, 0, 0);
    led_set_pixel(h, (int) wrap_x(((int) dude->x), X_ORIENT(dude->orientation)),
                         (int) wrap_y(((int) dude->y), Y_ORIENT(dude->orientation)),
                         0, 0, 0, 0);
}

void draw_dude(LED_HANDLE_T h, dude_t *dude)
{
    pixel_t *pixel = get_dude_color(dude);
    led_set_pixel(h, (int) dude->x, (int) dude->y,
            pixel->bright, pixel->r,
            pixel->g, pixel->b);
    led_set_pixel(h, (int) wrap_x(((int) dude->x), X_ORIENT(dude->orientation)),
                         (int) wrap_y(((int) dude->y), Y_ORIENT(dude->orientation)),
                         0x30, pixel->r, pixel->g, pixel->b);
}

void draw_dudes(LED_HANDLE_T h)
{
    int i;
    for (i = 0; i < sizeof(g_dudes) / sizeof(g_dudes[0]); i++)
        draw_dude(h, &g_dudes[i]);
}

void move_dude(LED_HANDLE_T h, dude_t *dude)
{
    double newx, newy;

    newx = wrap_x(dude->x, dude->dx);
    newy = wrap_y(dude->y, dude->dy);

    if ((int) newx != (int) dude->x ||
        (int) newy != (int) dude->y)
    {
        erase_dude(h, dude);
        dude->x = newx;
        dude->y = newy;
        draw_dude(h, dude);
    }
    else
    {
        dude->x = newx;
        dude->y = newy;
    }
}

void accelerate_dude(LED_HANDLE_T h, dude_t *dude)
{
    if (dude->orientation == ORIENT_NORTH)
        dude->dy -= .1;
    if (dude->orientation == ORIENT_SOUTH)
        dude->dy += .1;
    if (dude->orientation == ORIENT_EAST)
        dude->dx += .1;
    if (dude->orientation == ORIENT_WEST)
        dude->dx -= .1;
}

void erase_bullet(LED_HANDLE_T h, bullet_t *bullet)
{
    led_set_pixel(h, (int) bullet->x, (int) bullet->y, 0, 0, 0, 0);
}

void draw_bullet(LED_HANDLE_T h, bullet_t *bullet)
{
    pixel_t *pixel = get_bullet_color(bullet);
    led_set_pixel(h, (int) bullet->x, (int) bullet->y,
            pixel->bright, pixel->r,
            pixel->g, pixel->b);
}

int move_bullet(LED_HANDLE_T h, bullet_t *bullet)
{
    double newx, newy;

    newx = bullet->x + bullet->dx;
    newy = bullet->y + bullet->dy;

    if (newx >= g_wide || newx < 0.0 ||
        newy >= g_high || newy < 0.0)
    {
        erase_bullet(h, bullet);
        return 1;
    }

    if ((int) newx != (int) bullet->x ||
        (int) newy != (int) bullet->y)
    {
        erase_bullet(h, bullet);
        bullet->x = newx;
        bullet->y = newy;
        draw_bullet(h, bullet);
        draw_dudes(h);
    }
    else
    {
        bullet->x = newx;
        bullet->y = newy;
    }

    return 0;
}

void fire_bullet(LED_HANDLE_T h, dude_t *dude)
{
    if (g_bullet_count == MAX_BULLETS)
        return;

    bullet_t *bullet = &g_bullets[g_bullet_count++];
    bullet->id = dude->id;
    bullet->dx = dude->dx + BULLET_SPEED * X_ORIENT(dude->orientation);
    bullet->dy = dude->dy + BULLET_SPEED * Y_ORIENT(dude->orientation);
    bullet->x = (int) wrap_x((int)dude->x, X_ORIENT(dude->orientation));
    bullet->y = (int) wrap_y((int)dude->y, Y_ORIENT(dude->orientation));
}


void change_orientation(dude_t *dude, int o)
{
    dude->orientation += o;

    if (dude->orientation < 0)
        dude->orientation = ORIENT_WEST;
    if (dude->orientation > ORIENT_WEST)
        dude->orientation = ORIENT_NORTH;
}

void left(LED_HANDLE_T h, dude_t *dude)
{
    erase_dude(h, dude);
    change_orientation(dude, -1);
}

void right(LED_HANDLE_T h, dude_t *dude)
{
    erase_dude(h, dude);
    change_orientation(dude, 1);
}

void my_callback(LED_HANDLE_T h, unsigned long key)
{
    printf("Key 0x%lx\n", key);
    if (key == 'q')
        g_quit++;

    if (key == 'd')
        right(h, &g_dudes[BLUE_DUDE]);

    if (key == 'a')
        left(h, &g_dudes[BLUE_DUDE]);

    if (key == 'w')
        accelerate_dude(h, &g_dudes[BLUE_DUDE]);

    if (key == ' ' || key == 's')
        fire_bullet(h, &g_dudes[BLUE_DUDE]);

    if (key == XK_Right)
        right(h, &g_dudes[RED_DUDE]);

    if (key == XK_Left)
        left(h, &g_dudes[RED_DUDE]);

    if (key == XK_Up)
        accelerate_dude(h, &g_dudes[RED_DUDE]);

    if (key == XK_Down)
        fire_bullet(h, &g_dudes[RED_DUDE]);

    draw_dudes(h);
}


pixel_t * compute_explosion(explosion_t *exp, int x, int y)
{
    static pixel_t colors[2] =
    {
        { 0xc0, 0, 0xf, 0 },
        { 0x80, 0, 5, 0 }
    };

    if (x == 0 && y == 0)
        return &colors[1];
    return &colors[0];
}

int draw_explosion(LED_HANDLE_T h, explosion_t *exp)
{
    int i, j;
    for (i = -1 * EXPLOSION_RADIUS; i <= EXPLOSION_RADIUS; i++)
        for (j = -1 * EXPLOSION_RADIUS; j <= EXPLOSION_RADIUS; j++)
        {
            pixel_t *pixel = compute_explosion(exp, i, j);
            if (exp->x + i >= 0 && exp->x + i < g_wide &&
                exp->y + j >= 0 && exp->y + j < g_high)
                led_set_pixel(h, exp->x + i, exp->y + j,
                    pixel->bright, pixel->r, pixel->g, pixel->b);
        }

    exp->tick++;

    if (exp->tick >= EXPLOSION_LIFE_CYCLE)
        return 1;

    return 0;
}

void move_stuff(LED_HANDLE_T h)
{
    int i;

    for (i = 0; i < g_explosion_count; i++)
    {
        if (draw_explosion(h, &g_explosions[i]))
        {
            if (i < (g_explosion_count - 1))
                g_explosions[i] = g_explosions[g_explosion_count - 1];
            g_explosion_count--;
        }
    }
    move_dude(h, &g_dudes[BLUE_DUDE]);
    move_dude(h, &g_dudes[RED_DUDE]);
    for (i = 0; i < g_bullet_count; i++)
    {
        if (move_bullet(h, &g_bullets[i]))
        {
            if (i < (g_bullet_count - 1))
                g_bullets[i] = g_bullets[g_bullet_count - 1];
            g_bullet_count--;
        }
    }
}


#define MAX_COMMAND_LEN 80
#define FIFO_PATH "myfifo"

typedef void (*fifo_callback_t)(void *, void *);

typedef struct
{
    char *pathname;
    pthread_t t;
    void *first_parm;
    fifo_callback_t callback;
} fifo_t;

static void fifo_thread(fifo_t *f)
{
    char buf[MAX_COMMAND_LEN];
    FILE *fp;

    while (1)
    {
        fp = fopen(f->pathname, "r");
        if (! fp)
            break;

        while (! feof(fp))
        {
            buf[0] = '\0';
            if (fgets(buf, sizeof(buf), fp) == NULL)
                break;

            if (strlen(buf) > 0)
            {
                if (buf[strlen(buf) - 1] == '\n')
                    buf[strlen(buf) - 1] = '\0';
            }

            printf("Read [%s], going to %p\n", buf, f->callback);

            (*f->callback)(f->first_parm, buf);
        }
    }
}

int fifo_init(const char *pathname, fifo_callback_t callback, fifo_t *f, void * first_parm)
{
    pthread_attr_t attr;
    int rc;

    rc = mkfifo(pathname, S_IRWXU | S_IRWXG);
    if (rc == -1 && errno != EEXIST)
        return -1;

    if (pthread_attr_init(&attr) != 0 ||
        pthread_create(&f->t, &attr, (void * (*)(void *))fifo_thread, f) != 0)
        return -1;

    f->pathname = strdup(pathname);
    f->callback = callback;
    f->first_parm = first_parm;

    return 0;
}

void fifo_term(fifo_t *f)
{
    pthread_cancel(f->t);
    free(f->pathname);
}


void fifo_callback(void *parm, void *p)
{
    LED_HANDLE_T h = (LED_HANDLE_T) parm;
    int dude;
    char buf[MAX_COMMAND_LEN];
    printf("Callback of %s\n", (char *) p);
    sscanf(p, "%d-%s\n", &dude, buf);
    if (strcasecmp(buf, "left") == 0)
        left(h, &g_dudes[dude]);
    if (strcasecmp(buf, "right") == 0)
        right(h, &g_dudes[dude]);
    if (strcasecmp(buf, "up") == 0)
        accelerate_dude(h, &g_dudes[dude]);
    if (strcasecmp(buf, "fire") == 0)
        fire_bullet(h, &g_dudes[dude]);

    draw_dudes(h);
}

int main (int argc, char *argv[])
{
    LED_HANDLE_T p;
    fifo_t f;

    p = led_init((led_x_callback) my_callback);
    led_get_size(p, &g_wide, &g_high);

    if (fifo_init(FIFO_PATH, &fifo_callback, &f, p) != 0)
    {
        fprintf(stderr, "Error opening command fifo\n");
        exit(1);
    }

    memset(&g_dudes[BLUE_DUDE], 0, sizeof(g_dudes[BLUE_DUDE]));
    memset(&g_dudes[RED_DUDE], 0, sizeof(g_dudes[RED_DUDE]));
    memset(&g_bullets, 0, sizeof(g_bullets));
    g_dudes[BLUE_DUDE].id = BLUE_DUDE;
    g_dudes[BLUE_DUDE].orientation = ORIENT_EAST;
    g_dudes[RED_DUDE].id = RED_DUDE;
    g_dudes[RED_DUDE].orientation = ORIENT_WEST;
    g_dudes[RED_DUDE].y = g_high - 1;
    g_dudes[RED_DUDE].x = g_wide - 1;

    g_explosions[g_explosion_count].x = 5;
    g_explosions[g_explosion_count].y = 3;
    g_explosions[g_explosion_count].tick = 0;
    //g_explosion_count++;

    draw_dudes(p);
    while (! g_quit)
    {
        printf("tick\n");
        usleep(100000);
        move_stuff(p);
    }

    led_term(p);
    fifo_term(&f);

    return 0;
}
