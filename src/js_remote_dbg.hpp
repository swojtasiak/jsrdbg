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

#ifndef SRC_JS_REMOTE_DBG_H_
#define SRC_JS_REMOTE_DBG_H_

#include <jsdbg_common.h>
#include <jsrdbg.h>

#include "debuggers.hpp"

#include <map>
#include <vector>

#include <threads.hpp>
#include <log.hpp>

#include "client.hpp"
#include "message_builder.hpp"
#include "js_dbg_engine.hpp"

namespace JSR {

// Command sent to the debugger engine by one of the clients.
class CommandAction : public DebuggerAction {
public:
    CommandAction( ClientManager &clientManager, Command &command );
    virtual ~CommandAction();
    virtual ActionResult execute( JSContext *ctx, Debugger &debugger );
private:
    ClientManager &_clientManager;
    Command _command;
};

// Breaks paused debugger.
class ContinueAction : public DebuggerAction {
public:
    ContinueAction();
    virtual ~ContinueAction();
    virtual ActionResult execute( JSContext *ctx, Debugger &debugger );
};

struct JSContextDescriptor {
    // Name of the JS context.
    std::string contextName;
    // Generated id of the context.
    int contextId;
    // JS Context.
    JSContext *context;
};

typedef std::map< int, JSContextDescriptor > map_context;
typedef std::vector< JSContextDescriptor > vector_context;
typedef std::vector< JSContextDescriptor >::iterator list_context_iterator;
typedef std::pair< int, JSContextDescriptor > map_context_pair;
typedef std::map< int, JSContextDescriptor >::iterator map_context_iterator;

/**
 * This class is not proven to be thread safe, so
 * use it from the same thread that runs the destination
 * JS engine.
 */
class SpiderMonkeyDebugger : public Debugger,
    public Utils::QueueSignalHandler<Command>,
    protected Utils::EventHandler {
public:
    SpiderMonkeyDebugger( ClientManager &manager, const JSRemoteDebuggerCfg &cfg );
    virtual ~SpiderMonkeyDebugger();
    int install( JSContext *cx, const std::string &contextName, const JSDbgEngineOptions &options );
    int uninstall( JSContext *cx );
    int interrupt( JSContext *cx );
    int handlePendingCommands( JSContext *cx );
    int registerDebuggee( JSContext *cx, JS::HandleObject debuggee );
    int unregisterDebuggee( JSContext *cx, JS::HandleObject debuggee );
    const JSRemoteDebuggerCfg &getDebuggerConf() const;
    ClientManager &getClientManager() const;
    void setContextPaused( JSContext *cx, bool paused );
    bool isContextPaused( JSContext *cx );
public:
    // JS Engine events handlers.
    int loadScript( JSContext *cx, std::string file, std::string &script );
    bool sendCommand( int clientId, int contextId, std::string &command );
    bool waitForCommand( JSContext *cx, bool suspended );
public:
    void handle( command_queue &queue, int signal );
protected:
    void handle( Utils::Event &event );
private:
    void sendErrorMessage( int clientId, JSR::MessageFactory::ErrorCode errorCode, const std::string &msg );
    void sendContextsList( int clientId, int contextId, const std::string &requestId );
    void sendCommandToQueue( JSContext *ctx, action_queue &queue, Command &command );
    bool isSystemCommand( const std::string &command, const std::string &commandName );
    std::string extractRequestId( const std::string &command );
private:
    // ID of the next registered engine.
    static int _contextCounter;
    // Maps unique context ID to JSContext.
    map_context _contextMap;
    // Global logger.
    Utils::Logger &_log;
    // Debugger configuration.
    JSRemoteDebuggerCfg _cfg;
    // Manages all registered clients.
    ClientManager &_clientManager;
    // Debugger can be accessed by more than one runtime, thereby
    // the more than one thread can be used to access this component.
    Utils::Mutex _lock;
};

}

#endif /* SRC_JS_REMOTE_DBG_H_ */
