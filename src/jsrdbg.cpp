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

/* MozJS */
#include <jsapi.h>
#include <jsdbgapi.h>

#include <iostream>
#include <string.h>
#include <new>
#include <map>

#include <jsrdbg.h>
#include <threads.hpp>
#include <log.hpp>

#include "resources.hpp"
#include "protocol.hpp"
#include "tcp_protocol.hpp"
#include "js_remote_dbg.hpp"
#include "client.hpp"

using namespace Utils;
using namespace JSR;
using namespace std;

class ClientManagerFactory {
public:
    static ClientManager *createClientManager( const JSRemoteDebuggerCfg &cfg ) {
        return new ClientManager();
    }
};

class ProtocolFactory {
public:
    static Protocol *createProtocol( ClientManager &clientManager, SpiderMonkeyDebugger &debugger, const JSRemoteDebuggerCfg &cfg ) {
        // Allows to create dedicated protocol for given configuration.
        switch( cfg.getProtocol() ) {
        case JSRemoteDebuggerCfg::PROTOCOL_TCP_IP:
            return new TCPProtocol( clientManager, debugger, cfg );
        default:
            return NULL;
        }
    }
};

// Default implementation.
class JSRemoteDebuggerImpl : public IJSRemoteDbg {
public:

    JSRemoteDebuggerImpl( const JSRemoteDebuggerCfg &cfg )
        : _protocol( NULL ),
          _cfg( cfg ),
          _log( LoggerFactory::getLogger() ) {

        // Creates client's manager for given configuration.
        _clientManager = ClientManagerFactory::createClientManager( _cfg );

        // Internal implementation of the remote debugger.
        _debugger = new SpiderMonkeyDebugger( *_clientManager, _cfg );

    }

    ~JSRemoteDebuggerImpl() {

        if( _protocol ) {
            _log.error( "~JSRemoteDebuggerImpl: Debugger hasn't been stopped yet!" );
            delete _protocol;
        }

        delete _debugger;

        delete _clientManager;
    }

    int install( JSContext *ctx, const std::string &contextName, const JSDbgEngineOptions &options ) {
        return _debugger->install( ctx, contextName, options );
    }

    int uninstall( JSContext *ctx ) {
        return _debugger->uninstall( ctx );
    }

    int interrupt( JSContext *ctx ) {
        return _debugger->interrupt( ctx );
    }

    int start() {

        MutexLock lock( _mutex );

        if( _protocol ) {
            return JSR_ERROR_DEBUGGER_ALREADY_STARTED;
        }

        int error = JSR_ERROR_NO_ERROR;

        // Creates protocol used to communicate with remote debugger.
        AutoPtr<Protocol> protocol = ProtocolFactory::createProtocol( *_clientManager, *_debugger, _cfg );
        if( !protocol ) {
            _log.error("Protocol not supported. Wrong configuration provided.");
            return JSR_ERROR_UNKNOWN_PROTOCOL;
        }

        // Initializes protocol, for example binds service to the port given in the configuration.
        error = protocol->init();
        if( error ) {
            _log.error("Cannot initialize protocol. Error code: %d", error);
            return error;
        }

        // Makes the protocol usable. For instance starts events loop in case of TCP/IP.
        error = protocol->startProtocol();
        if( error ) {
            _log.error("Cannot start protocol. Error code: %d", error);
            return error;
        }

        _protocol = protocol.release();

        return error;

    }

    int stop() {

        MutexLock lock( _mutex );

        if( !_protocol ) {
            return JSR_ERROR_DEBUGGER_NOT_STARTED;
        }

        int error = JSR_ERROR_NO_ERROR;

        error = _protocol->stopProtocol();
        if( !error ) {
            delete _protocol;
            _protocol = NULL;
        } else {
            _log.error( "Cannot stop debugger %d.", error );
        }

        return error;
    }

    int addDebuggee( JSContext *ctx, JS::HandleObject debuggee ) {
        return _debugger->registerDebuggee( ctx, debuggee );
    }

    int removeDebuggee( JSContext *ctx, JS::HandleObject debuggee ) {
        return _debugger->unregisterDebuggee( ctx, debuggee );
    }

private:
    // Protocol implementation.
    Protocol *_protocol;
    // Internal remote debugger implementation.
    SpiderMonkeyDebugger *_debugger;
    // Manages all connected clients.
    ClientManager *_clientManager;
    // Debugger configuration.
    const JSRemoteDebuggerCfg _cfg;
    // Generic logger.
    Logger &_log;
    // Used to make this component thread safe.
    Mutex _mutex;
};

JSRemoteDebugger::JSRemoteDebugger() {
    JSRemoteDebuggerCfg cfg;
    _impl = new JSRemoteDebuggerImpl( cfg );
}

JSRemoteDebugger::JSRemoteDebugger( const JSRemoteDebuggerCfg& cfg ) {
    _impl = new JSRemoteDebuggerImpl( cfg );
}

JSRemoteDebugger::~JSRemoteDebugger() {
    delete _impl;
}

int JSRemoteDebugger::install( JSContext *ctx, const std::string &contextName, const JSDbgEngineOptions &options ) {
    return _impl->install( ctx, contextName, options );
}

int JSRemoteDebugger::uninstall( JSContext *ctx ) {
    return _impl->uninstall( ctx );
}

int JSRemoteDebugger::interrupt( JSContext *ctx ) {
    return _impl->interrupt( ctx );
}

int JSRemoteDebugger::start() {
    return _impl->start();
}

int JSRemoteDebugger::stop() {
    return _impl->stop();
}

int JSRemoteDebugger::removeDebuggee( JSContext *ctx, JS::HandleObject debuggee ) {
    return _impl->removeDebuggee( ctx, debuggee );
}

int JSRemoteDebugger::addDebuggee( JSContext *ctx, JS::HandleObject debuggee ) {
    return _impl->addDebuggee( ctx, debuggee );
}

/*********************************
 * Debugger configuration class.
 *********************************/

JSRemoteDebuggerCfg::JSRemoteDebuggerCfg( JSRProtocolType protocol, int tcpPort, const std::string &tcpHost, int tcpBufferSize )
    : _tcpPort( tcpPort ),
      _tcpHost( tcpHost ),
      _tcpBufferSize( tcpBufferSize ),
      _protocol(protocol),
      _scriptLoader(NULL) {
}

JSRemoteDebuggerCfg::JSRemoteDebuggerCfg( const JSRemoteDebuggerCfg &cpy ) {
    _tcpPort = cpy._tcpPort;
    _tcpHost = cpy._tcpHost;
    _tcpBufferSize = cpy._tcpBufferSize;
    _protocol = cpy._protocol;
    _scriptLoader = cpy._scriptLoader;
}

JSRemoteDebuggerCfg::~JSRemoteDebuggerCfg() {
}

JSRemoteDebuggerCfg& JSRemoteDebuggerCfg::operator=( const JSRemoteDebuggerCfg &cpy ) {
    if( &cpy != this ) {
        _tcpPort = cpy._tcpPort;
        _tcpHost = cpy._tcpHost;
        _tcpBufferSize = cpy._tcpBufferSize;
        _protocol = cpy._protocol;
        _scriptLoader = cpy._scriptLoader;
    }
    return *this;
}

size_t JSRemoteDebuggerCfg::getTcpBufferSize() const {
    return _tcpBufferSize;
}

void JSRemoteDebuggerCfg::setTcpBufferSize(size_t tcpBufferSize) {
    _tcpBufferSize = tcpBufferSize;
}

const std::string& JSRemoteDebuggerCfg::getTcpHost() const {
    return _tcpHost;
}

void JSRemoteDebuggerCfg::setTcpHost(const char* tcpHost) {
    _tcpHost = tcpHost;
}

int JSRemoteDebuggerCfg::getTcpPort() const {
    return _tcpPort;
}

JSRemoteDebuggerCfg::JSRProtocolType JSRemoteDebuggerCfg::getProtocol() const {
    return _protocol;
}

IJSScriptLoader* JSR::JSRemoteDebuggerCfg::getScriptLoader() const {
    return _scriptLoader;
}

void JSR::JSRemoteDebuggerCfg::setScriptLoader(IJSScriptLoader* scriptLoader) {
    _scriptLoader = scriptLoader;
}

void JSRemoteDebuggerCfg::setProtocol(JSRProtocolType protocol) {
    _protocol = protocol;
}

void JSRemoteDebuggerCfg::setTcpPort(int tcpPort) {
    _tcpPort = tcpPort;
}

IJSRemoteDbg::IJSRemoteDbg() {
}

IJSRemoteDbg::~IJSRemoteDbg() {
}

IJSScriptLoader::IJSScriptLoader() {
}

IJSScriptLoader::~IJSScriptLoader() {
}
