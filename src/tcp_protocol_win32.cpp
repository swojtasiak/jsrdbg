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

#include "tcp_protocol_win32.hpp"

#include <Ws2tcpip.h> // GetAddrInfoW

#include <timestamp.hpp>
#include <utils.hpp>
#include <js_utils.hpp>

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <utility>
#include <array>

using namespace JSR;
using namespace Utils;

#define JSR_TCP_MAX_CLIENTS_SUPPORTED       1
#define JSR_TCP_DEFAULT_PORT                8089
#define JSR_TCP_LOCAL_BUFFER                1024
#define JSR_TCP_DEFAULT_SEPARATOR           "\n"
#define JSR_TCP_DEFAULT_SEPARATOR_SIZE      sizeof( JSR_TCP_DEFAULT_SEPARATOR )

namespace {

// Must be called once and must have a corresponding winSockCleanup call.
bool winSockStartup() {
    auto& log = LoggerFactory::getLogger();
    WORD versionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    int res = WSAStartup( versionRequested, &wsaData );
    if ( res != 0 ) {
        log.error("WSAStartup failed with error code %d", res);
        return false;
    }

    // Confirm that the WinSock DLL supports 2.2
    if ( LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2 ) {
        // Tell the user that we couldn't find a usable WinSock DLL
        log.error("Could not find a usable version of Winsock.dll");
        WSACleanup();
        return false;
    }

    return true;
}

// Must be called iff winSockStartup() returned true.
void winSockCleanup() {
    WSACleanup();
}

}

namespace JSR {

    /**
     * All commands separators supported by the protocol.
     */
    const char* COMMANDS_SEPARATORS[] = {
        "\r\n",
        "\n",
        NULL
    };

}

/************
 * TCPClient
 ************/

TCPClientWin32::TCPClientWin32( const JSRemoteDebuggerCfg &cfg, SOCKET clientSocket ) :
        Client(clientSocket),
        _log( LoggerFactory::getLogger() ),
        _socket(clientSocket),
        _closed(false),
        _cfg(cfg) {
    // We are interested in new commands from debugger in order
    // to inform the server that there is something to send.
    getOutQueue().setSignalHandler(this);
}

TCPClientWin32::~TCPClientWin32() {
    // Just to be sure that socket is closed.
    closeSocket();
}

// This method should be called by main protocol thread.
void TCPClientWin32::closeSocket() {
    // Closes client's socket.
    if( _socket != INVALID_SOCKET ) {
        int res = closesocket( _socket );
        if ( res != 0 ) {
            int code = WSAGetLastError();
            _log.error("TCPClientWin32::closeSocket: failed to close socket; error code %s",
                systemErrorString(code).c_str());
        }
        _socket = INVALID_SOCKET;
    }
}

void TCPClientWin32::disconnect() {

}

bool TCPClientWin32::isConnected() {
    MutexLock lock(_mutex);
    return _socket != INVALID_SOCKET;
}

void TCPClientWin32::handle( BlockingQueue<Command> &queue, int signal ) {
    // We need to send something to the client on the other side.
    // Send information about something to write through
    // the pipe which is scanned by main TCP thread.

}

// Internal API, not need to be synchronized. Used only inside thread-safe methods.
void TCPClientWin32::sendCommand( uint8_t command, uint32_t args ) {
}

SOCKET TCPClientWin32::getSocket() const {
    return _socket;
}

// Internal API, not need to be synchronized. Used only inside
// thread-safe methods.
bool TCPClientWin32::handleBuffers() {

    // Handles read buffer.
    size_t pos;

    const char **separators = COMMANDS_SEPARATORS;

    while (*separators) {

        const char *separator = *separators;

        if ((pos = _readBuffer.find(separator)) != std::string::npos) {

            // Adds received command into the queue of the commands
            // sent by the connected client.
            std::string commandStr = _readBuffer.substr(0, pos);

            // Check if there is context ID in the command string.
            int contextId = -1;
            if (!Utils::MozJSUtils::splitCommand(commandStr, contextId,
                    commandStr)) {
                _log.error( "TCPClientWin32::handleBuffers: Broken context ID: %s",
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
        std::string commandRaw;
        int contextId = pendingCommand.getContextId();
        if (contextId != -1) {
            std::stringstream ss;
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
int TCPClientWin32::recv() {

    char buffer[JSR_TCP_LOCAL_BUFFER];

    while( true ) {
        int available = _cfg.getTcpBufferSize() - _readBuffer.size();
        int min = available > JSR_TCP_LOCAL_BUFFER ? JSR_TCP_LOCAL_BUFFER : available;
        if( min == 0 ) {
            // Sorry the buffer is full, client have to interpret it first.
            _log.warn("TCPClientWin32::recv: TCP read buffer for incoming commands is full.");
            break;
        }
        int bytes_received = ::recv( _socket, buffer, min, 0 );
        if( bytes_received == SOCKET_ERROR ) {
            auto err = WSAGetLastError();
            if( ( err == WSAEWOULDBLOCK ) || ( err == WSAEINTR ) ) {
                // Do nothing, maybe next time something will be read.
                return JSR_ERROR_NO_ERROR;
            }
            else {
                // RECV failed.
                _log.error("TCPClientWin32::recv: recv failed: %s", systemErrorString(err).c_str());
                return JSR_ERROR_RECV_FAILED;
            }

        }
        else if( bytes_received == 0 ) {
            // Connection closed.
            return JSR_ERROR_CONNECTION_CLOSED;

        }
        else {
            // Sanity check. Communication protocol doesn't allow zeroes.
            size_t i;
            for( i = 0; i < bytes_received && buffer[i] != '\0'; i++ ) { }
            if( i != bytes_received ) {
                // Malicious data, disconnect the client.
                return JSR_ERROR_CONNECTION_CLOSED;
            }
            bytes_received = fillReadBuffer( buffer, 0, bytes_received );
            if( bytes_received ) {
                return bytes_received;
            }
        }
    }

    return JSR_ERROR_NO_ERROR;
}

// Internal API, not need to be synchronized. Used only inside thread-safe methods.
int TCPClientWin32::fillReadBuffer( char *buffer, int offset, size_t size ) {

    if( size <= 0 ) {
        // Nothing to read from.
        return JSR_ERROR_NO_ERROR;
    }

    // Just in case.
    if( _readBuffer.size() + size > _cfg.getTcpBufferSize() ) {
        // Max buffer exceeded.
        _log.warn("TCPClientWin32::fillReadBuffer: Maximal size of the read buffer reached.");
        return JSR_ERROR_OUT_OF_MEMORY;
    }

    // Append to the buffer, and check if there is any command which can be treated as completed one.
    // Commands are separated by new line characters.
    _readBuffer.append( std::string( buffer, offset, size ) );

    handleBuffers();

    return JSR_ERROR_NO_ERROR;
}

// Sends data into the socket.
// Internal API, not need to be synchronized. Used only inside thread-safe methods.
int TCPClientWin32::send() {

    while( !_writeBuffer.empty() || handleBuffers() ) {
        int bytes_send = ::send( _socket, _writeBuffer.c_str(), _writeBuffer.size(), 0 );
        if( bytes_send == SOCKET_ERROR ) {
            auto err = WSAGetLastError();
            if( ( err == WSAEWOULDBLOCK ) || ( err == WSAEINTR ) ) {
                // Do nothing, maybe next time something will be written.
                return JSR_ERROR_WOULD_BLOCK;
            }
            else {
                _log.error("TCPClientWin32::send: send failed with error: %s",
                    systemErrorString(err).c_str());
                return JSR_ERROR_RECV_FAILED;
            }
        }
        else if( bytes_send > 0 ) {
            // Remove anything that has been already sent.
            _writeBuffer = _writeBuffer.substr( bytes_send );
        }
    }

    return JSR_ERROR_NO_ERROR;
}

TCPProtocolWin32::TCPProtocolWin32( ClientManager &clientManager, QueueSignalHandler<Command> &commandHandler, const JSRemoteDebuggerCfg &cfg) :
        _clientManager(clientManager),
        _log(LoggerFactory::getLogger()),
        _cfg(cfg),
        _serverSocket(INVALID_SOCKET),
        _thread(*this),
        _inCommandHandler(commandHandler),
        _stopEvent(nullptr),
        _networkEvent(nullptr) {

    winSockStartup();
}

TCPProtocolWin32::~TCPProtocolWin32() {
    if( _serverSocket != INVALID_SOCKET ) {
        closesocket( _serverSocket );
        _serverSocket = INVALID_SOCKET;
    }
    if( _stopEvent ) {
        CloseHandle( _stopEvent );
    }
    if( _networkEvent ) {
        WSACloseEvent( _networkEvent );
    }

    winSockCleanup();
}

void TCPProtocolWin32::run() {
    _clientManager.start();
    OnScopeExit stop_clients([&] { _clientManager.stop(); });

    try {

        std::vector<WSAEVENT> events;
        events.push_back( _networkEvent );
        events.push_back( _stopEvent );

        // Loop indefinitely, waiting for remote connections or a stop signal

        bool running = true;
        while( running ) {

            DWORD ready = WSAWaitForMultipleEvents( events.size(), &events[0], false, INFINITE, false );

            if ( ready == WSA_WAIT_FAILED ) {
                _log.error("TCPProtocolWin32::run: wait failed with error: %d", ready);
                running = false;
                break;
            }

            if ( ready == WSA_WAIT_EVENT_0 ) {

                // Some event on network. WSAEnumNetworkEvents resets the
                // event object atomically for us, no need to reset manually

                WSANETWORKEVENTS networkEvents;
                if ( WSAEnumNetworkEvents( _serverSocket, _networkEvent, &networkEvents ) == SOCKET_ERROR ) {
                    _log.error("TCPProtocolWin32::run: enumerating network events failed: %s", 
                        systemErrorString(WSAGetLastError()).c_str());
                    running = false;
                    break;
                }

                long flags = networkEvents.lNetworkEvents;
                if (flags & FD_ACCEPT) {

                    // Accept new connection.
                    acceptClient();

                }
                else {
                    _clientManager.forEach([&](Client* client) {
                        TCPClientWin32* win32Client = dynamic_cast<TCPClientWin32*>(client);
                        assert(win32Client);
                        SOCKET clientSocket = win32Client->getSocket();
                        WSANETWORKEVENTS clientEvents;
                        if (WSAEnumNetworkEvents(clientSocket, _networkEvent, &clientEvents) == SOCKET_ERROR) {
                            _log.error("TCPProtocolWin32::run: enumerating network events failed: %s", 
                                systemErrorString(WSAGetLastError()).c_str());
                            running = false;
                            return;
                        }
                        long clientFlags = clientEvents.lNetworkEvents;
                        if (clientFlags & FD_READ) {
                        }
                        else if (clientFlags & FD_WRITE) {
                        }
                        else if (clientFlags & FD_CLOSE) {
                        }
                    });
                }
            }
            else if ( ready == WSA_WAIT_EVENT_0 + 1 ) {

                // Stop event.
                running = false;
                break;
            }

            _clientManager.periodicCleanup();
        }

    } catch( std::bad_alloc &ex ) {
        _log.error("TCPProtocolWin32::run: Out of memory, server failed: %s.", ex.what());
    } catch( std::exception &ex ) {
        _log.error("TCPProtocolWin32::run: Unexpected exception, server failed: %s.", ex.what());
    }

    // Closes all connected clients, but doesn't touch the server itself.
    int error = _clientManager.stop();
    if( error == JSR_ERROR_CANNOT_REMOVE_CONNECTIONS ) {
        // Wait a bit for all clients to be released.
        int i = 3;
        while( i-- >= 0 ) {
            _clientManager.periodicCleanup();
            Sleep(1);
        }
        if( _clientManager.getClientsCount() > 0 ) {
            _log.error("TCPProtocolWin32::run: Cannot close the server in a gently way; some clients are still in use.");
        }
    } else if( error != JSR_ERROR_NO_ERROR ) {
        _log.error("TCPProtocolWin32::run: Cannot close the server in a gently way: %d.", error);
    }
}

/**
 * Accepts newly connected client and adds the client to the clients manager.
 */
void TCPProtocolWin32::acceptClient() {

    SOCKADDR_IN clientName;
    memset( &clientName, 0, sizeof( clientName ) );
    int size = sizeof( clientName );

    SOCKET clientSocket = accept( _serverSocket, reinterpret_cast<SOCKADDR*>(&clientName), &size );

    if (clientSocket == INVALID_SOCKET) {
        _log.error("TCPProtocolWin32::acceptClient: accept failed with error: %s", systemErrorString(WSAGetLastError()).c_str());
        return;
    }

    // Connection accepted

    auto tcpClient = std::unique_ptr<TCPClientWin32>(new TCPClientWin32( _cfg, clientSocket ));
    tcpClient->getInQueue().setSignalHandler( &_inCommandHandler );

    int retcode = _clientManager.addClient( tcpClient.get() );
    if( retcode == JSR_ERROR_NO_ERROR) {
        tcpClient.release();
    }
    else {
        closesocket(clientSocket);
    }
}


void TCPProtocolWin32::interrupt() {
    SetEvent( _stopEvent );
}

// Internal API, not need to be synchronized. Used only inside thread-safe methods.
void TCPProtocolWin32::sendCommand( HANDLE writePipe, uint8_t command, uint32_t args ) {
    uint8_t buffer[5];
    buffer[0] = command;
    uint32_t n_socket = htonl( args );
    memcpy( buffer + 1, &n_socket, sizeof( uint32_t ) );

    DWORD bytesWritten = 0;
    bool ok = false;
    int again = 0;
    do {
        ok = WriteFile( writePipe, buffer, sizeof( buffer ),  &bytesWritten, nullptr);
        if( !ok ) {
            int err = GetLastError();
            if( err == ERROR_IO_PENDING ) {
                Sleep(1);
                again++;
            }
            else {
                // PIPE failed, so nothing to do here.
                break;
            }
        }
    } while( !ok && again <= 3 );
}

int TCPProtocolWin32::init() {

    // Create a custom event so that we can signal/interrupt the server thread

    _stopEvent = CreateEvent( nullptr, true, false, nullptr );
    if ( !_stopEvent ) {
        _log.error("Could not create stop event, error: %d", GetLastError());
        return JSR_ERROR_INTERNAL_PIPE_FAILED;

    }

    // Create socket and bind it to IP address and port from configuration.

    SOCKET serverSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if( serverSocket == INVALID_SOCKET ) {
        _log.error("TCPProtocolWin32::init: Could not create server socket: %s",
            systemErrorString(WSAGetLastError()).c_str());
        return JSR_ERROR_CANNOT_CREATE_SOCKET;
    }
    OnScopeExit closeSocket([&] { closesocket(serverSocket); });

    // Set the SO_REUSEADDR socket option, which explicitly allows a process
    // to bind to a port which remains in TIME_WAIT (it still only allows a
    // single process to be bound to that port). This is the both the simplest
    // and the most effective option for reducing the "address already in use"
    // error.

    DWORD enable = TRUE;
    int res = setsockopt( serverSocket, SOL_SOCKET, SO_REUSEADDR,
        reinterpret_cast<char*>(&enable), sizeof( decltype(enable) ) );
    if( res != 0 ) {
        _log.error("TCPProtocolWin32::init: setsockopt failed: %s",
            systemErrorString(WSAGetLastError()).c_str());
        return JSR_ERROR_CANNOT_CHANGE_SOCKET_OPTS;
    }

    SOCKADDR_IN serverIp;
    serverIp.sin_family = AF_INET;
    serverIp.sin_port = htons( _cfg.getTcpPort() ? _cfg.getTcpPort() : JSR_TCP_DEFAULT_PORT );
    if ( _cfg.getTcpHost().empty() ) {
        serverIp.sin_addr.s_addr = INADDR_ANY;
    }
    else {

        ADDRINFOA hints;
        memset( &hints, 0, sizeof( hints ) );
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        ADDRINFOA* result;

        int rescode = getaddrinfo( _cfg.getTcpHost().c_str(), nullptr, &hints, &result );
        OnScopeExit freeAddrInfo([&] { FreeAddrInfoA(result); });
        if (rescode != 0) {
            _log.error("TCPProtocolWin32::init: Can't resolve hostname '%s': %s",
                _cfg.getTcpHost().c_str(), gai_strerror( rescode ));
            return JSR_ERROR_CANNOT_RESOLVE_HOST_NAME;
        }

        serverIp.sin_addr = reinterpret_cast<SOCKADDR_IN*>(result->ai_addr)->sin_addr;
    }

    if ( bind(serverSocket, reinterpret_cast<SOCKADDR*>(&serverIp), sizeof( SOCKADDR ) ) == SOCKET_ERROR ) {
        _log.error("TCPProtocolWin32::init: bind failed: %s",
            systemErrorString(WSAGetLastError()).c_str());
        return JSR_ERROR_CANNOT_BIND_SOCKET;
    }

    _networkEvent = WSACreateEvent();
    if (WSAEventSelect(serverSocket, _networkEvent, FD_ACCEPT | FD_CLOSE | FD_READ ) == SOCKET_ERROR) {
        _log.error("TCPProtocolWin32::init: WSAEventSelect failed on server socket: %s",
            systemErrorString(WSAGetLastError()).c_str());
        return JSR_ERROR_INTERNAL_PIPE_FAILED;
    }

    // TODO: How many clients can we support? JSR_TCP_MAX_CLIENTS_SUPPORTED | SOMAXCONN

    if ( listen( serverSocket, SOMAXCONN ) == SOCKET_ERROR ) {
        _log.error("TCPProtocolWin32::init: listen failed");
        return JSR_ERROR_CANNOT_LISTEN_TO_SOCKET;
    }

    _serverSocket = serverSocket;
    closeSocket.release();
    return JSR_ERROR_NO_ERROR;
}

int TCPProtocolWin32::startProtocol() {

    // Starts server at a dedicated thread. Bear in mind that
    // the client's status is changed by the thread routine.
    _thread.start();

    return JSR_ERROR_NO_ERROR;
}

int TCPProtocolWin32::stopProtocol() {
    _thread.interrupt();
    _thread.join();
    return JSR_ERROR_NO_ERROR;
}

ClientManager& TCPProtocolWin32::getClientManager() {
    return _clientManager;
}
