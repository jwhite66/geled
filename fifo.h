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

int fifo_init(const char *pathname, fifo_callback_t callback, fifo_t *f, void * first_parm);
void fifo_term(fifo_t *f);
