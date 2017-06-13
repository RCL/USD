//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/timing.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/export.h"

#if defined(ARCH_OS_LINUX)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#elif defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <chrono>
#include <thread>
#endif

PXR_NAMESPACE_OPEN_SCOPE

static double Arch_NanosecondsPerTick = 1.0;

#if defined(ARCH_OS_DARWIN)

ARCH_HIDDEN
void
Arch_InitTickTimer()
{
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    Arch_NanosecondsPerTick = static_cast<double>(info.numer) / info.denom;
}

#elif defined(ARCH_OS_LINUX)

ARCH_HIDDEN
void
Arch_InitTickTimer()
{
    // clock_gettime() already returns nanoseconds
    Arch_NanosecondsPerTick = 1;
}
#elif defined(ARCH_OS_WINDOWS)

ARCH_HIDDEN
void
Arch_InitTickTimer()
{
    // We want to use rdtsc so we need to find the frequency.  We run a
    // small sleep here to compute it using QueryPerformanceCounter()
    // which is independent of rdtsc.  So we wait for some duration using
    // QueryPerformanceCounter() (whose frequency we know) then compute
    // how many ticks elapsed during that time and from that the number
    // of ticks per nanosecond.
    LARGE_INTEGER qpcFreq, qpcStart, qpcEnd;
    QueryPerformanceFrequency(&qpcFreq);
    const auto delay = (qpcFreq.QuadPart >> 4); // 1/16th of a second.
    QueryPerformanceCounter(&qpcStart);
    const auto t1 = ArchGetTickTime();
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        QueryPerformanceCounter(&qpcEnd);
    } while (qpcEnd.QuadPart - qpcStart.QuadPart < delay);
    QueryPerformanceCounter(&qpcEnd);
    const auto t2 = ArchGetTickTime();

    // Total time take during the loop in seconds.
    const auto durationInSeconds =
        static_cast<double>(qpcEnd.QuadPart - qpcStart.QuadPart) /
        qpcFreq.QuadPart;

    // Nanoseconds per tick.
    constexpr auto nanosPerSecond = 1.0e9;
    Arch_NanosecondsPerTick = nanosPerSecond * durationInSeconds / (t2 - t1);
}

#else    
#error Unknown architecture.
#endif

int64_t
ArchTicksToNanoseconds(uint64_t nTicks)
{
#if defined(ARCH_OS_LINUX)
    return nTicks; // clock_gettime() already returns nanoseconds
#else
    return int64_t(static_cast<double>(nTicks)*Arch_NanosecondsPerTick + .5);
#endif
}

double
ArchTicksToSeconds(uint64_t nTicks)
{
    return double(ArchTicksToNanoseconds(nTicks)) / 1e9;
}

uint64_t
ArchSecondsToTicks(double seconds) {
    return static_cast<uint64_t>(1.0e9 * seconds / ArchGetNanosecondsPerTick());
}

double 
ArchGetNanosecondsPerTick() 
{
    return Arch_NanosecondsPerTick;
}

PXR_NAMESPACE_CLOSE_SCOPE
