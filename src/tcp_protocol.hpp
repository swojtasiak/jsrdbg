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

#ifndef JSR_SRC_TCP_PROTOCOL_H_
#define JSR_SRC_TCP_PROTOCOL_H_

#include <stdint.h>
#include <map>
#include <vector>
#include <string>

#include <jsrdbg.h>
#include "client.hpp"
#include "protocol.hpp"
#include <threads.hpp>
#include <utils.hpp>
#include <log.hpp>

namespace JSR {

class TCPClient : public Client, protected Utils::QueueSignalHandler<Command>, protected Utils::NonCopyable {
public:

    TCPClient( const JSRemoteDebuggerCfg &cfg, int socket, int pipe );
    virtual ~TCPClient();

public:

    /**
     * Reads data from the client socket and puts it into the
     * read buffer which is then interpreted in order to identity
     * incoming commands. By default commands are separated
     * by new line characters.
     */
    int recv();

    /**
     * Sets pending data through the client socket.
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
    int getSocket() const;

    // Client.
    void disconnect();
    bool isConnected();

protected:

    /**
     * Every client is also a signal handler. Signals are sent
     * by the BlockingQueues just in order to inform interested parts
     * about new elements put in them. In case of TCP client such a signal
     * is then sent through the pipe just to wake up the main thread
     * responsible for carrying out the network transmission.
     */
    void handle( Utils::BlockingQueue<Command> &queue, int signal );

private:

    /**
     * Sends command through pipe.
     */
    void sendCommand( uint8_t command, uint32_t args );

    /**
     * Adds bytes to client's read buffer.
     */
    int fillReadBuffer( char *buffer, int offset, size_t size );

protected:
    // Shared logger.
    Utils::Logger &_log;
private:
    // Used for internal state synchronization.
    Utils::Mutex _mutex;
    // Buffers used for buffering commands which are reading now.
    std::string _readBuffer;
    std::string _writeBuffer;
    // TCP/IP socket.
    int _socket;
    // Pipe used to inform the main thread
    int _pipe;
    // True if client is already closed and shouldn't
    // handle any communication.
    bool _closed;
    // Configuration.
    const JSRemoteDebuggerCfg &_cfg;
};

/**
 * Class responsible for handling TCP/IP based communication model.
 */
class TCPProtocol : public Protocol, protected Utils::Runnable {
public:

    /* Pipe commands. */
    enum Commands {
       JSR_TCP_PIPE_COMMAND_WRITE = 1,
       JSR_TCP_PIPE_COMMAND_DISCONNECT,
       JSR_TCP_PIPE_COMMAND_EXIT
    };

    enum ClearBitSet {
        CBS_READ,
        CBS_WRITE,
        CBS_ALL
    };

    TCPProtocol( ClientManager &clientManager, Utils::QueueSignalHandler<Command> &commandHandler, const JSRemoteDebuggerCfg &cfg );
    virtual ~TCPProtocol();
public:
    virtual int init();
    virtual int startProtocol();
    virtual int stopProtocol();
    virtual ClientManager& getClientManager();
    static void sendCommand( int pipe, uint8_t command, uint32_t args );
protected:
    void run();
    void interrupt();
private:
    // PIPE commands.
    void commandMarkWrite( int socket, fd_set &write_fds, int &fdmax );
    void commandDisconnectClient( int socket, fd_set &read_fds, fd_set &write_fds, int &fdmax );
private:
    void clearFD( int fd, ClearBitSet setType, fd_set &read_fds, fd_set &write_fds, int &fdmax );
    void closeSocket( int socket, fd_set &read_fds, fd_set &write_fds, int &fdmax );
    void acceptClient( fd_set &read_fd, int &fdmax );
    void disposeClient( TCPClient *client, fd_set &read_fds, fd_set &write_fds, int &fdmax );
    bool recvCommand(int pipe, uint8_t &command, uint32_t &arg);
protected:
    Utils::Logger &_log;
private:
    ClientManager &_clientManager;
    JSRemoteDebuggerCfg _cfg;
    int _serverSocket;
    int _pipefd[2];
    Utils::Thread _thread;
    Utils::QueueSignalHandler<Command> &_inCommandHandler;
};

}

#endif /* JSR_SRC_TCP_PROTOCOL_H_ */
