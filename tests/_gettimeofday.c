/**
 * _gettimeofday
 *
 * 1. assert _gettimeofday does not overrun buffer (leads to a stack corruption)
 * 2. assert time moves forward (pthread-embedded depends on time to move forward, or "pthread_timechange_handler_np"
 * must be called)
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct timeval timeval;
timeval gettimeofday_safe()
{
    /* make sure you never overrun stack */

    static const size_t BUFFER_SIZE = 100;
    uint8_t buffer[BUFFER_SIZE];
    memset(&buffer, 0xff, sizeof(buffer));

    int r = gettimeofday((struct timeval *)&buffer[0], NULL);
    assert(r == 0);

    static const size_t TV_SIZE = sizeof(struct timeval);
    for (int i = TV_SIZE; i < BUFFER_SIZE; i++)
    {
        assert(buffer[i] == 0xff);
    }

    return *(struct timeval *)(&buffer[0]);
}

static void __attribute__((constructor)) test_gettimeofday()
{
    struct timeval tv1 = gettimeofday_safe();

    for (int i = 0; i < 40; i++)
    {
        usleep(100000); // 100ms

        struct timeval tv2 = gettimeofday_safe();

        assert(tv2.tv_sec >= tv1.tv_sec);
        assert(tv2.tv_sec > tv1.tv_sec || tv2.tv_usec > tv1.tv_usec);
        tv1 = tv2;
    }
}
