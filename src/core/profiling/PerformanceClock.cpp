#include <core/profiling/PerformanceClock.hpp>

#include <core/utilities/Time.hpp>

#ifdef HYP_UNIX
#include <sys/time.h>
#elif defined (HYP_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace hyperion {
namespace profiling {

uint64 PerformanceClock::Now()
{
    // use clock_gettime if available
#ifdef HYP_UNIX
    struct timeval tv;
    gettimeofday(&tv, 0);

    return uint64(tv.tv_sec) * 1000000 + uint64(tv.tv_usec);
#else
    // @TODO 

    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    return uint64(ft.dwHighDateTime) << 32 | ft.dwLowDateTime;
#endif
}

uint64 PerformanceClock::TimeSince(uint64 microseconds)
{
    const uint64 us = microseconds;
    const uint64 now = Now();

    return now - us;
}
    
} // namespace profiling
} // namespace hyperion