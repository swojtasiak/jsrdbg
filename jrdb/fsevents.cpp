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

#include <vector>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <log.hpp>
#include "fsevents.hpp"

using namespace std;
using namespace Utils;

FSEventLoop::FSEventLoop( std::vector<IFSEventProducer*> producers, std::vector<IFSEventConsumer*> consumers )
    : AbstractEventLoop( producers, consumers ),
      _running( false ) {
}

FSEventLoop::~FSEventLoop() {
}

void FSEventLoop::abort() {
    // Break the loop when possible. This method should be called from
    // the handlers invoked by the events fired by the event's loop in
    // order to avoid the need to interrupt the select function.
    _running = false;
}

int FSEventLoop::loop() {

    int error = JDB_ERROR_NO_ERROR;

    Logger &log = LoggerFactory::getLogger();

    fd_set read_fds;
    fd_set write_fds;
    int fdmax = 0;

    FD_ZERO(&write_fds);
    FD_ZERO(&read_fds);

    _running = true;

    while( _running ) {

        int rc;

        fdmax = 0;

        // Prepare bits for incoming events.
        FD_ZERO(&read_fds);
        for( std::vector<IFSEventConsumer*>::iterator it = _consumers.begin(); it != _consumers.end(); it++ ) {
            IFSEventConsumer *consumer = *it;
            int fd = consumer->getConsumerFD();
            FD_SET( fd, &read_fds );
            if( fd > fdmax ) {
                fdmax = fd;
            }
        }

        // Prepare outgoing flags basing on the current state of the producers.
        FD_ZERO(&write_fds);
        for( std::vector<IFSEventProducer*>::iterator it = _producers.begin(); it != _producers.end(); it++ ) {
            IFSEventProducer *producer = *it;
            if( producer->isReady() ) {
                int fd = producer->getProducerFD();
                FD_SET( fd, &write_fds );
                if( fd > fdmax ) {
                    fdmax = fd;
                }
            }
        }

        if( ( rc = select( fdmax + 1, &read_fds, &write_fds, NULL, NULL ) ) == -1 ) {
            cout << "Select failed " << errno << " " << strerror( errno ) << endl;
            error = JDB_ERROR_SELECT_FAILED;
            break;
        }

        for( int i = 0; i <= fdmax; i++) {

            // Write data.
            if( FD_ISSET( i, &write_fds ) ) {
                // Find producer for given socket.
                for( std::vector<IFSEventProducer*>::iterator it = _producers.begin(); it != _producers.end(); ) {
                   if( (*it)->getProducerFD() == i ) {
                       int error = (*it)->write();
                       if( error ) {
                           log.error("Consumer failed with error: %d", error);
                           (*it)->closeProducer( error );
                           _producers.erase(it);
                           continue;
                       }
                   }
                   it++;
                }
            }

            // Read data.
            if( FD_ISSET( i, &read_fds ) ) {
                // Find producer for given socket.
                for( std::vector<IFSEventConsumer*>::iterator it = _consumers.begin(); it != _consumers.end(); ) {
                   if( (*it)->getConsumerFD() == i ) {
                       int error = (*it)->read();
                       if( error && error != JDB_ERROR_WOULD_BLOCK ) {
                           log.error("Consumer failed with error: %d", error);
                           (*it)->closeConsumer( error );
                           _consumers.erase(it);
                           continue;
                       }
                   }
                   it++;
                }
            }

        }
    }

    return error;
}

/*********************
 * Buffered Consumer.
 *********************/

BufferedFSEventConsumer::BufferedFSEventConsumer() {
}

BufferedFSEventConsumer::~BufferedFSEventConsumer() {
}

int BufferedFSEventConsumer::read() {
    Logger& log = LoggerFactory::getLogger();
    std::vector<int8_t> &consumerBuffer = getConsumerBuffer();
    int8_t buffer[FD_MAX_LOCAL_BUFFER_SIZE];
    while( true ) {
        int available = FD_MAX_BUFFER_SIZE - consumerBuffer.size();
        int min = available > FD_MAX_LOCAL_BUFFER_SIZE ? FD_MAX_LOCAL_BUFFER_SIZE : available;
        if( min == 0 ) {
            // Sorry the buffer is full, client have to interpret it first.
            return JDB_ERROR_BUFFER_IS_FULL;
        }
        int rc = ::read( getConsumerFD(), buffer, min );
        if( rc < 0 ) {
            if( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) ) {
                // Do nothing, maybe next time something will be read.
                int error;
                if( ( error = handleBuffer() ) ) {
                    return error;
                }
                return JDB_ERROR_WOULD_BLOCK;
            } else {
                log.error( "read error %d %s while reading from consumer's file.", errno, strerror( errno ) );
                // Low level read error.
                return JDB_ERROR_READ_ERROR;
            }
        } else if( rc == 0 ) {
            // In this case we are not interested in returned value,
            // because the file descriptor is being closed anyway.
            handleBuffer();
            // Connection closed.
            return JDB_ERROR_FILE_DESCRIPTOR_CLOSED;
        } else {
            // Copy read data to the destination buffer. Notice that we do not
            // net the best performance possible here, so we can improve
            // code readability a make it less-error-prone.
            if( consumerBuffer.capacity() == 0 ) {
                consumerBuffer.reserve( consumerBuffer.size() + ( FD_MAX_LOCAL_BUFFER_SIZE * 10 ) );
            }
            for( int i = 0; i < rc; i++ ) {
                consumerBuffer.push_back( buffer[i] );
            }
        }
    }
}

vector<int8_t>& BufferedFSEventConsumer::getConsumerBuffer() {
    return _consumerBuffer;
}

/*********************
 * Buffered Producer.
 *********************/

BufferedFSEventProducer::BufferedFSEventProducer() {
}

BufferedFSEventProducer::~BufferedFSEventProducer() {
}

bool BufferedFSEventProducer::isReady() {
    return !prepareBuffer() && !_producerBuffer.empty();
}

int BufferedFSEventProducer::BufferedFSEventProducer::write() {
    Logger& log = LoggerFactory::getLogger();
    int error = JDB_ERROR_NO_ERROR;
    vector<int8_t> &producerBuffer = getProducerBuffer();
    error = prepareBuffer();
    if( error ) {
        return error;
    }
    int fd = getProducerFD();
    while( ( error == JDB_ERROR_NO_ERROR ) && !producerBuffer.empty() ) {
        int i;
        static uint8_t buffer[FD_MAX_LOCAL_BUFFER_SIZE];
        int bufferSize = producerBuffer.size();
        // Fill local buffer, just in order to allow write
        // whole block at once.
        for( i = 0; i < bufferSize && i < FD_MAX_LOCAL_BUFFER_SIZE; i++ ) {
            buffer[i] = producerBuffer[i];
        }
        int sent = 0;
        while( i ) {
            int rc = ::write( fd, buffer + sent, i  );
            if( rc < 0 ) {
                if( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) ) {
                    // Do nothing, maybe next time something will be written.
                    error = JDB_ERROR_WOULD_BLOCK;
                    break;
                } else {
                    log.error( "write failed with error %d %s.", errno, strerror( errno ) );
                    // Write failed.
                    error = JDB_ERROR_WRITE_ERROR;
                    break;
                }
            } else if( rc > 0 ) {
                i -= rc;
                sent += rc;
            }
        }
        if( sent > 0 ) {
            producerBuffer.erase( producerBuffer.begin(), producerBuffer.begin() + sent );
        }
    }
    return error;
}

vector<int8_t>& BufferedFSEventProducer::getProducerBuffer() {
    return _producerBuffer;
}

IFSEventConsumer::IFSEventConsumer() {
}

IFSEventConsumer::~IFSEventConsumer() {
}

IFSEventProducer::IFSEventProducer() {
}

IFSEventProducer::~IFSEventProducer() {
}

IEventHandler::IEventHandler() {
}

IEventHandler::~IEventHandler() {
}
