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

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <stdexcept>
#include <stddef.h>
#include <vector>

namespace Utils {

/**
 * Base class for all events.
 */
class Event {
public:
    Event( int code );
    virtual ~Event();
public:
    virtual void setReturnCode( int code );
    virtual int getReturnCode();
    virtual int getCode();
private:
    int _code;
    int _returnCode;
};

/**
 * Generic event handler which can be used to handle almost
 * every kind of events in the system environment.
 */
class EventHandler {
public:
    EventHandler();
    virtual ~EventHandler();
public:
    virtual void handle( Event &event ) = 0;
};

class Mutex;

/**
 * Inherit from this class in order to add events firing
 * related functionality to your own class.
 * @warning This class is thread safe.
 */
class EventEmitter {
public:
    EventEmitter();
    virtual ~EventEmitter();
public:
    /**
     * Adds new events handler.
     */
    void addEventHandler( EventHandler *handler );
    /**
     * Removed given event handler.
     */
    void removeEventHandler( EventHandler *handler );
protected:
    /**
     * Fires given event to all the registered event handlers.
     * @param event Event to be fired.
     */
    void fire( Event &event );
    /**
     * Fires standard CodeEvent to all registered clients.
     * @param Code to be fired.
     */
    void fire( int code );
private:
    // Registered event handlers.
    Mutex *_mutex;
    std::vector<EventHandler*> _eventHandlers;
};

/**
 * Just inherit from this class to make your class non copyable.
 */
class NonCopyable {
protected:
    NonCopyable() {
    }
private:
    /**
    * @throw OperationNotSupportedException
    */
    NonCopyable( const NonCopyable &cpy ) {
        throw std::runtime_error("Operation not supported.");
    }
    /**
    * @throw OperationNotSupportedException
    */
    NonCopyable& operator=( const NonCopyable &exc ) {
        throw std::runtime_error("Operation not supported.");
    }
};

template<typename T>
class AutoPtr {
public:
    AutoPtr( T* ptr = NULL ) : _ptr(ptr) {
    }
    ~AutoPtr() {
        if( _ptr ) {
            delete _ptr;
        }
    }
    AutoPtr& operator=(AutoPtr<T> &other) {
        if( &other != this ) {
            _ptr = other._ptr;
        }
        return *this;
    }
    AutoPtr& operator=(T* other) {
        if( other != _ptr ) {
            _ptr = other;
        }
        return *this;
    }
    T& operator*() const {
        return *_ptr;
    }
    T* operator&() const {
        return _ptr;
    }
    T* operator->() const {
        return _ptr;
    }
    operator bool() {
        return _ptr != NULL;
    }
    operator T*() {
        return _ptr;
    }
    operator T&() {
        return *_ptr;
    }
    void reset() {
        _ptr = NULL;
    }
    T* get() {
        return _ptr;
    }
    T* release() {
        T *tmp = NULL;
        if( _ptr ) {
            tmp = _ptr;
            _ptr = NULL;
        }
        return tmp;
    }
private:
    T *_ptr;
};

}

#endif /* SRC_UTILS_H_ */
