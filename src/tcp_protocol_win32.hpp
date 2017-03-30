/*
 * A Remote Debugger for SpiderMonkey Java Script engine.
 * Copyright (C) 2016 otris software
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

#ifndef JSR_SRC_TCP_PROTOCOL_WIN32_H_
#define JSR_SRC_TCP_PROTOCOL_WIN32_H_

#ifndef _WIN32
#error "This file should only be included on a Windows platform"
#endif

#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")

#include <stdint.h>

#include <string>

#include <jsrdbg.h>
#include "client.hpp"
#include "protocol.hpp"
#include <threads.hpp>
#include <utils.hpp>
#include <log.hpp>

namespace JSR {

class TCPClientWin32 final : public Client, protected Utils::QueueSignalHandler<Command>, protected Utils::NonCopyable {
public:

    TCPClientWin32( const JSRemoteDebuggerCfg &cfg, SOCKET clientSocket );
    ~TCPClientWin32();

public:

    /**
     * Reads data from the client socket and puts it into the
     * read buffer which is then interpreted in order to identity
     * incoming commands. By default commands are separated
     * by new line characters.
     */
    int recv();

    /**
     * Sends pending data through the client socket.
     */
    int send();

    /**
     * Handles read and write buffers. Read buffers are interpreted and
     * converted into input commands, while write buffers are converted to
     * output commands which are to be send to the sockets.
     */
    bool handleBuffers();

    /**
     * Closes socket associated with the client.
     */
    void closeSocket();

    /**
     * Gets client's socket.
     */
    SOCKET getSocket() const;

    HANDLE getWriteEvent() const;

    // Client.
    void disconnect();
    bool isConnected();

private:
    /**
     * Every client is also a signal handler. Signals are sent
     * by the BlockingQueues just in order to inform interested parts
     * about new elements put in them. In case of TCP client such a signal
     * is then sent through the pipe just to wake up the main thread
     * responsible for carrying out the network transmission.
     */
    void handle( Utils::BlockingQueue<Command> &queue, int signal );

    /**
     * Adds bytes to client's read buffer.
     */
    int fillReadBuffer( char *buffer, int offset, size_t size );

    Utils::Logger &_log;
    Utils::Mutex _mutex;
    // Buffers used for buffering commands which are reading now.
    std::string _readBuffer;
    std::string _writeBuffer;
    // Client socket.
    SOCKET _socket;
    WSAEVENT _writeEvent;
    bool _closed;
    const JSRemoteDebuggerCfg &_cfg;
};

/**
 * Class responsible for handling TCP/IP based communication model.
 */
class TCPProtocolWin32 final : public Protocol, protected Utils::Runnable {
public:

    TCPProtocolWin32( ClientManager &clientManager,
        Utils::QueueSignalHandler<Command> &commandHandler,
        const JSRemoteDebuggerCfg &cfg );
    ~TCPProtocolWin32();

    static void sendCommand( HANDLE writePipe, uint8_t command, uint32_t args );

private:
    int init();
    int startProtocol();
    int stopProtocol();
    ClientManager& getClientManager();

    void run();
    void interrupt();
    TCPClientWin32* acceptClient();
    void disposeClient(TCPClientWin32* client, std::vector<WSAEVENT>& events);

private:
    Utils::Logger &_log;
    ClientManager &_clientManager;
    JSRemoteDebuggerCfg _cfg;
    SOCKET _serverSocket;
    Utils::Thread _thread;
    Utils::QueueSignalHandler<Command> &_inCommandHandler;
    HANDLE _stopEvent;
    WSAEVENT _networkEvent;
};

}

#endif /* JSR_SRC_TCP_PROTOCOL_WIN32_H_ */
