/**
 * Test errno and nanosleep are working appropriately
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

static void __attribute__((constructor)) test_nanosleep()
{
    struct timespec ts;
    ts.tv_sec = 0x2;
    ts.tv_nsec = 0x100;
    assert(nanosleep(&ts, NULL) == 0);

    assert(nanosleep(NULL, NULL) == -1);
    assert(errno == EFAULT);

    ts.tv_nsec = 1000000000;
    assert(nanosleep(&ts, NULL) == -1);
    assert(errno == EINVAL);
}
