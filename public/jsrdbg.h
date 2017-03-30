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

#ifndef PUBLIC_JSRDBG_H_
#define PUBLIC_JSRDBG_H_

#include <jsapi.h>
#include <jsdbgapi.h>

#include "jsdbg_common.h"

#include <string>
#include <stdint.h>

namespace JSR {

/**
 * Base interface for components responsible
 * for providing scripts source code.
 */
class JSRDBG_API IJSScriptLoader {
public:
    IJSScriptLoader();
    virtual ~IJSScriptLoader();
    /**
     * Loads script source from a given path into the provided
     * string instance. Loaded source code should be encoded using
     * UTF-8. Any multibyte encoding should be acceptable, but
     * you will get some characters badly encoded as the function
     * expects UTF-8.
     * @param path File name (Environment encoding).
     * @param script Destination
     */
    virtual int load( JSContext *cx, const std::string &path, std::string &script ) = 0;
};

// Default TCP listening port.

#define JSR_DEFAULT_PROTOCOL JSRemoteDebuggerCfg::PROTOCOL_TCP_IP
#define JSR_DEFAULT_TCP_PORT 8089
#define JSR_DEFAULT_TCP_BINDING_IP ""
#define JSR_DEFAULT_TCP_BUFFER_SIZE (1024 * 1024 * 50)

/**
 * Debugger configuration.
 */
class JSRDBG_API JSRemoteDebuggerCfg {
public:

    /* Type of the protocol used to connect with a debugger. */
    enum JSRProtocolType {
        /* Ordinal TCP/IP v4. */
        PROTOCOL_TCP_IP
    };

    /**
     * Creates configuration instance for given parameters.
     * @param tcpPort TCP port.
     * @param tcpIP TCP IP/Host.
     * @param rcpBufferSize Maximum size of the TCP buffer.
     */
    JSRemoteDebuggerCfg( JSRProtocolType protocol = JSR_DEFAULT_PROTOCOL,
            int tcpPort = JSR_DEFAULT_TCP_PORT,
            const std::string &tcpHost = JSR_DEFAULT_TCP_BINDING_IP,
            int tcpBufferSize = JSR_DEFAULT_TCP_BUFFER_SIZE );
    /**
     * Creates a copy of a configuration.
     * @param cpy Configuration to be copied.
     */
    JSRemoteDebuggerCfg( const JSRemoteDebuggerCfg &cpy );
    /**
     * Deletes configuration instance.
     */
    virtual ~JSRemoteDebuggerCfg();
public:
    /**
     * Copies configuration into another instance.
     * @param cpy Configuration to be copied.
     */
    JSRemoteDebuggerCfg& operator=( const JSRemoteDebuggerCfg &cpy );
    /**
     * Gets maximum size of the TCP buffer. This is a buffer where asynchronous
     * commands are placed just before they are sent over TCP connection.
     * It has to be big enough to be store the biggest command. Commands
     * which exceeds this limit are ignored and appropriate information
     * is sent to the logging daemon.
     * @return Returns TCP buffer size 50MB by default.
     */
    size_t getTcpBufferSize() const;
    /**
     * Sets maximum TCP buffer size. See description of the
     * method above for more information.
     * @param size The new buffer size.
     */
    void setTcpBufferSize(size_t size);
    /**
     * Gets internet address that the socket should be bound to.
     * @return IP/Host.
     */
    const std::string& getTcpHost() const;
    /**
     * Sets internet address that the socket should be bound to.
     * @param tcpHost IP/Host.
     */
    void setTcpHost(const char* tcpHost);
    /**
     * Gets TCP port number the server should listen at.
     * @return TCP port.
     */
    int getTcpPort() const;
    /**
     * Sets TCP port number the server should listen at.
     * @param tcpPort TCP port.
     */
    void setTcpPort(int tcpPort);
    /**
     * Gets used protocol. Currently only TCP is supported.
     * @return Protocol.
     */
    JSRProtocolType getProtocol() const;
    /**
     * Sets communication protocol.
     * @param protocol Communication protocol.
     */
    void setProtocol(JSRProtocolType protocol);
    /**
     * Gets scripts loader implementation.
     * @return Script's loader.
     */
    IJSScriptLoader* getScriptLoader() const;
    /**
     * Sets script's loader implementation.
     * @param scriptLoader Script's loader.
     */
    void setScriptLoader(IJSScriptLoader *scriptLoader);
private:
    // IP address/Host we should listen on.
    std::string _tcpHost;
    // Port number we should listen at.
    int _tcpPort;
    // Size of the TCP buffers.
    size_t _tcpBufferSize;
    // Communication protocol.
    JSRProtocolType _protocol;
    // Component responsible for providing scripts source code.
    IJSScriptLoader *_scriptLoader;
};

/**
 * Base interface for debugger implementation. Destined
 * for internal use only.
 */
class JSRDBG_API IJSRemoteDbg {
public:
    IJSRemoteDbg();
    virtual ~IJSRemoteDbg();
public:
    virtual int install( JSContext *ctx, const std::string &contextName, const JSDbgEngineOptions &options ) = 0;
    virtual int uninstall( JSContext *ctx ) = 0;
    virtual int start() = 0;
    virtual int stop() = 0;
    virtual int interrupt( JSContext *ctx ) = 0;
    virtual int removeDebuggee( JSContext *ctx, JS::HandleObject debuggee ) = 0;
    virtual int addDebuggee( JSContext *ctx, JS::HandleObject debuggee ) = 0;
};

/**
 * Remote JavaScript debugger.
 */
class JSRDBG_API JSRemoteDebugger : public IJSRemoteDbg {
public:
    JSRemoteDebugger();
    JSRemoteDebugger( const JSRemoteDebuggerCfg &cfg );
    virtual ~JSRemoteDebugger();
public:
    /**
     * Registers a debugger instance for given JSContext.
     * Debugger is stopped after installation. In order to start it call 'start'.
     * This method have to be called from the JS engine thread.
     *
     * @return Error code.
     */
    virtual int install( JSContext *ctx, const std::string &contextName, const JSDbgEngineOptions &options );
    /**
     * Uninstall given debugger.
     * This method frees JSR_Dbg structure.
     * This method have to be called from the JS engine thread.
     *
     * @return Error code.
     */
    virtual int uninstall( JSContext *ctx );
    /**
     * Can be used to interrupt a paused debugger. Even if the debugger is not paused
     * in the time of calling this method it is left in 'interrupted' state. Therefore
     * use this method with care, only if you have to break paused debugger in order
     * to shutdown it definitely. Call it with NULL to interrupt all the debuggers.
     *
     * @param ctx JSContext handling the debuggee.
     * @return Error code.
     */
    virtual int interrupt( JSContext *ctx );
    /**
     * Starts a debugger instance.
     * This method have to be called from the JS engine thread.
     * Starts a new thread in the background if protocol is PROTOCOL_TCP_IP.
     * @return Error code.
     */
    virtual int start();
    /**
     * Stops a debugger instance.
     * This method have to be called from the JS engine thread.
     *
     * @return Error code.
     */
    virtual int stop();
    /**
     * Removes global object from a debugger.
     * This method have to be called from the JS engine thread.
     *
     * @param debuggee The debuggee instance.
     * @return Error code.
     */
    virtual int removeDebuggee( JSContext *ctx, JS::HandleObject debuggee );
    /**
     * Adds new debuggee object to a debugger. Every global new
     * object should be make visible for the debugger using this method.
     * New global objects are not registered automatically.
     * This method have to be called from the JS engine thread.
     *
     * @param debuggee The debuggee instance.
     * @return Error code.
     */
    virtual int addDebuggee( JSContext *ctx, JS::HandleObject debuggee );
private:
    // Internal API.
    IJSRemoteDbg *_impl;
};

}

#endif /* PUBLIC_JSRDBG_H_ */
