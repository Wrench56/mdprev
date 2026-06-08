#ifndef MDPREV_THREADPOOL_H
#define MDPREV_THREADPOOL_H

#include <pthread.h>
#include <stdint.h>

#define THREADPOOL_SZ 8

typedef enum {
    UNINIT = 0,
    WORKING = 1,
    IDLE = 2,
} status_t;

typedef struct {
    pthread_t thrd;
    _Atomic status_t status;
} pool_entry_t;

typedef void (handler_t)(void*);

typedef struct {
    pool_entry_t* pool_entry;
    void* data;
    handler_t* handler;
} pool_data_t;

void assign_worker(void* handler, void* data);

#endif // MDPREV_THREADPOOL_H
