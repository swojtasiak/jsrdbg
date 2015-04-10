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

#ifndef SRC_FSEVENTS_HPP_
#define SRC_FSEVENTS_HPP_

#include <vector>
#include <streambuf>
#include <iostream>
#include <stdint.h>

#include "events.hpp"

#define FD_MAX_LOCAL_BUFFER_SIZE       512
#define FD_MAX_BUFFER_SIZE             50 * 1024 * 1024

typedef std::vector<int8_t> byte_vector;

class IFSEventConsumer: public IEventConsumer {
public:
    IFSEventConsumer();
    virtual ~IFSEventConsumer();
public:
    /**
     * Returns file descriptor associated with the consumer.
     * @return Consumer file descriptor.
     */
    virtual int getConsumerFD() = 0;
    /**
     * Called by the FS aware event loop in order
     * to read data from the file descriptor.
     * @return Internal error code.
     */
    virtual int read() = 0;
};

class IFSEventProducer: public IEventProducer {
public:
    IFSEventProducer();
    virtual ~IFSEventProducer();
public:
    /**
     * Returns file descriptor associated with the producer.
     * @return Producer file descriptor.
     */
    virtual int getProducerFD() = 0;
    /**
     * Writes pending data into the managed file descriptor.
     * @return Internal error code.
     */
    virtual int write() = 0;
};

/**
 * Handles asynchronous reads from file descriptors
 * putting everything that have been read into the
 * internal byte buffer.
 */
class BufferedFSEventConsumer: public IFSEventConsumer {
public:
    BufferedFSEventConsumer();
    virtual ~BufferedFSEventConsumer();
public:
    virtual int read();
protected:
    /**
     * Interprets buffer containing information that just has been read.
     * This method is responsible for removing interpreted data from
     * the buffer directly. Notice that we are not interested in
     * protocol errors but anyway you can provoke the lower layers
     * just to close the consumer gently by returning false. This method
     * should invoke 'consume' in the end.
     * @param buffer Buffer with read data.
     * @return Internal error.
     */
    virtual int handleBuffer() = 0;
    /**
     * Gets reference to the consumer's input buffer.
     * @return Buffer's reference.
     */
    byte_vector& getConsumerBuffer();
private:
    // Input byte buffer associated with consumer.
    byte_vector _consumerBuffer;
};

class BufferedFSEventProducer: public IFSEventProducer {
public:
    BufferedFSEventProducer();
    virtual ~BufferedFSEventProducer();
public:
    virtual int write();
    virtual bool isReady();
protected:
    /**
     * This method is called by the implementation of the 'write'
     * method just to prepare data for producer's internal buffer.
     * The buffer is then used as a source of bytes when data
     * is written into the managed file descriptor.
     * @return Internal error.
     */
    virtual int prepareBuffer() = 0;
    /**
     * Returns reference to internal producer's buffer.
     * @return Buffer's reference.
     */
    byte_vector& getProducerBuffer();
private:
    // Everything waiting to be written is held here.
    byte_vector _producerBuffer;
};

class FSEventLoop : public AbstractEventLoop<IFSEventProducer, IFSEventConsumer> {
public:
    /**
     * Creates an event loop instance for given
     * list of consumers and producers.
     */
    FSEventLoop( std::vector<IFSEventProducer*> producers, std::vector<IFSEventConsumer*> consumers );
    virtual ~FSEventLoop();
    virtual int loop();
    virtual void abort();
private:
    bool _running;
};

#endif /* SRC_FSEVENTS_HPP_ */
