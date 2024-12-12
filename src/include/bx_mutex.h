#ifndef BX_MUTEX_H__
#define BX_MUTEX_H__

#include <stdatomic.h>
#include <stdbool.h>
#include <threads.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

#define MUTEX_USE_ATOMIC 1
#define MUTEX_RETRY_BEFORE_FAIL     1000

#if MUTEX_USE_ATOMIC
typedef atomic_bool BXMutex;

static inline bool bx_mutex_lock(BXMutex * mutex) 
{
    assert(mutex != NULL);
    bool expected = false;
    int retry = MUTEX_RETRY_BEFORE_FAIL;
    while(!atomic_compare_exchange_strong(mutex, &expected, true)) {
        expected = false;
        thrd_yield();
        if (retry-- <= 0) { 
            return false;
        }
    }
    return true;
}

static inline void bx_mutex_unlock(BXMutex * mutex)
{
    assert(mutex != NULL);
    atomic_store(mutex, false);
}

static inline void bx_mutex_init(BXMutex * mutex)
{
    assert(mutex != NULL);
    atomic_store(mutex, false);
}
#else
#include <pthread.h>

typedef pthread_mutex_t BXMutex;

static inline bool bx_mutex_lock(BXMutex * mutex)
{
    int retry = 100;
    while(pthread_mutex_trylock(mutex) == EBUSY) {
        thrd_yield();
        if (retry-- <= 0) { return false; }
    }
    return true;
}

static inline void bx_mutex_unlock(BXMutex * mutex)
{
    pthread_mutex_unlock(mutex);
}

static inline void bx_mutex_init(BXMutex * mutex)
{
    pthread_mutex_init(mutex, NULL);
}

#endif /* MUTEX_USE_ATOMIC */


#endif /* BX_MUTEX_H__ */