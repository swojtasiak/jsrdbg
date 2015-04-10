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

#include "utils.hpp"
#include <string.h>
#include <algorithm>

#include "threads.hpp"

using namespace Utils;
using namespace std;

/********
 * Event
 ********/

Event::Event( int code )
    : _code(code),
      _returnCode(0) {
}

Event::~Event() {
}

int Event::getCode() {
    return _code;
}

void Event::setReturnCode( int code ) {
    this->_returnCode = code;
}

int Event::getReturnCode() {
    return _returnCode;
}

/***************
 * EventHandler
 ***************/

EventHandler::EventHandler() {
}

EventHandler::~EventHandler() {
}

/***************
 * EventEmitter
 ***************/

EventEmitter::EventEmitter() {
    _mutex = new Mutex();
}

EventEmitter::~EventEmitter() {
    delete _mutex;
}

void EventEmitter::addEventHandler( EventHandler *handler ) {
    MutexLock lock(*_mutex);
    _eventHandlers.push_back( handler );
}

void EventEmitter::removeEventHandler( EventHandler *handler ) {
    // Remove given handler if there is any.
    MutexLock lock(*_mutex);
    std::vector<EventHandler*>::iterator it = find( _eventHandlers.begin(), _eventHandlers.end(), handler );
    if( it != _eventHandlers.end() ) {
        _eventHandlers.erase( it );
    }
}

void EventEmitter::fire( Event &event ) {
    // Send given event to all the registered event handlers.
    _mutex->lock();
    std::vector<EventHandler*> handlers = _eventHandlers;
    _mutex->unlock();
    for( std::vector<EventHandler*>::iterator it = handlers.begin(); it != handlers.end(); it++ ) {
        (*it)->handle( event );
    }
}

void EventEmitter::fire( int code ) {
    // Send given event to all the registered event handlers.
    Event event( code );
    fire( event );
}

