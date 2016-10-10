/*
 * A Remote Debugger for SpiderMonkey Java Script engine.
 * Copyright (C) 2014-2015 Sławomir Wojtasiak
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "timestamp.hpp"
#include "time.h"

#ifdef _WIN32
#include <Windows.h>
#include <stdint.h>

namespace {
    static_assert(2 * sizeof(DWORD) == sizeof(int64_t), "");
    static const int CLOCK_REALTIME = 0;
    // Quick and dirty implementation of clock_gettime(2) for Windows. We could
    // switch to QueryPerformanceCounter if a higher resolution is needed.
    int clock_gettime( int unused, timespec *tp ) {
        (void)unused;

        int64_t time;
        GetSystemTimeAsFileTime( reinterpret_cast<FILETIME*>(&time) );
        time -= 116444736000000000LL;    // Jan 1st 1601 -> Jan 1st 1970
        tp->tv_sec  = time / 10000000LL;
        tp->tv_nsec = time % 10000000LL * 100;
        return 0;
    }
}
#endif

using namespace Utils;

TimeStamp::TimeStamp() {
    _ts = current();
}

TimeStamp::TimeStamp( const TimeStamp &ts ) {
    _ts = ts._ts;
}

TimeStamp::TimeStamp(uint64_t nanos) {
    _ts.tv_sec = nanos / 1000000000;
    _ts.tv_nsec = nanos % 1000000000;
}

TimeStamp::TimeStamp(timespec &ts) {
    _ts = ts;
}

TimeStamp::~TimeStamp() {
}

TimeStamp TimeStamp::ns( uint64_t ns ) {
    return TimeStamp( ns );
}

TimeStamp TimeStamp::ms( uint64_t ms ) {
    return TimeStamp( ms * 1000000 );
}

TimeStamp TimeStamp::mi( uint64_t mi ) {
    return TimeStamp( mi * 1000 );
}

timespec TimeStamp::current() {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts;
}

TimeStamp& TimeStamp::operator=(TimeStamp b) {
    if (this != &b) {
        _ts = b._ts;
    }
    return *this;
}

TimeStamp TimeStamp::operator+(TimeStamp b) {
    return TimeStamp( this->getNanos() + b.getNanos() );
}

TimeStamp TimeStamp::operator-(TimeStamp b) {
    return TimeStamp( this->getNanos() - b.getNanos() );
}

timespec& TimeStamp::getTs() {
    return _ts;
}

uint64_t TimeStamp::getNanos() {
    return _ts.tv_sec * 1000000000 + _ts.tv_nsec;
}

uint64_t TimeStamp::getMicros() {
    return getNanos() / 1000;
}

uint64_t TimeStamp::getMilis() {
    return getNanos() / 1000000;
}




