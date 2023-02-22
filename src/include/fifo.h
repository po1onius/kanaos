#ifndef __FIFO_H__
#define __FIFO_H__

#include <types.h>
#define FIFOSIZE 128

struct fifo_t
{
    int8 buf[FIFOSIZE];
    uint32 head;
    uint32 tail;
};

void fifo_init(struct fifo_t *fifo);
bool fifo_full(struct fifo_t *fifo);
bool fifo_empty(struct fifo_t *fifo);
int8 fifo_get(struct fifo_t *fifo);
void fifo_put(struct fifo_t *fifo, int8 byte);


#endif
