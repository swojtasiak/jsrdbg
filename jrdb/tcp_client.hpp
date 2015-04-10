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

#ifndef SRC_TCP_CLIENT_HPP_
#define SRC_TCP_CLIENT_HPP_

#include <string>
#include <queue>

#include "fsevents.hpp"
#include "debugger.hpp"

class ClientDisconnectedEvent : public IEvent {
public:
    ClientDisconnectedEvent( int clientId, int reason );
    virtual ~ClientDisconnectedEvent();
    int getClientId();
    int getReason();
private:
    int _clientId;
    int _reason;
};

class TCPClient : public BufferedFSEventConsumer, public BufferedFSEventProducer {
private:
    TCPClient( int socket );
public:
    virtual ~TCPClient();

    void setEventHandler( IEventHandler *eventHandler );

    /**
     * Closes both consumer and producer.
     */
    void disconnect(int error);

    // IEventConsumer
    virtual bool consume( IEvent *event );
    virtual void closeConsumer( int error );

    // IFSEventConsumer
    virtual int getConsumerFD();

    // BufferedFSEventConsumer
    virtual int handleBuffer();

    // IEventProducer
    virtual IEvent *produce();
    virtual void closeProducer( int error );

    // IFSEventProducer
    virtual int getProducerFD();

    // BufferedFSEventProducer
    virtual int prepareBuffer();

    // TCPClient
    void sendCommand( const DebuggerCommand &cmd );

    /**
     * Connects to the debugger. It allocated a new client object after connection.
     * If anything failed, internal error code is returned. Anyway every returned
     * error has errno associated with it, so fell free to use errno to get error
     * description if function fails.
     * @param host Host or IP.
     * @param port Debugger port.
     * @param[out] client Client connected to the debugger.
     * @return Internal error code.
     */
    static int Connect( const std::string host, int port, TCPClient **client );

private:
    // Outgoing commands.
    std::queue<DebuggerCommand> _outgoingCommands;
    // TCP socket.
    int _socket;
    // Handles incoming commands.
    IEventHandler *_eventHandler;
};

#endif /* SRC_TCP_CLIENT_HPP_ */
