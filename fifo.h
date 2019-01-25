#ifndef __FIFO_H
#define __FIFO_H
#include <string.h>

/*
    Assumptions
    Sender copies message into fifo item fo that no malloc and free are required
*/

#define FIFO_QUEUE_CAPACITY 128
#define FIFO_MAX_ITEM_SIZE 128
#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 64
#endif


typedef struct {
    volatile int head __attribute__ ((aligned(CACHELINE_SIZE)));
    volatile int tail __attribute__ ((aligned(CACHELINE_SIZE)));
    void* data[FIFO_QUEUE_CAPACITY][FIFO_MAX_ITEM_SIZE] __attribute__ ((aligned(CACHELINE_SIZE)));
} msgFifo_t __attribute__ ((aligned(CACHELINE_SIZE)));


extern void initFifo(msgFifo_t* pFifo);


extern int availableFifo(msgFifo_t* pFifo);



extern int enqueueFifo(msgFifo_t* pFifo, void* item);


extern int dequeueFifo(msgFifo_t* pFifo, void* item);

#endif


