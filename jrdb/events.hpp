/*
 * A Remote Debugger Client for SpiderMonkey Java Script Engine.
 * Copyright (C) 2014-2015 Slawomir Wojtasiak
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef SRC_EVENTS_HPP_
#define SRC_EVENTS_HPP_

#include <vector>
#include <string>

#include "errors.hpp"

class IEvent {
public:
    IEvent();
    virtual ~IEvent();
};

class StringEvent : public IEvent {
public:
    StringEvent( const std::string &str );
    virtual ~StringEvent();
    const std::string &str();
private:
    std::string _str;
};

class IEventHandler {
public:
    IEventHandler();
    virtual ~IEventHandler();
    virtual void handle( IEvent *event ) = 0;
};

class IEventLoop {
public:
    IEventLoop();
    virtual ~IEventLoop();
    /**
     * Starts event's loop.
     */
    virtual int loop() = 0;
    /**
     * Breaks the event's loop.
     */
    virtual void abort() = 0;
};

class IEventProducer {
public:
    IEventProducer();
    virtual ~IEventProducer();
    /**
     * Get next event in the queue to be sent. Returns
     * null if there isn't any. This method is not blocking one.
     * @return Event.
     */
    virtual IEvent *produce() = 0;
    /**
     * Returns true if there is something to write for this
     * producer. This method is here just in order to make a
     * possibility of introducing laziness into the'produce'
     * method.
     * @return True if there is something to be written.
     */
    virtual bool isReady() = 0;
    /**
     * Closes producer and disposes all resources
     * associated with it. After calling this method
     * producer instance is not usable anymore and
     * should be deleted.
     */
    virtual void closeProducer( int error ) = 0;
};

class IEventConsumer {
public:
    IEventConsumer();
    virtual ~IEventConsumer();
   /**
    * Consumes incoming event. It's not blocking
    * method.
    * @param event Event to be consumed.
    * @return True if event could be consumed.
    */
    virtual bool consume( IEvent *event ) = 0;
    /**
     * Closes consumer and disposes all resources
     * associated with it. After calling this method
     * consumer instance is not usable anymore and
     * should be deleted.
     */
    virtual void closeConsumer( int error ) = 0;
};

template<typename P, typename C>
class AbstractEventLoop : public IEventLoop {
public:
    /**
     * Creates an event loop instance for given
     * list of consumers and producers.
     */
    AbstractEventLoop( std::vector<P*> producers, std::vector<C*> consumers ) :
        _producers(producers),
        _consumers(consumers) {
    }
    virtual ~AbstractEventLoop() {
    }
protected:
    // A list of all known producers.
    std::vector<P*> _producers;
    // A list of all known consumers.
    std::vector<C*> _consumers;
};

#endif /* SRC_EVENTS_HPP_ */
