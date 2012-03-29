#ifndef TETRIS_H
#define TETRIS_H

/*
 *  LED TETRIS - Based on tint - :
 *
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

/*
 * Boolean definitions
 */

#ifndef bool
#define bool int
#endif

#if !defined(false) || (false != 0)
#define false    0
#endif

#if !defined(true) || (true != 0)
#define true    1
#endif

#if !defined(FALSE) || (FALSE != false)
#define FALSE    false
#endif

#if !defined(TRUE) || (TRUE != true)
#define TRUE    true
#endif

/*
 * Error flags
 */

#if !defined(ERR) || (ERR != -1)
#define ERR        -1
#endif

#if !defined(OK) || (OK != 0)
#define OK        0
#endif



/* Color maps */
#define COLOR_BLACK   0
#define COLOR_WHITE   1
#define COLOR_CYAN    2
#define COLOR_GREEN   3
#define COLOR_YELLOW  4
#define COLOR_BLUE    5
#define COLOR_MAGENTA 6
#define COLOR_RED     7
#define COLOR_COUNT   8

typedef struct
{
    int bright;
    int r;
    int g;
    int b;
} color_map_t;


color_map_t g_color_map[COLOR_COUNT] =
{
    { 0, 0, 0, 0 },                                    /* Black   */
    { MAX_BRIGHT, MAX_RGB, MAX_RGB, MAX_RGB },         /* White   */
    { MAX_BRIGHT, 0,       MAX_RGB, MAX_RGB },         /* Cyan    */
    { MAX_BRIGHT, 0,       MAX_RGB, 0       },         /* Green   */
    { MAX_BRIGHT, MAX_RGB, MAX_RGB, 0       },         /* Yellow  */
    { MAX_BRIGHT, 0,       0,       MAX_RGB },         /* Blue    */
    { MAX_BRIGHT, MAX_RGB, 0,       MAX_RGB },         /* Magenta */
    { MAX_BRIGHT, MAX_RGB, 0,       0       },         /* Red     */
};


/*
 * Macros
 */

/* Number of levels in the game */
#define MINLEVEL    1
#define MAXLEVEL    9

/* This calculates the time allowed to move a shape, before it is moved a row down */
#define USECS_PER_TICK  50000
#define TICKS_BEFORE_DROP(level) (48 / (level + 2))

/* The score is multiplied by this to avoid losing precision */
#define SCOREFACTOR 2

/* This calculates the stored score value */
#define SCOREVAL(x) (SCOREFACTOR * (x))

/* This calculates the real (displayed) value of the score */
#define GETSCORE(score) ((score) / SCOREFACTOR)

/* Number of shapes in the game */
#define NUMSHAPES    7

/* Number of blocks in each shape */
#define NUMBLOCKS    4

/* Number of rows and columns in board */
#define NUMROWS    10
#define NUMCOLS    10

#define RAND_VALUE(range) ((random () % range))

#define INBOUNDS(x,y,dx,dy) ( ((x)+(dx)) >= 0 && ((x)+(dx)) < NUMCOLS && ((y)+(dy)) >= 0 && ((y)+(dy)) < NUMROWS )

/*
 * Type definitions
 */

typedef int board_t[NUMCOLS][NUMROWS];

typedef struct
{
    int x,y;
} block_t;

typedef struct
{
    int color;
    int type;
    bool flipped;
    block_t block[NUMBLOCKS];
} shape_t, shapes_t[NUMSHAPES];

typedef struct
{
    int moves;
    int rotations;
    int dropcount;
    int efficiency;
    int droppedlines;
} status_t;

typedef struct engine_struct
{
    int curx, cury;                                     /* coordinates of current piece */
    int curshape, nextshape;                            /* current & next shapes */
    int score;                                          /* score */
    shapes_t shapes;                                    /* shapes */
    board_t  board;                                     /* board */
    status_t status;                                    /* current status of shapes */
    void (*score_function)(struct engine_struct *);     /* score function */
} engine_t;

typedef enum { ACTION_LEFT, ACTION_ROTATE, ACTION_RIGHT, ACTION_DROP } action_t;

/*
 * Global variables
 */

extern const shapes_t SHAPES;

/*
 * Functions
 */

/*
 * Initialize specified tetris engine
 */
void engine_init (engine_t *engine,void (*score_function)(engine_t *));

/*
 * Perform the given action on the specified tetris engine
 */
void engine_move (engine_t *engine,action_t action);

/*
 * Evaluate the status of the specified tetris engine
 *
 * OUTPUT:
 *   1 = shape moved down one line
 *   0 = shape at bottom, next one released
 *  -1 = game over (board full)
 */
int engine_evaluate (engine_t *engine);

#endif    /* #ifndef TETRIS_H */
