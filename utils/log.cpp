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

#include "log.hpp"

#include <stdarg.h>
#include <syslog.h>
#include <stdio.h>

using namespace Utils;
using namespace std;

/* Logging. */

Logger::Logger() {
}

Logger::~Logger() {
}

/* Unix logger. */

class UnixLogger : public Logger {
public:

    UnixLogger() {
        openlog( "jsrdbg", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER );
    };

    virtual ~UnixLogger() {
        closelog();
    }

    enum Level {
        UL_INFO,
        UL_DEBUG,
        UL_WARN,
        UL_ERROR
    };

    void debug( const std::string log, ... ) {
        va_list args;
        va_start( args, log );
        logMessage( UL_DEBUG, log, args );
        va_end( args );
    }

    void info( const std::string log, ... ) {
        va_list args;
        va_start( args, log );
        logMessage( UL_INFO, log, args );
        va_end( args );
    }

    void warn( const std::string log, ... ) {
        va_list args;
        va_start( args, log );
        logMessage( UL_WARN, log, args );
        va_end( args );
    }

    void error( const std::string log, ... ) {
        va_list args;
        va_start( args, log );
        logMessage( UL_ERROR, log, args );
        va_end( args );
    }

private:

    void logMessage( Level level, const string log, va_list args ) {
        char buffer[1024];
        vsnprintf( buffer, sizeof( buffer ), log.c_str(), args );
        int priority = 0;
        switch ( level ) {
        case UL_DEBUG:
            priority = LOG_DEBUG;
            break;
        case UL_INFO:
            priority = LOG_INFO;
            break;
        case UL_WARN:
            priority = LOG_WARNING;
            break;
        case UL_ERROR:
            priority = LOG_ERR;
            break;
        }
        syslog( priority, "%s", buffer );
    }
};

Logger& LoggerFactory::getLogger() {
    static UnixLogger log;
    return log;
}


