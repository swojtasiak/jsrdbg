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

#ifndef SRC_JS_DBG_UTILS_HPP_
#define SRC_JS_DBG_UTILS_HPP_

#include <jsdbg_common.h>
#include <jsapi.h>
#include <jsdbgapi.h>
#include <map>

#include <threads.hpp>
#include <log.hpp>

namespace JSR {

// Interface for handling events emitted by debugger engine.
class JSEngineEventHandler {
public:
    JSEngineEventHandler();
    virtual ~JSEngineEventHandler();
public:
    /**
     * Loads script source code from external source.
     * @param file Path.
     * @param script Output parameter for source code.
     * @return Error code.
     */
    virtual int loadScript( JSContext *ctx, std::string file, std::string &script ) = 0;
    /**
     * Receives commands sent by engine itself.
     * @param clientId Client id.
     * @param command JSON formated command.
     * @return True if command has been sent successfully.
     */
    virtual bool sendCommand( int clientId, int contextId, std::string &command ) = 0;
    /**
     * Called when debugger is going to pause. This method is
     * blocked one. Debugger is blocked until this method returns.
     * @param suspended True if this is first call to the method due to the
     *                  fact that debugger is starting is suspended mode.
     * @return True if debugger should continue, and false if debugger
     * has been interrupted and is going to shutdown.
     */
    virtual bool waitForCommand( JSContext *ctx, bool suspended ) = 0;
};

// Generic JS engine debugger implementation.
class JSDebuggerEngine {
public:
    JSDebuggerEngine( JSEngineEventHandler &handler, JSContext *ctx, int contextId, const JSDbgEngineOptions &options );
    ~JSDebuggerEngine();
public:
    /**
     *  Install debugger for the context engine was created for.
     *  @return Unified error code.
     */
    int install();
    /**
     *  Uninstall debugger from the context this object was created for.
     *  @return Unified error code.
     */
    int uninstall();
    /**
     * Registers a new debuggee inside the debugger.
     * @param debuggee Debuggee to be installed.
     * @return Unified error code.
     */
    int registerDebuggee( const JS::HandleObject debuggee );
    /**
     * Unregisters a debuggee from the debugger.
     * @param debuggee Debuggee to be uninstalled.
     * @return Unified error code.
     */
    int unregisterDebuggee( const JS::HandleObject debuggee );
    /**
     * Gets true if debugger is already installed.
     * @return True if debugger is already installed.
     */
    bool isInstalled() const;
    /**
     * Sends command to the debugger.
     * @param clientId Client id.
     * @param command JSON encoded command.
     * @param[out] engineState The result of the command.
     * @return True if command has been handled correctly.
     */
    bool sendCommand( int clientId, const std::string &command, DebuggerStateHint &engineState );
    /**
     * Gets a handler responsible for handling events emitted
     * by JS engine.
     */
    JSEngineEventHandler &getEngineEventHandler();
    /**
     * Gets the engine module instance. It's internal API so you
     * have to use it with care!
     * @return The main engine module.
     */
    JSObject *getEngineModule() const;
    /**
     * Debugger's environment object.
     * @return Debugger's environment object.
     */
    JS::HandleObject getEnv() const;
    /**
     * Debugger's global object.
     * @return Debugger's global data.
     */
    JSObject *getDebuggerGlobal() const;
    /**
     * Gets debugger's JS context.
     * @return JSContext.
     */
    JSContext *getJSContext() const;
    /**
     * Gets user's private data. You can use it whatever you want,
     * this value is not used by the engine internally.
     * @return User's private data.
     */
    void *getTag() const;
    /**
     * Sets user's private data.
     * @param tag User's data.
     */
    void setTag( void *tag );
    /**
     * Gets engines options.
     * @return Options.
     */
    const JSDbgEngineOptions &getEngineOptions() const;
    /**
     * Gets numeric ID of the used context.
     * @return Context id.
     */
    int getContextId() const;
public:
    /**
     * This method is used i order to map context to its counterpart
     * engine instance. This method is here just to avoid JSContext private
     * data usage, because it can be a bit more invasive.
     * @param ctx JSContext.
     * @return The context's engine.
     */
    static JSDebuggerEngine *getEngineForContext( JSContext *ctx );
    /**
     * Sets engine instance for given JS context.
     * @param ctx JS context.
     * @param Debugger engine.
     */
    static void setEngineForContext( JSContext *ctx, JSDebuggerEngine *engine );
private:
    // Global map used to map context to corresponding engine.
    // It's used this way in order to avoid filling context private data.
    static std::map<JSContext*,JSDebuggerEngine*> _ctxToEngine;
    // The map above has to be synchronized because it has to
    // support more than one JS engine at a time.
    static Utils::Mutex _mutex;
    // Handler responsible for handling events from JS engine.
    JSEngineEventHandler &_eventHandler;
    // Spider monkey context.
    JSContext *_ctx;
    // Numeric context ID.
    int _contextId;
    // Module of the installed debugger.
    JSObject *_debuggerModule;
    // Debugger's global object.
    JSObject *_debuggerGlobal;
    // Environment object.
    JS::Heap<JSObject*> _env;
    // Logger.
    Utils::Logger &_log;
    // Private data that can be set by anyone. It's not used by
    // the engine internally.
    void *_tag;
    // Engine configuration options.
    JSDbgEngineOptions _options;
};

}

#endif /* SRC_JS_DBG_UTILS_HPP_ */
