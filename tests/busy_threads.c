/**
 * 
 * arm-vita-eabi-gcc -shared -O3 -o busy_threads.so ./busy_threads.c -pthread
 */ 

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

static pthread_barrier_t barrier;
static atomic_int serialThreadCount;

static void* func(void *arg)
{
    int result = pthread_barrier_wait(&barrier);

    assert(result == PTHREAD_BARRIER_SERIAL_THREAD || result == 0);
    if (result == PTHREAD_BARRIER_SERIAL_THREAD)
    {
        __atomic_fetch_add(&serialThreadCount, 1, __ATOMIC_SEQ_CST);
    }

    return NULL;
}

static void __attribute__((constructor)) test_busy_threads()
{
    static const size_t ITERATIONS = 1000;
    static const size_t NUMTHREADS = 16;

    pthread_t t[NUMTHREADS];

    for (int j = 0; j < ITERATIONS; j++)
    {
        printf("iteration %d\n", j);

        __atomic_store_n(&serialThreadCount, 0, __ATOMIC_SEQ_CST);

        assert(pthread_barrier_init(&barrier, NULL, NUMTHREADS) == 0);

        for (int i = 0; i < NUMTHREADS; i++)
        {
            assert(pthread_create(&t[i], NULL, func, NULL) == 0);
        }

        for (int i = 0; i < NUMTHREADS; i++)
        {
            assert(pthread_join(t[i], NULL) == 0);
        }

        assert(__atomic_load_n(&serialThreadCount, __ATOMIC_SEQ_CST) == 1);

        assert(pthread_barrier_destroy(&barrier) == 0);
    }
}
