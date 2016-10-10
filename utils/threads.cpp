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

#include <pthread.h>
#include <stdexcept>
#include <time.h>
#include <errno.h>
#include <stdint.h>

#include "threads.hpp"
#include "timestamp.hpp"

using namespace std;
using namespace Utils;

/* Interruption exception. */

InterruptionException::InterruptionException() {
}

InterruptionException::~InterruptionException() {
}

/* Runnable */

Runnable::Runnable() {
}

Runnable::~Runnable() {
}

void Runnable::interrupt() {
    // Default implementation does nothing.
}

/* Thread */

static void *JRS_Thread_Start_Routine( void *args ) {
    Runnable *runnable = static_cast<Runnable*>( args );
    if( runnable ) {
        try {
            runnable->run();
        } catch(...) {
            // Runnable is responsible for handling exceptions, so
            // we are ignoring everything that arrives here. It's not
            // the best way to handle it, but as a shared library without
            // any logging mechanism it's everything we can do here.
        }
    }
    // We are not interested in the result.
    pthread_exit(NULL);
}

Thread::Thread(Runnable &runnable)
    : _runnable(runnable),
      _started( false ) {
}

Thread::~Thread() {
}

void Thread::start() {

    _mutex.lock();

    if( _started ) {
        _mutex.unlock();
        throw runtime_error("Thread has been already started.");
    }

    pthread_attr_t attr;

    if( pthread_attr_init(&attr) ) {
        _mutex.unlock();
        throw runtime_error("Cannot initialize thread attributes.");
    }

    if( pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) ) {
        pthread_attr_destroy(&attr);
        _mutex.unlock();
        throw runtime_error("Cannot make the thread a joinable one.");
    }

    if( pthread_create( &_thread, &attr, &JRS_Thread_Start_Routine, &_runnable ) ) {
        pthread_attr_destroy(&attr);
        _mutex.unlock();
        throw runtime_error("Cannot start thread.");
    }

    _started = true;

    pthread_attr_destroy(&attr);

    _mutex.unlock();
}

void Thread::interrupt() {
    if( isStarted() ) {
        _runnable.interrupt();
    }
}

void Thread::join() {
    if( isStarted() ) {
        void *result;
        pthread_join( _thread, &result );
    }
}

bool Thread::isStarted() {
    bool started = false;
    _mutex.lock();
    started = _started;
    _mutex.unlock();
    return started;
}

/*
 * First of all - these synchronization primitives are fairly likely to fail, so
 * the absence of the error handling is intended here. It's just a debugger
 * not a real time critical transaction processing system so it's acceptable.
 * Notice that libraries such as Qt follow the same model. In these cases I would
 * call it unacceptable, because they are very generic in nature and can be used
 * to handle anything. At last they was intended to be used for anything...
 */

/**
 * Reentrant mutex.
 */

Mutex::Mutex() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init( &_mutex, &attr );
    pthread_mutexattr_destroy(&attr);
}

Mutex::~Mutex() {
    pthread_mutex_destroy( &_mutex );
}

void Mutex::lock() {
    pthread_mutex_lock( &_mutex );
}

void Mutex::unlock() {
    pthread_mutex_unlock( &_mutex );
}

/**
 * Mutex locker.
 */

MutexLock::MutexLock( Mutex &mutex )
    : _mutex(mutex) {
    _mutex.lock();
}

MutexLock::~MutexLock() {
    _mutex.unlock();
}

/**
 * Condition.
 */

Condition::Condition() {
    pthread_cond_init( &_condition, NULL );
}

Condition::~Condition() {
    pthread_cond_destroy( &_condition );
}

void Condition::signal() {
    pthread_cond_signal( &_condition );
}

void Condition::broadcast() {
    pthread_cond_broadcast( &_condition );
}

void Condition::wait( Mutex &mutex ) {
    pthread_cond_wait( &_condition, &mutex._mutex );
}

bool Condition::wait( Mutex &mutex, int milis ) {
    TimeStamp ts = TimeStamp() + TimeStamp::ms( milis );
    return pthread_cond_timedwait( &_condition, &mutex._mutex, &ts.getTs() ) != ETIMEDOUT;
}
