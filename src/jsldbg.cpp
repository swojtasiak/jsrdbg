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

#include <jsldbg.h>
#include <log.hpp>
#include "js_dbg_engine.hpp"

using namespace JSR;
using namespace JS;
using namespace Utils;

#define JSR_LOC_DBG_CLIENT_ID   1

class JSLocalDebuggerImpl : protected JSEngineEventHandler {
public:
    JSLocalDebuggerImpl(JSLocalDebugger &localDebugger, JSContext *ctx, JSDbgEngineOptions &options)
        : _engine(*this, ctx, -1, options),
          _ctx(ctx),
          _localDebugger(localDebugger) {
    }
    virtual ~JSLocalDebuggerImpl() {
    }
public:

    // Event from debugger engine.
    int loadScript( JSContext *ctx, std::string file, std::string &script ) {
        return _localDebugger.loadScript( file, script );
    }

    // Event from debugger engine.
    bool sendCommand( int clientId, int contextId, std::string &command ) {
        return _localDebugger.handleCommand( command );
    }

    // Event from debugger engine.
    bool waitForCommand( JSContext *ctx, bool suspended ) {
        return _localDebugger.handlePause( suspended );
    }

    JSDebuggerEngine& getEngine() {
        return _engine;
    }

private:
    JSDebuggerEngine _engine;
    JSContext *_ctx;
    JSLocalDebugger &_localDebugger;

};

JSLocalDebugger::JSLocalDebugger( JSContext *ctx, JSDbgEngineOptions &options ) {
    _impl = new JSLocalDebuggerImpl(*this, ctx, options);
}

JSLocalDebugger::JSLocalDebugger( JSContext *ctx ) {
    JSDbgEngineOptions options;
    _impl = new JSLocalDebuggerImpl(*this, ctx, options);
}

JSLocalDebugger::~JSLocalDebugger() {
    JSLocalDebuggerImpl *impl = static_cast<JSLocalDebuggerImpl*>(_impl);
    delete impl;
}

int JSLocalDebugger::install() {
    JSLocalDebuggerImpl *impl = static_cast<JSLocalDebuggerImpl*>(_impl);
    return impl->getEngine().install();
}

int JSLocalDebugger::uninstall() {
    JSLocalDebuggerImpl *impl = static_cast<JSLocalDebuggerImpl*>(_impl);
    return impl->getEngine().uninstall();
}

bool JSLocalDebugger::sendCommand( const std::string &command, DebuggerStateHint &hint ) {
    JSLocalDebuggerImpl *impl = static_cast<JSLocalDebuggerImpl*>(_impl);
    return impl->getEngine().sendCommand(JSR_LOC_DBG_CLIENT_ID, command, hint);
}

int JSLocalDebugger::loadScript( const std::string &file, std::string &outScript ) {
    JSLocalDebuggerImpl *impl = static_cast<JSLocalDebuggerImpl*>(_impl);
    return impl->loadScript( NULL, file, outScript );
}

JSContext *JSLocalDebugger::getCtx() {
    JSLocalDebuggerImpl *impl = static_cast<JSLocalDebuggerImpl*>(_impl);
    return impl->getEngine().getJSContext();
}

int JSLocalDebugger::removeDebuggee( JS::HandleObject debuggee ) {
    JSLocalDebuggerImpl *impl = static_cast<JSLocalDebuggerImpl*>(_impl);
    return impl->getEngine().unregisterDebuggee( debuggee );
}

int JSLocalDebugger::addDebuggee( JS::HandleObject debuggee ) {
    JSLocalDebuggerImpl *impl = static_cast<JSLocalDebuggerImpl*>(_impl);
    return impl->getEngine().registerDebuggee( debuggee );
}
