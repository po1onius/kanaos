#include <fifo.h>
#include <assert.h>
#include <string.h>

static _inline uint32 fifo_next(struct fifo_t *fifo, uint32 pos)
{
    return (pos + 1) % FIFOSIZE;
}

void fifo_init(struct fifo_t *fifo)
{
    memset(fifo->buf, 0, FIFOSIZE);
    fifo->head = 0;
    fifo->tail = 0;
}

bool fifo_full(struct fifo_t *fifo)
{
    bool full = (fifo_next(fifo, fifo->head) == fifo->tail);
    return full;
}

bool fifo_empty(struct fifo_t *fifo)
{
    return (fifo->head == fifo->tail);
}

int8 fifo_get(struct fifo_t *fifo)
{
    assert(!fifo_empty(fifo));
    int8 byte = fifo->buf[fifo->tail];
    fifo->tail = fifo_next(fifo, fifo->tail);
    return byte;
}

void fifo_put(struct fifo_t *fifo, int8 byte)
{
    while (fifo_full(fifo))
    {
        fifo_get(fifo);
    }
    fifo->buf[fifo->head] = byte;
    fifo->head = fifo_next(fifo, fifo->head);
}


