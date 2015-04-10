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

#ifndef SRC_TIMESTAMP_H_
#define SRC_TIMESTAMP_H_

#include <stdint.h>
#include <time.h>

namespace Utils {

/**
 * Represents time stamp with the nanosecond precision. The default
 * implementation is based on the real time clock. This time stamp
 * class is able to represent time period as well as a standard UNIX
 * time.
 */
class TimeStamp {
public:
    TimeStamp();
    TimeStamp( const TimeStamp &ts );
    TimeStamp(uint64_t nanos);
    TimeStamp(timespec &ts);
    ~TimeStamp();
    static timespec current();
    static TimeStamp ns( uint64_t ns );
    static TimeStamp ms( uint64_t ms );
    static TimeStamp mi( uint64_t mi );
public:
    TimeStamp& operator =(TimeStamp b);
    TimeStamp operator +(TimeStamp b);
    TimeStamp operator -(TimeStamp b);
    timespec& getTs();
    uint64_t getNanos();
    uint64_t getMicros();
    uint64_t getMilis();
private:
    timespec _ts;
};

}

#endif /* SRC_TIMESTAMP_H_ */

