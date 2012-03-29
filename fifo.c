#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>
#include "fifo.h"

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

            (*f->callback)(f->first_parm, buf);
        }
    }
}

int fifo_init(const char *pathname, fifo_callback_t callback, fifo_t *f, void * first_parm)
{
    pthread_attr_t attr;
    int rc;

    rc = mkfifo(pathname, S_IRWXU | S_IRWXG | S_IRWXO);
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
