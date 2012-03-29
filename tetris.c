
/*
 *  LED TETRIS - Based on tint - :
 * Copyright (c) Abraham vd Merwe <abz@blio.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of other contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "led.h"
#include "tetris.h"
#include "fifo.h"

#if defined(SIMULATOR)
#include <X11/keysym.h>
#endif
/*
 * Global variables
 */

const shapes_t SHAPES =
{
   { COLOR_CYAN,    0, FALSE, { {  1,  0 }, {  0,  0 }, {  0, -1 }, { -1, -1 } } },
   { COLOR_GREEN,   1, FALSE, { {  1, -1 }, {  0, -1 }, {  0,  0 }, { -1,  0 } } },
   { COLOR_YELLOW,  2, FALSE, { { -1,  0 }, {  0,  0 }, {  1,  0 }, {  0,  1 } } },
   { COLOR_BLUE,    3, FALSE, { { -1, -1 }, {  0, -1 }, { -1,  0 }, {  0,  0 } } },
   { COLOR_MAGENTA, 4, FALSE, { { -1,  1 }, { -1,  0 }, {  0,  0 }, {  1,  0 } } },
   { COLOR_WHITE,   5, FALSE, { {  1,  1 }, {  1,  0 }, {  0,  0 }, { -1,  0 } } },
   { COLOR_RED,     6, FALSE, { { -1,  0 }, {  0,  0 }, {  1,  0 }, {  2,  0 } } }
};

static int g_level = MINLEVEL - 1;
static int g_shapecount[NUMSHAPES];
static bool g_shownext = false;
static bool g_finished = false;
static bool g_wakeup = false;
static int g_wide = 0;
static int g_high = 0;

/*
   Helper type
*/
typedef struct
{
    LED_HANDLE_T p;
    engine_t     engine;
} led_engine_t;

/*
 * Functions
 */

/* This rotates a shape */
static void real_rotate (shape_t *shape,bool clockwise)
{
   int i,tmp;
   if (clockwise)
     {
        for (i = 0; i < NUMBLOCKS; i++)
          {
             tmp = shape->block[i].x;
             shape->block[i].x = -shape->block[i].y;
             shape->block[i].y = tmp;
          }
     }
   else
     {
        for (i = 0; i < NUMBLOCKS; i++)
          {
             tmp = shape->block[i].x;
             shape->block[i].x = shape->block[i].y;
             shape->block[i].y = -tmp;
          }
     }
}

/* Rotate shapes the way tetris likes it (= not mathematically correct) */
static void fake_rotate (shape_t *shape)
{
   switch (shape->type)
     {
      case 0:    /* Just rotate this one anti-clockwise and clockwise */
        if (shape->flipped) real_rotate (shape,TRUE); else real_rotate (shape,FALSE);
        shape->flipped = !shape->flipped;
        break;
      case 1:    /* Just rotate these two clockwise and anti-clockwise */
      case 6:
        if (shape->flipped) real_rotate (shape,FALSE); else real_rotate (shape,TRUE);
        shape->flipped = !shape->flipped;
        break;
      case 2:    /* Rotate these three anti-clockwise */
      case 4:
      case 5:
        real_rotate (shape,FALSE);
        break;
      case 3:    /* This one is not rotated at all */
        break;
     }
}

/* Draw a shape on the board */
static void drawshape (board_t board,shape_t *shape,int x,int y)
{
   int i;
   for (i = 0; i < NUMBLOCKS; i++)
       if (INBOUNDS(x, y, shape->block[i].x, shape->block[i].y))
            board[x + shape->block[i].x][y + shape->block[i].y] = shape->color;
}

/* Erase a shape from the board */
static void eraseshape (board_t board,shape_t *shape,int x,int y)
{
    int i;
    for (i = 0; i < NUMBLOCKS; i++)
        if (INBOUNDS(x, y, shape->block[i].x, shape->block[i].y))
            board[x + shape->block[i].x][y + shape->block[i].y] = COLOR_BLACK;
}

/* Check if shape is allowed to be in this position */
static bool allowed (board_t board,shape_t *shape,int x,int y)
{
    int i, occupied = FALSE;
    if (x < 0 || x >= NUMCOLS || y >= NUMROWS)
        return false;

    for (i = 0; i < NUMBLOCKS; i++)
    {
        if  ( (x + shape->block[i].x) < 0 || (x + shape->block[i].x) >= NUMCOLS)
            return false;

        if  ( (y + shape->block[i].y) >= NUMROWS)
            return false;

        /* We allow shapes to start 'off screen' */
        if (y + shape->block[i].y < 0)
            continue;

        if (board[x + shape->block[i].x][y + shape->block[i].y])
            occupied = TRUE;
    }
    return (!occupied);
}

/* Move the shape left if possible */
static bool shape_left (board_t board,shape_t *shape,int *x,int y)
{
   bool result = FALSE;
   eraseshape (board,shape,*x,y);
   if (allowed (board,shape,*x - 1,y))
     {
        (*x)--;
        result = TRUE;
     }
   drawshape (board,shape,*x,y);
   return result;
}

/* Move the shape right if possible */
static bool shape_right (board_t board,shape_t *shape,int *x,int y)
{
   bool result = FALSE;
   eraseshape (board,shape,*x,y);
   if (allowed (board,shape,*x + 1,y))
     {
        (*x)++;
        result = TRUE;
     }
   drawshape (board,shape,*x,y);
   return result;
}

/* Rotate the shape if possible */
static bool shape_rotate (board_t board,shape_t *shape,int x,int y)
{
   bool result = FALSE;
   shape_t test;
   eraseshape (board,shape,x,y);
   memcpy (&test,shape,sizeof (shape_t));
   fake_rotate (&test);
   if (allowed (board,&test,x,y))
     {
        memcpy (shape,&test,sizeof (shape_t));
        result = TRUE;
     }
   drawshape (board,shape,x,y);
   return result;
}

/* Move the shape one row down if possible */
static bool shape_down (board_t board,shape_t *shape,int x,int *y)
{
   bool result = FALSE;
   eraseshape (board,shape,x,*y);
   if (allowed (board,shape,x,*y + 1))
     {
        (*y)++;
        result = TRUE;
     }
   drawshape (board,shape,x,*y);
   return result;
}

/* Check if shape can move down (= in the air) or not (= at the bottom */
/* of the board or on top of one of the resting shapes) */
static bool shape_bottom (board_t board,shape_t *shape,int x,int y)
{
   bool result = FALSE;
   eraseshape (board,shape,x,y);
   result = !allowed (board,shape,x,y + 1);
   drawshape (board,shape,x,y);
   return result;
}

/* Drop the shape until it comes to rest on the bottom of the board or */
/* on top of a resting shape */
static int shape_drop (board_t board,shape_t *shape,int x,int *y)
{
   int droppedlines = 0;
   eraseshape (board,shape,x,*y);
   while (allowed (board,shape,x,*y + 1))
     {
        (*y)++;
        droppedlines++;
     }
   drawshape (board,shape,x,*y);
   return droppedlines;
}

/* This removes all the rows on the board that is completely filled with blocks */
static int droplines (board_t board)
{
    int x,y,ny,status,droppedlines;
    board_t newboard;
    /* initialize new board */
    memset (newboard,0,sizeof (board_t));
    ny = NUMROWS - 1;
    droppedlines = 0;
    for (y = NUMROWS - 1; y >= 0; y--)
    {
        status = 0;
        for (x = 0; x < NUMCOLS; x++) if (board[x][y]) status++;
        if (status < NUMCOLS)
        {
             for (x = 0; x < NUMCOLS; x++) newboard[x][ny] = board[x][y];
             ny--;
        }
        else
            droppedlines++;
    }
    memcpy (board, newboard, sizeof (board_t));
    return droppedlines;
}

static bool can_start_high(shape_t *shape)
{
    int i;
    bool rc = true;
    for (i = 0; i < NUMBLOCKS; i++)
        if (shape->block[i].y < 0)
            rc = false;

    return rc;
}

/*
 * Initialize specified tetris engine
 */
void engine_init (engine_t *engine,void (*score_function)(engine_t *))
{
   engine->score_function = score_function;
   /* intialize values */
   engine->curx = (NUMCOLS / 2);
   engine->cury = 0;
   engine->curshape = RAND_VALUE (NUMSHAPES);
   engine->nextshape = RAND_VALUE (NUMSHAPES);
   engine->score = 0;
   engine->status.moves = engine->status.rotations = engine->status.dropcount = engine->status.efficiency = engine->status.droppedlines = 0;
   /* initialize shapes */
   memcpy (engine->shapes, SHAPES, sizeof (shapes_t));

   if (can_start_high(&engine->shapes[engine->curshape]))
       engine->cury--;

   /* initialize board */
   memset (engine->board, 0, sizeof (board_t));
}

/*
 * Perform the given action on the specified tetris engine
 */
void engine_move (engine_t *engine,action_t action)
{
   switch (action)
     {
        /* move shape to the left if possible */
      case ACTION_LEFT:
        if (shape_left (engine->board,&engine->shapes[engine->curshape],&engine->curx,engine->cury)) engine->status.moves++;
        break;
        /* rotate shape if possible */
      case ACTION_ROTATE:
        if (shape_rotate (engine->board,&engine->shapes[engine->curshape],engine->curx,engine->cury)) engine->status.rotations++;
        break;
        /* move shape to the right if possible */
      case ACTION_RIGHT:
        if (shape_right (engine->board,&engine->shapes[engine->curshape],&engine->curx,engine->cury)) engine->status.moves++;
        break;
        /* drop shape to the bottom */
      case ACTION_DROP:
        engine->status.dropcount += shape_drop (engine->board,&engine->shapes[engine->curshape],engine->curx,&engine->cury);
     }
}

/*
 * Evaluate the status of the specified tetris engine
 *
 * OUTPUT:
 *   1 = shape moved down one line
 *   0 = shape at bottom, next one released
 *  -1 = game over (board full)
 */
int engine_evaluate (engine_t *engine)
{
    int deltax;
    if (shape_bottom (engine->board,&engine->shapes[engine->curshape],engine->curx,engine->cury))
    {
        /* increase score */
        engine->score_function (engine);
        /* update status information */
        engine->status.droppedlines += droplines (engine->board);
        deltax = abs(engine->curx - (NUMCOLS / 2));
        engine->status.rotations = 4 - engine->status.rotations;
        engine->status.rotations = engine->status.rotations > 0 ? 0 : engine->status.rotations;
        engine->status.efficiency += engine->status.dropcount + engine->status.rotations + (deltax - engine->status.moves);
        engine->status.efficiency >>= 1;
        engine->status.dropcount = engine->status.rotations = engine->status.moves = 0;
        /* intialize values */
        engine->curx = (NUMCOLS / 2);
        engine->cury = 0;
        engine->curshape = engine->nextshape;
        engine->nextshape = RAND_VALUE (NUMSHAPES);
        /* initialize shapes */
        memcpy (engine->shapes,SHAPES,sizeof (shapes_t));
       if (can_start_high(&engine->shapes[engine->curshape]))
           engine->cury--;

        /* return games status */
        return allowed (engine->board,&engine->shapes[engine->curshape],engine->curx,engine->cury) ? 0 : -1;
    }
    shape_down (engine->board,&engine->shapes[engine->curshape],engine->curx,&engine->cury);
    return 1;
}


/*
 * Functions
 */

/* This function is responsible for increasing the score appropriately whenever
 * a block collides at the bottom of the screen (or the top of the heap */
static void score_function (engine_t *engine)
{
    int score = SCOREVAL (g_level * (engine->status.dropcount + 1));

    engine->score += score;
    if (g_shownext)
        score /= 2;
}

/* Draw the board on the screen */
static void drawboard (led_engine_t *le)
{
    int x, y;
    for (y = 0; y < NUMROWS; y++)
        for (x = 0; x < NUMCOLS; x++)
        {
            color_map_t *map = &g_color_map[le->engine.board[x][y]];
            led_set_pixel(le->p, x, y, map->bright, map->r, map->g, map->b);
        }
}


          /***************************************************************************/
          /***************************************************************************/
          /***************************************************************************/

static void showhelp ()
{
   fprintf (stderr,"USAGE: tint [-h] [-l level] [-n] [-d] [-b char]\n");
   fprintf (stderr,"  -h           Show this help message\n");
   fprintf (stderr,"  -l <level>   Specify the starting level (%d-%d)\n",MINLEVEL,MAXLEVEL);
   fprintf (stderr,"  -n           Draw next shape\n");
   exit (EXIT_FAILURE);
}

static void parse_options (int argc,char *argv[])
{
    int i = 1;
    while (i < argc)
    {
        /* Help? */
        if (strcmp (argv[i],"-h") == 0)
          showhelp ();
        /* Level? */
        else if (strcmp (argv[i],"-l") == 0)
        {
             i++;
             if (i >= argc)
                 showhelp();
             else
                 g_level = atoi(argv[i]);

             if ((g_level < MINLEVEL) || (g_level > MAXLEVEL))
               {
                  fprintf (stderr,"You must specify a level between %d and %d\n",MINLEVEL,MAXLEVEL);
                  exit (EXIT_FAILURE);
               }
        }
        /* Show next? */
        else if (strcmp (argv[i],"-n") == 0)
          g_shownext = TRUE;
        else
          {
             fprintf (stderr,"Invalid option -- %s\n",argv[i]);
             showhelp ();
          }
        i++;
     }
}


#if defined(SIMULATOR)
static led_engine_t *g_engine_p;
void my_callback(LED_HANDLE_T p, unsigned long key)
{
    if (key == 'q')
    {
        g_finished++;
        g_wakeup = true;
    }

    if (key == XK_Right)
        engine_move (&g_engine_p->engine, ACTION_RIGHT);

    if (key == XK_Left)
        engine_move (&g_engine_p->engine, ACTION_LEFT);

    if (key == XK_Up)
        engine_move (&g_engine_p->engine, ACTION_ROTATE);

    if (key == XK_Down)
    {
        engine_move (&g_engine_p->engine, ACTION_DROP);
        g_wakeup = true;
    }

    /* FIXME - todo pause, next level, shownext */
    drawboard(g_engine_p);
}
#endif

void fifo_callback(void *parm, void *p)
{
    led_engine_t *le = (led_engine_t *) parm;
    char buf[MAX_COMMAND_LEN];
    sscanf(p, "%s\n", buf);
    if (strcasecmp(buf, "left") == 0)
        engine_move (&le->engine, ACTION_LEFT);
    if (strcasecmp(buf, "right") == 0)
        engine_move (&le->engine, ACTION_RIGHT);
    if (strcasecmp(buf, "rotate") == 0)
        engine_move (&le->engine, ACTION_ROTATE);
    if (strcasecmp(buf, "drop") == 0)
    {
        engine_move (&le->engine, ACTION_DROP);
        g_wakeup = true;
    }
    if (strcasecmp(buf, "quit") == 0)
    {
        g_finished = true;
        g_wakeup = true;
    }

    /* FIXME - todo pause, next level, shownext */

    drawboard (le);
}

int main (int argc, char *argv[])
{
    int i;
    fifo_t f;
    led_engine_t le;

    /* Initialize */
    srandom (time (NULL));                      /* must be called before engine_init () */
    engine_init (&le.engine,score_function);    /* must be called before using engine.curshape */
    le.p = led_init();

    memset (g_shapecount, 0, sizeof(g_shapecount));
    g_shapecount[le.engine.curshape]++;

    parse_options (argc,argv);                /* must be called after initializing variables */

    if (g_level < MINLEVEL)
        g_level = MINLEVEL;

#if defined(SIMULATOR)
    g_engine_p = &le;
    ledsim_set_x_callback(le.p, my_callback);
#endif

    led_get_size(le.p, &g_wide, &g_high);

    umask(0);
    if (fifo_init(FIFO_PATH, &fifo_callback, &f, &le) != 0)
    {
        fprintf(stderr, "Error opening command fifo\n");
        exit(1);
    }

    while (! g_finished)
    {
        drawboard (&le);
        for (i = 0; i < TICKS_BEFORE_DROP(g_level) && ! g_wakeup; i++)
            usleep(USECS_PER_TICK);

        if (g_finished)
            break;

        switch (engine_evaluate (&le.engine))
        {
            /* game over (board full) */
            case -1:
                if ((g_level < MAXLEVEL) && ((le.engine.status.droppedlines / 10) > g_level))
                    g_level++;
                g_finished = TRUE;
                break;

            /* shape at bottom, next one released */
            case 0:
                if ((g_level < MAXLEVEL) && ((le.engine.status.droppedlines / 10) > g_level))
                    g_level++;
                g_shapecount[le.engine.curshape]++;
                break;

            /* shape moved down one line */
            case 1:
                break;
        }

        g_wakeup = false;
    }

    led_term(le.p);
    fifo_term(&f);

    return 0;
}
