/**
 * @file    pseudo_tcp.c
 * @brief   pseudoTCP
 *
 *
 * @versioN $Revision$
 * @date    $Date$
 * @author  albert.liao
 *
 * @par Fixes:
 *   None.
 * @par To Fix:
 * @par Reference:
 *****************************************************************************
 *  <b>CONFIDENTIAL</b><br>
 *****************************************************************************/

/** @addtogroup pseudoTCP
 *  @addtogroup pseudoTCP Utilities
 *  @ingroup pseudoTCP
 *  @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/*** STANDARD INCLUDES *******************************************************/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef linux
#include <error.h>
#endif
#include <errno.h>
#include <time.h>

/*** PROJECT INCLUDES ********************************************************/
#include "mi_types.h"
#include "mi_util.h"

#if __APPLE__
#include <mach/mach_time.h>
#endif

/*** MACROS ******************************************************************/
#define EFFICIENT_IMPLEMENTATION 1
#ifndef U32_MAX
#define U32_MAX  ((U32_t)-1)
#endif

#ifndef INT64_C
#define INT64_C(x) x ## LL
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## ULL
#endif
/*****************************************************************************/
static const S64 kNumMillisecsPerSec = INT64_C(1000);
//static const S64 kNumMicrosecsPerSec = INT64_C(1000000);
static const S64 kNumNanosecsPerSec = INT64_C(1000000000);

const U32 HALF = 0x80000000;

U64 TimeNanos()
{
    S64 ticks = 0;
#if 1
//#if defined(OSX) || defined(IOS)
#if __APPLE__
    static mach_timebase_info_data_t timebase;
    if (timebase.denom == 0) {
        // Get the timebase if this is the first time we run.
        // Recommended by Apple's QA1398.
        //VERIFY(KERN_SUCCESS == mach_timebase_info(&timebase));
        if(KERN_SUCCESS != mach_timebase_info(&timebase)) {
            printf("mach_timebase_info() error!!\n");;
        }

    }
    // Use timebase to convert absolute time tick units into nanoseconds.
    ticks = mach_absolute_time() * timebase.numer / timebase.denom;
//#elif defined(POSIX) || defined(LINUX)
#elif __LINUX__
    struct timespec ts;
    // TODO: Do we need to handle the case when CLOCK_MONOTONIC
    // is not supported?
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ticks = kNumNanosecsPerSec * (S64)(ts.tv_sec) + (S64)(ts.tv_nsec);
#endif

#else
    struct timespec ts;
    // TODO: Do we need to handle the case when CLOCK_MONOTONIC
    // is not supported?
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ticks = kNumNanosecsPerSec * (S64)(ts.tv_sec) + (S64)(ts.tv_nsec);
#endif

    return ticks;
}

U32 Time()
{
    //return (U32)(TimeNanos() / kNumNanosecsPerMillisec);
    U32 vTime=0;
    vTime = (U32)(TimeNanos() / (kNumNanosecsPerSec/kNumMillisecsPerSec));
    //printf("Time() = %d \n", vTime);
    return vTime;
    //return (U32)(TimeNanos() / (kNumNanosecsPerSec/kNumMillisecsPerSec));
}


bool TimeIsBetween(U32 earlier, U32 middle, U32 later)
{
    if (earlier <= later) {
        return ((earlier <= middle) && (middle <= later));
    } else {
        return !((later < middle) && (middle < earlier));
    }
}

S32 TimeDiff(U32 later, U32 earlier)
{
#if EFFICIENT_IMPLEMENTATION
    return later - earlier;
#else
    const bool later_or_equal = TimeIsBetween(earlier, later, earlier + HALF);
    if (later_or_equal) {
        if (earlier <= later) {
            return (long)(later - earlier);
        } else {
            return (long)(later + (U32_MAX - earlier) + 1);
        }
    } else {
        if (later <= earlier) {
            return -(long)(earlier - later);
        } else {
            return -(long)(earlier + (U32_MAX - later) + 1);
        }
    }
#endif
}

S32 TimeSince(U32 earlier)
{
    return TimeDiff(Time(), earlier);
}

U32 TimeAfter(S32 elapsed)
{
    return Time() + elapsed;
}

S32 TimeUntil(U32 later)
{
    return TimeDiff(later, Time());
}

bool TimeIsLater(U32 earlier, U32 later)
{
#if EFFICIENT_IMPLEMENTATION
    S32 diff = later - earlier;
    return (diff > 0 && (U32)(diff) < HALF);
#else
    const bool earlier_or_equal = TimeIsBetween(later, earlier, later + HALF);
    return !earlier_or_equal;
#endif
}


// if test in linux or macos
#if 0
//#if defined(__linux__) ||  defined(__APPLE__)
#include <execinfo.h>
#define NOT_USED(a) (void )(a)
static void _print_backtrace(void)
{
    void *bt[1024];
    int bt_size;
    char **bt_syms;
    int i;

    bt_size = backtrace(bt, 1024);
    bt_syms = backtrace_symbols(bt, bt_size);
    for (i = 1; i < bt_size; i++) {
        printf("%s\n", bt_syms[i]);
    }
    free(bt_syms);
}

static void _segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
    NOT_USED(signal);
    NOT_USED(arg);
    printf("Caught segfault at address %p\n", si->si_addr);
    _print_backtrace();
    exit(0);
}

void Assert_WithBackTrace(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = _segfault_sigaction;
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);
}

bool Assert(bool result, const char* function, const char* file,
            int line, const char* expression)
{
    Assert_WithBackTrace();
    return true;
}

#else

bool Assert(bool result, const char* function, const char* file,
            int line, const char* expression)
{
    if (!result) {
        fprintf(stderr, "%s %s:%d %s\n", function, file, line, expression);
        // On POSIX systems, SIGTRAP signals debuggers to break without killing the
        // process. If a debugger isn't attached, the uncaught SIGTRAP will crash the
        // app.
        raise(SIGTRAP);
        return false;
    }
    return true;
}

#endif
/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/**  @} */
/******************************************************************************
 *  CONFIDENTIAL
 *****************************************************************************/
