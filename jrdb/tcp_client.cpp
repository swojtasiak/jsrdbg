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

#include <stdio.h>
#include <vector>
#include <string.h>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sstream>

#include "errors.hpp"
#include "tcp_client.hpp"
#include <log.hpp>

#define QUEUE_GUARD     1024

using namespace std;
using namespace Utils;

ClientDisconnectedEvent::ClientDisconnectedEvent( int clientId, int reason ) {
    this->_clientId = clientId;
    this->_reason = reason;
}

ClientDisconnectedEvent::~ClientDisconnectedEvent() {
}

int ClientDisconnectedEvent::getClientId() {
    return _clientId;
}

int ClientDisconnectedEvent::getReason() {
    return _reason;
}

TCPClient::TCPClient( int socket )
    : _socket( socket),
      _eventHandler(nullptr) {
}

TCPClient::~TCPClient() {
}

bool TCPClient::consume( IEvent *event ) {
    if( _eventHandler ) {
        _eventHandler->handle(event);
    }
    return true;
}

void TCPClient::closeConsumer( int error ) {
    disconnect(error);
}

int TCPClient::getConsumerFD() {
    return _socket;
}

int TCPClient::handleBuffer() {

    // Looking for a command separator.
    byte_vector &buffer = getConsumerBuffer();

    for( byte_vector::size_type i = 0; i < buffer.size(); i++ ) {
        int c = buffer[i];
        if( c == '\n' ) {
            // Separator has been found, so extract a command.
            stringstream buff;
            for( byte_vector::size_type j = 0; j < i; j++ ) {
                buff << buffer[j];
            }
            // Remove command from the buffer.
            buffer.erase(buffer.begin(), buffer.begin() + i + 1);
            DebuggerCommandEvent *event = new DebuggerCommandEvent(buff.str());
            consume( event );
            // Reset counter.
            i = 0;
        } else if( c == '\0' ) {
            // Protocol doesn't allow zeroes.
            return JDB_ERROR_MALICIOUS_DATA;
        }
    }

    return JDB_ERROR_NO_ERROR;
}

IEvent *TCPClient::produce() {
    // Not used, everything is placed inside 'prepareBuffer'.
    return nullptr;
}

void TCPClient::closeProducer( int error ) {
    disconnect(error);
}

int TCPClient::getProducerFD() {
    return _socket;
}

int TCPClient::prepareBuffer() {
    if( !_outgoingCommands.empty() ) {
        vector<int8_t> &buffer = getProducerBuffer();
        // Put commands directly into the destination buffer.
        while( !_outgoingCommands.empty() ) {
            DebuggerCommand &cmd = _outgoingCommands.front();
            // Copy command to the output buffer.
            string content = cmd.getContent();
            if( cmd.getContextId() != -1 ) {
                stringstream ss;
                ss << cmd.getContextId() << '/' << content;
                content = ss.str();
            }
            if( content.size() + buffer.size() + 1 < FD_MAX_BUFFER_SIZE ) {
                for(string::const_iterator it = content.begin(); it != content.end(); ++it) {
                    buffer.push_back( *it );
                }
                buffer.push_back( '\n' );
            } else {
                LoggerFactory::getLogger().error("Output buffer is full, command ignored.");
            }
            _outgoingCommands.pop();
        }
    }
    return JDB_ERROR_NO_ERROR;
}

void TCPClient::sendCommand( const DebuggerCommand &cmd ) {
    if( _outgoingCommands.size() < QUEUE_GUARD ) {
        _outgoingCommands.push( cmd );
    } else {
        LoggerFactory::getLogger().error("Outgoing command queue is full.");
    }
}

void TCPClient::setEventHandler( IEventHandler *eventHandler ) {
    _eventHandler = eventHandler;
}

void TCPClient::disconnect(int error) {
    ::close( _socket );
    if( _socket && _eventHandler ) {
        _eventHandler->handle(new ClientDisconnectedEvent(_socket, error));
    }
    _socket = 0;
}

int TCPClient::Connect( const std::string host, int port, TCPClient **client ) {

    Logger &log = LoggerFactory::getLogger();

    int socket;

    struct sockaddr_in clientIp;

    if( ( socket = ::socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) {
        log.error("Cannot create server socket: %d", errno);
        return JDB_ERROR_CANNOT_CREATE_SOCKET;
    }

    struct hostent *hostinfo;
    clientIp.sin_family = AF_INET;
    clientIp.sin_port = htons ( port );
    hostinfo = ::gethostbyname ( host.c_str() );
    if ( !hostinfo ) {
        log.error("gethostbyname failed for %s with %d.", host.c_str(), errno);
        ::close( socket );
        return JDB_ERROR_CANNOT_RESOLVE_HOST_NAME;
    }

    ::memcpy( &(clientIp.sin_addr.s_addr), hostinfo->h_addr, hostinfo->h_length );

    if( ::connect(socket, (struct sockaddr *)&clientIp, sizeof(clientIp) ) < 0 ) {
        ::close( socket );
        log.error("Cannot connect to the debugger: %d", errno);
        return JDB_ERROR_CANNOT_CONNECT;
    }

    // Set socket to nonblocking.
    int flags;
    if ( ( ( flags = ::fcntl( socket, F_GETFL, 0 ) ) < 0)
            || ( ::fcntl( socket, F_SETFL, flags | O_NONBLOCK ) < 0 ) ) {
        ::close( socket );
        log.error( "fcntl failed with error: %d.", errno );
        return JDB_ERROR_CANNOT_SET_SOCKET_NONBLOCK;
    }

    *client = new TCPClient( socket );

    return JDB_ERROR_NO_ERROR;
}
