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

#include "tcp_protocol.hpp"

#include <timestamp.hpp>
#include <js_utils.hpp>

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

using namespace JSR;
using namespace std;
using namespace Utils;

#define JSR_TCP_MAX_CLIENTS_SUPPORTED       1
#define JSR_TCP_DEFAULT_PORT                8089
#define JSR_TCP_LOCAL_BUFFER                1024
#define JSR_TCP_DEFAULT_SEPARATOR           "\n"
#define JSR_TCP_DEFAULT_SEPARATOR_SIZE      sizeof( JSR_TCP_DEFAULT_SEPARATOR )

namespace JSR {

    /**
     * All commands separators supported by the protocol.
     */
    const char* COMMANDS_SEPARATORS[] = {
        "\r\n",
        "\n",
        nullptr
    };

}

/************
 * TCPClient
 ************/

TCPClient::TCPClient( const JSRemoteDebuggerCfg &cfg, int socket, int pipe ) :
        Client(socket),
        _log( LoggerFactory::getLogger() ),
        _socket(socket),
        _pipe(pipe),
        _closed(false),
        _cfg(cfg) {
    // We are interested in new commands from debugger in order
    // to inform the server that there is something to send.
    getOutQueue().setSignalHandler(this);
}

TCPClient::~TCPClient() {
    // Just to be sure that socket is closed.
    closeSocket();
}

// This method should be called by main protocol thread.
void TCPClient::closeSocket() {
    // Closes client's socket.
    if( _socket ) {
        ::close( _socket );
        _socket = 0;
    }
}

void TCPClient::disconnect() {
    // Sends disconnect signal to the asynchronous server loop.
    // We shouldn't disconnect any socket using different threads than
    // the main one which uses select to wait for events. Just in order not
    // to make select fail. Remember that invoking this method does't
    // set the client in 'disconnected' status.
    sendCommand(TCPProtocol::JSR_TCP_PIPE_COMMAND_DISCONNECT, _socket);
}

bool TCPClient::isConnected() {
    MutexLock lock(_mutex);
    bool connected = _socket != 0;
    return connected;
}

void TCPClient::handle( BlockingQueue<Command> &queue, int signal ) {
    // Send information about something to write through
    // the pipe which is scanned by main TCP thread.
    sendCommand(TCPProtocol::JSR_TCP_PIPE_COMMAND_WRITE, _socket);
}

// Internal API, not need to be synchronized. Used only inside thread-safe methods.
void TCPClient::sendCommand( uint8_t command, uint32_t args ) {
    TCPProtocol::sendCommand( _pipe, command, args );
}

int TCPClient::getSocket() const {
    return _socket;
}

// Internal API, not need to be synchronized. Used only inside
// thread-safe methods.
bool TCPClient::handleBuffers() {

    // Handles read buffer.
    size_t pos;

    const char **separators = COMMANDS_SEPARATORS;

    while (*separators) {

        const char *separator = *separators;

        if ((pos = _readBuffer.find(separator)) != string::npos) {

            // Adds received command into the queue of the commands
            // sent by the connected client.
            string commandStr = _readBuffer.substr(0, pos);

            // Check if there is context ID in the command string.
            int contextId = -1;
            if (!Utils::MozJSUtils::splitCommand(commandStr, contextId,
                    commandStr)) {
                _log.error( "TCPClient::handleBuffers: Broken context ID: %s",
                        commandStr.c_str() );
            }

            // It should be moved to some kind of protocol abstraction
            // in the future, which would be responsible for converting content
            // into a command.
            Command command(getID(), contextId, commandStr);
            BlockingQueue<Command> &queue = getInQueue();
            if (!queue.add(command)) {
                // Just ignore the command, maybe next time. This operation
                // cannot block.
                _log.warn("TCP queue for incoming commands is full.");
            } else {
                _readBuffer = _readBuffer.substr( pos + strlen( separator ) );
            }

            // Command handled, so skip different separators.
            break;
        }

        separators++;

    }

    // Handles write buffer.
    Command pendingCommand;
    BlockingQueue<Command> &queue = getOutQueue();

    // Take into account that the command is removed from the queue only
    // if it can be properly handled; otherwise it is left where it is.
    while (queue.peek(pendingCommand)) {

        // Do not send unknown context ID.
        string commandRaw;
        int contextId = pendingCommand.getContextId();
        if (contextId != -1) {
            stringstream ss;
            ss << contextId << '/';
            ss << pendingCommand.getValue();
            commandRaw = ss.str();
        } else {
            commandRaw = pendingCommand.getValue();
        }

        if ((commandRaw.size() + _writeBuffer.size() +
                JSR_TCP_DEFAULT_SEPARATOR_SIZE ) < _cfg.getTcpBufferSize()) {

            _writeBuffer.append( commandRaw );
            _writeBuffer.append( JSR_TCP_DEFAULT_SEPARATOR );

            queue.popOnly();

        } else {

            // Check if there is even a chance to send it later.
            if (commandRaw.size() > _cfg.getTcpBufferSize()) {
                // Command is bigger than TCP buffer, so it cannot be sent,
                // just ignore it.
                _log.error("Command bigger than TCP buffer has been ignored.");
                queue.popOnly();
            }

            // Break, maybe in the next step. For now, buffer is full.
            _log.warn("TCP write buffer for outgoing commands is full.");

            break;
        }
    }

    return _writeBuffer.size() > 0;

}

// Reads data from the socket until there is anything.
// Internal API, not need to be synchronized. Used only inside thread-safe methods.
int TCPClient::recv() {

    char buffer[JSR_TCP_LOCAL_BUFFER];

    while( true ) {
        int available = _cfg.getTcpBufferSize() - _readBuffer.size();
        int min = available > JSR_TCP_LOCAL_BUFFER ? JSR_TCP_LOCAL_BUFFER : available;
        if( min == 0 ) {
            // Sorry the buffer is full, client have to interpret it first.
            _log.warn("TCPClient::recv: TCP read buffer for incoming commands is full.");
            break;
        }
        int rc = ::recv( _socket, buffer, min, 0 );
        if( rc < 0 ) {
            if( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) || ( errno == EINTR ) ) {
                // Do nothing, maybe next time something will be read.
                return JSR_ERROR_NO_ERROR;
            } else {
                // RECV failed.
                _log.error("TCPClient::recv: recv failed with error %d.", errno);
                return JSR_ERROR_RECV_FAILED;
            }
        } else if( rc == 0 ) {
            // Connection closed.
            return JSR_ERROR_CONNECTION_CLOSED;
        } else {
            // Sanity check. Communication protocol doesn't allow zeroes.
            size_t i;
            for( i = 0; i < rc && buffer[i] != '\0'; i++ ) { }
            if( i != rc ) {
                // Malicious data, disconnect the client.
                return JSR_ERROR_CONNECTION_CLOSED;
            }
            rc = fillReadBuffer( buffer, 0, rc );
            if( rc ) {
                return rc;
            }
        }
    }

    return JSR_ERROR_NO_ERROR;
}

// Internal API, not need to be synchronized. Used only inside thread-safe methods.
int TCPClient::fillReadBuffer( char *buffer, int offset, size_t size ) {

    if( size <= 0 ) {
        // Nothing to read from.
        return JSR_ERROR_NO_ERROR;
    }

    // Just in case.
    if( _readBuffer.size() + size > _cfg.getTcpBufferSize() ) {
        // Max buffer exceeded.
        _log.warn("TCPClient::fillReadBuffer: Maximal size of the read buffer reached.");
        return JSR_ERROR_OUT_OF_MEMORY;
    }

    // Append to the buffer, and check if there is any command which can be treated as completed one.
    // Commands are separated by new line characters.
    _readBuffer.append( string( buffer, offset, size ) );

    handleBuffers();

    return JSR_ERROR_NO_ERROR;
}

// Sends data into the socket.
// Internal API, not need to be synchronized. Used only inside thread-safe methods.
int TCPClient::send() {

    int rc;
    while( !_writeBuffer.empty() || handleBuffers() ) {
        rc = ::send( _socket, _writeBuffer.c_str(), _writeBuffer.size(), 0 );
        if( rc < 0 ) {
            if( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) || ( errno == EINTR ) ) {
                // Do nothing, maybe next time something will be written.
                return JSR_ERROR_WOULD_BLOCK;
            } else {
                _log.error("TCPClient::send: send failed with error %d.", errno);
                // RECV failed.
                return JSR_ERROR_RECV_FAILED;
            }
        } else if( rc > 0 ) {
            // Remove anything that has been already sent.
            _writeBuffer = _writeBuffer.substr( rc );
        }
    }

    return JSR_ERROR_NO_ERROR;
}

TCPProtocol::TCPProtocol( ClientManager &clientManager, QueueSignalHandler<Command> &commandHandler, const JSRemoteDebuggerCfg &cfg) :
        _log(LoggerFactory::getLogger()),
        _clientManager(clientManager),
        _cfg(cfg),
        _serverSocket(0),
        // This pointer does not escape here,
        // because this thread is not started
        // Immediately.
        _thread(*this),
        _inCommandHandler(commandHandler) {
    _pipefd[0] = 0;
    _pipefd[1] = 0;
}

TCPProtocol::~TCPProtocol() {
    if( _serverSocket ) {
        close( _serverSocket );
        _serverSocket = 0;
    }
    if( _pipefd[0] ) {
        close(_pipefd[0]);
    }
    if( _pipefd[1] ) {
        close(_pipefd[1]);
    }
}

void TCPProtocol::run() {

    bool running = true;

    try {

       int rc;
       fd_set fds;
       fd_set read_fds;
       fd_set write_fds;
       int fdmax;

       FD_ZERO(&fds);
       FD_ZERO(&write_fds);

       int pipe = _pipefd[0];

       FD_SET(_serverSocket, &fds );
       FD_SET( pipe, &fds );
       fdmax = ( pipe > _serverSocket ) ? pipe : _serverSocket;

       _clientManager.start();

       while( running ) {

           // Prepare read_fds again, because it might has been cleared by the "select".
           read_fds = fds;

           rc = ::select( fdmax + 1, &read_fds, &write_fds, nullptr, nullptr );

           if ( rc == -1 ) {
               if ( errno == EINTR ) {
                   continue;
               } else {
                   _log.error( "TCPProtocol::run: select failed with error: %d - %s.", errno, strerror( errno ) );
                   break;
               }
           }

           for( int i = 0; i <= fdmax; i++) {

               // Write data.
               if( FD_ISSET( i, &write_fds ) ) {
                   // Notice that client is protected from removing while the whole block is executed.
                   ClientPtrHolder<TCPClient> client(_clientManager, i);
                   if( client ) {
                       if( ( rc = client->send() ) ) {
                           if( rc != JSR_ERROR_WOULD_BLOCK ) {
                               // Something failed, so disconnect the socket, notice
                               // that "would block" is silently ignored and FD flag
                               // is not cleared.
                               disposeClient( &client, fds, write_fds, fdmax );
                           }
                       } else {
                           // Clear this flag, but only if everything have been sent.
                           clearFD( i, CBS_WRITE, fds, write_fds, fdmax );
                       }
                   } else {
                       // Something wrong, there is no such a client, so clear write flag anyway.
                       closeSocket( i, fds, write_fds, fdmax );
                   }
               }

               // Read data.
               if( FD_ISSET( i, &read_fds ) ) {

                   // Accept new connection.
                   if( i == _serverSocket ) {

                       // Store the client under its socket.
                       acceptClient( fds, fdmax );

                   // Handle PIPE related communication.
                   } else if ( i == pipe ) {

                       // Reads command and arguments from pipe.
                       uint8_t command;
                       uint32_t socket;
                       if( recvCommand( pipe, command, socket ) ) {
                            if( command == JSR_TCP_PIPE_COMMAND_DISCONNECT ) {
                                // Disconnect the socket.
                                commandDisconnectClient( socket, fds, write_fds, fdmax );
                            } else if( command == JSR_TCP_PIPE_COMMAND_WRITE ) {
                                // Mark socket as "I have something to write..."
                                commandMarkWrite( socket, write_fds, fdmax );
                            } else if( command == JSR_TCP_PIPE_COMMAND_EXIT ) {
                                // Close the debugger.
                                running = false;
                                break;
                            }
                       } else {
                           _log.error("TCPProtocol::run: Reading from PIPE failed: %d.", errno);
                       }

                   } else {
                       // Client received data.
                       ClientPtrHolder<TCPClient> client(_clientManager, i);
                       if( client ) {
                           bool disconnect = false;
                           if( ( rc = client->recv() ) ) {
                               if( ( rc != JSR_ERROR_NO_ERROR ) ) {
                                   // No matter what, just disconnect the client.
                                   disconnect = true;
                               }
                           }
                           if( disconnect ) {
                               disposeClient( &client,  fds, write_fds, fdmax );
                           }
                       } else {
                           // Unknown client, just disconnect it. It should never happen.
                           closeSocket( i, fds, write_fds, fdmax );
                       }
                   }
               }
           }

           _clientManager.periodicCleanup();
       }

   } catch( std::bad_alloc &ex ) {
       _log.error("TCPProtocol::run: Out of memory, server failed: %s.", ex.what());
   } catch( std::exception &ex ) {
       _log.error("TCPProtocol::run: Unexpected exception, server failed: %s.", ex.what());
   }

   // Closes all connected clients, but doesn't touch the server itself.
   int error = _clientManager.stop();
   if( error == JSR_ERROR_CANNOT_REMOVE_CONNECTIONS ) {
       // Wait a bit for all clients to be released.
       int i = 3;
       while( i-- >= 0 ) {
           _clientManager.periodicCleanup();
           ::usleep(100);
       }
       if( _clientManager.getClientsCount() > 0 ) {
           _log.error("TCPProtocol::run: Cannot close the server in a gently way; some clients are still in use.");
       }
   } else if( error != JSR_ERROR_NO_ERROR ) {
       _log.error("TCPProtocol::run: Cannot close the server in a gently way: %d.", error);
   }


}

/**
 * Marks client's socket as 'I would like to write something' if
 * there is anything in the client's output buffer.
 */
void TCPProtocol::commandMarkWrite( int socket, fd_set &write_fds, int &fdmax ) {
    ClientPtrHolder<TCPClient> client(_clientManager, socket);
    if( client ) {
        if( client->handleBuffers() ) {
            FD_SET (client->getSocket(), &write_fds);
            if( fdmax < client->getSocket() ) {
                fdmax = client->getSocket();
            }
        }
    } else {
        _log.error("Unknown socket read from pipe: %d", socket);
    }
}

/**
 * Reads client's id from the pipe and tries to dispose given client
 * if there is any for given id.
 */
void TCPProtocol::commandDisconnectClient( int socket, fd_set &read_fds, fd_set &write_fds, int &fdmax ) {
    ClientPtrHolder<TCPClient> client(_clientManager, socket);
    if( client ) {
        disposeClient( &client, read_fds, write_fds, fdmax );
    } else {
        _log.error("Unknown socket read from pipe: %d", socket);
    }
}

/**
 * Accepts newly connected client and adds the client to the clients manager.
 */
void TCPProtocol::acceptClient( fd_set &read_fd, int &fdmax ) {

    struct sockaddr_in clientName = {0};
    unsigned int size;

    int clientSocket;
    size = sizeof( clientName );
    clientSocket = ::accept(_serverSocket, (struct sockaddr *) &clientName, &size);
    if ( clientSocket < 0 ) {
        _log.error( " TCPProtocol::acceptClient: accept failed with error: %d.", errno );
        return;
    }

    // Make the socket a nonblocking one.
    int flags;
    if ( ( ( flags = ::fcntl( clientSocket, F_GETFL, 0 ) ) < 0 ) ||
            ( ::fcntl( clientSocket, F_SETFL, flags | O_NONBLOCK ) < 0 ) ) {
        _log.error( " TCPProtocol::acceptClient: fcntl failed with error: %d.", errno );
        return;
    }

    int rc;

    TCPClient *tcpClient = new TCPClient( _cfg, clientSocket, _pipefd[1] );
    tcpClient->getInQueue().setSignalHandler( &_inCommandHandler );

    if( ( rc = _clientManager.addClient( tcpClient ) ) ) {
        delete tcpClient;
        ::close(clientSocket);
    } else {
        FD_SET (clientSocket, &read_fd);
        if( fdmax < clientSocket ) {
            fdmax = clientSocket;
        }
    }

}

void TCPProtocol::disposeClient( TCPClient *client, fd_set &read_fds, fd_set &write_fds, int &fdmax ) {

    // Disconnect the connection. This operation is very safe because
    // client do not expose this socket directly. Whole communication
    // is realized using dedicated buffers on the main loop thread.
    int socket = client->getSocket();
    client->closeSocket();
    clearFD( socket, CBS_ALL, read_fds, write_fds, fdmax );

    // Remove the client.
    _clientManager.removeClient(client);
}

bool TCPProtocol::recvCommand(int pipe, uint8_t &command, uint32_t &arg) {
    // If there is something in the pipe, it means that there is probably
    // something to send through the network. We do not need to interpret the
    // data in the pipe in any way. The fact it's there is definitely enough.
    uint8_t buffer[5];
    int rc = ::read( pipe, buffer, sizeof( buffer ) );
    if( rc < sizeof( buffer ) ) {
        // It should never happened in standard circumstances.
        _log.error( "TCPProtocol::recvCommand: Cannot read from signal pipe: %d.", errno );
        // This is a critical error which can make the whole debugger
        // very unstable, so it's better just to close the debugger.
        return false;
    }
    command = buffer[0];
    arg = ::htonl(*((uint32_t*)(buffer+1)));
    return true;
}

void TCPProtocol::interrupt() {
    // Interrupt the main thread.
    TCPProtocol::sendCommand( _pipefd[1], JSR_TCP_PIPE_COMMAND_EXIT, 0 );
}

// Internal API, not need to be synchronized. Used only inside thread-safe methods.
void TCPProtocol::sendCommand( int pipe, uint8_t command, uint32_t args ) {
    uint8_t buffer[5];
    buffer[0] = command;
    uint32_t n_socket = ::htonl( args );
    ::memcpy( buffer + 1, &n_socket, sizeof( uint32_t ) );
    int rc = 0;
    int again = 0;
    do {
        rc = ::write( pipe, buffer, sizeof( buffer ) );
        if( rc == -1 ) {
            if( errno == EAGAIN ) {
                ::usleep(100);
                again++;
            } else {
                // PIPE failed, so nothing to do here.
                break;
            }
        }
    } while( rc == -1 && again <= 3 );
}

/**
 * Closes socket and clears its bit inside all available bit sets.
 */
void TCPProtocol::closeSocket( int socket, fd_set &read_fds, fd_set &write_fds, int &fdmax ) {
    ::close( socket );
    clearFD( socket, CBS_ALL, read_fds, write_fds, fdmax );
}

/**
 * Clears socket's bit inside all provided bit sets.
 */
void TCPProtocol::clearFD( int fd, ClearBitSet setType, fd_set &read_fds, fd_set &write_fds, int &fdmax ) {
    // Clear disconnected client.
    if( setType == CBS_READ || setType == CBS_ALL) {
        FD_CLR( fd, &read_fds );
    }
    if( setType == CBS_WRITE || setType == CBS_ALL) {
        FD_CLR( fd, &write_fds );
    }
    if( fd == fdmax ) {
        fdmax = 0;
        for( int i = 0; i < FD_SETSIZE; i++ ) {
           if( FD_ISSET( i, &read_fds ) || FD_ISSET( i, &write_fds ) ) {
               fdmax = i;
           }
        }
    }
}

int TCPProtocol::init() {

    // Create socket and bind it into the IP and port from configuration.

    int serverSocket;
    struct sockaddr_in serverIp;

    if( ( serverSocket = ::socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) {
        _log.error("TCPProtocol::init: Cannot create server socket: %d", errno);
        return JSR_ERROR_CANNOT_CREATE_SOCKET;
    }

    int enable = 1;
    if( ::setsockopt( serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof( int ) ) == -1 ) {
        _log.error("TCPProtocol::init: setsockopt failed: %d", errno);
        ::close( serverSocket );
        return JSR_ERROR_CANNOT_CHANGE_SOCKET_OPTS;
    }

    struct hostent *hostinfo;
    serverIp.sin_family = AF_INET;
    serverIp.sin_port = htons( _cfg.getTcpPort() ? _cfg.getTcpPort() : JSR_TCP_DEFAULT_PORT );
    if( !_cfg.getTcpHost().empty() ) {
        hostinfo = ::gethostbyname ( _cfg.getTcpHost().c_str() );
        if ( !hostinfo ) {
            _log.error("TCPProtocol::init: gethostbyname failed for %s with %d.", _cfg.getTcpHost().c_str(), errno);
            ::close( serverSocket );
            return JSR_ERROR_CANNOT_RESOLVE_HOST_NAME;
        }
        ::memcpy( &(serverIp.sin_addr.s_addr), hostinfo->h_addr, hostinfo->h_length);
    } else {
        serverIp.sin_addr.s_addr = INADDR_ANY;
    }

    if( ::bind( serverSocket, (struct sockaddr *)&serverIp, sizeof( serverIp ) ) == -1 ) {
        _log.error("TCPProtocol::init: bind failed %d.", errno);
        ::close( serverSocket );
        return JSR_ERROR_CANNOT_BIND_SOCKET;
    }

    // Currently only one client is supported.
    if( ::listen( serverSocket, JSR_TCP_MAX_CLIENTS_SUPPORTED ) == -1 ) {
        _log.error("TCPProtocol::init: listen failed %d.", errno);
        ::close( serverSocket );
        return JSR_ERROR_CANNOT_LISTEN_TO_SOCKET;
    }

    // Prepare communication pipe.
    if( ::pipe2( _pipefd, O_NONBLOCK ) ) {
        _log.error("TCPProtocol::init: pipe2 failed %d.", errno);
        ::close( serverSocket );
        return JSR_ERROR_INTERNAL_PIPE_FAILED;
    }

    _serverSocket = serverSocket;

    return JSR_ERROR_NO_ERROR;
}

int TCPProtocol::startProtocol() {

    // Starts server at a dedicated thread. Bear in mind that
    // the client's status is changed by the thread routine.
    _thread.start();

    return JSR_ERROR_NO_ERROR;
}

int TCPProtocol::stopProtocol() {
    _thread.interrupt();
    _thread.join();
    return JSR_ERROR_NO_ERROR;
}

ClientManager& TCPProtocol::getClientManager() {
    return _clientManager;
}
