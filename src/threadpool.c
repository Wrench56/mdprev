#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "threadpool.h"

static pool_entry_t THREADPOOL[THREADPOOL_SZ] = { 0 };
static pool_data_t POOLDATA[THREADPOOL_SZ] = { 0 };

static void* internal_handler(void* data) {
    pool_data_t* pdata = (pool_data_t*) data;

    pdata->handler(pdata->data);

    pdata->pool_entry->status = IDLE;
    return NULL;
}

static void add_work_to_entry(pool_data_t* pdata, pool_entry_t* entry, void* handler, void* data) {
    entry->status = WORKING;

    pdata->pool_entry = entry;
    pdata->handler = handler;
    pdata->data = data;

    entry->thrd = 0;
    if (pthread_create(&entry->thrd, NULL, internal_handler, pdata) != 0) {
        perror("pthread_create() failed:");

        // TODO: Cleanup
        exit(1);
    }

    pthread_detach(entry->thrd);
}

void assign_worker(void* handler, void* data) {
    int64_t potential = -1;
    for (uint32_t i = 0; i < THREADPOOL_SZ; i++) {
        pool_entry_t* entry = &THREADPOOL[i];
        if (entry->status == WORKING) {
            printf("Found WORKING worker: %d\n", i);
            continue;
        }

        if (entry->status == UNINIT) {
            potential = i;
        }

        if (entry->status == IDLE) {
            add_work_to_entry(&POOLDATA[i], entry, handler, data);
            printf("Using initialized IDLE worker: %d\n", i);
            potential = -1;
            return;
        }
    }

    if (potential != -1) {
        pool_entry_t* entry = &THREADPOOL[potential];
        add_work_to_entry(&POOLDATA[potential], entry, handler, data);
        printf("Using uninitialized worker: %ld\n", potential);
        return;
    }

    fprintf(stderr, "Error: Ran out of workers!");
    exit(1);
}
