#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fifo.h"

/*
    Assumptions
    Sender copies message into fifo item fo that no malloc and free are required
*/


void initFifo(msgFifo_t* pFifo)
{
    pFifo->head = 0;
    pFifo->tail = 0;
}

int availableFifo(msgFifo_t* pFifo)
{
	if (pFifo->tail < pFifo->head)
		return pFifo->head - pFifo->tail - 1;
	else
		return pFifo->head + (FIFO_QUEUE_CAPACITY - pFifo->tail);
}



int enqueueFifo(msgFifo_t* pFifo, void* item)
{
    if (availableFifo(pFifo) == 0) // when queue is full
    {
        //printf("Queue is full\n");
        return 1;
    }
    else
    {
        memcpy((pFifo->data)[pFifo->tail] ,item, FIFO_MAX_ITEM_SIZE);
        (pFifo->tail)++;
        (pFifo->tail) %= FIFO_QUEUE_CAPACITY;
    }
    return 0;
}

int dequeueFifo(msgFifo_t* pFifo, void* item)
{
    // Return 0 when queue is empty
    // Return 1 and fill (void*)item from the head otherwise.
    //printf("dequeueFifo head %d tail %d\n",pFifo->head ,pFifo->tail );
    if (pFifo->head == pFifo->tail){
        return 0;
    }
    else
    {
        memcpy(item,(pFifo->data)[pFifo->head], FIFO_MAX_ITEM_SIZE);
        (pFifo->head)++;
        (pFifo->head) %= FIFO_QUEUE_CAPACITY;
    }
    return 1;
}


