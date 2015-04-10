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

#ifndef SRC_DEBUGGERS_H_
#define SRC_DEBUGGERS_H_

#include <jsdbg_common.h>
#include <jsrdbg.h>

#include <jsapi.h>
#include <jsdbgapi.h>

#include <threads.hpp>
#include <log.hpp>

#include "client.hpp"
#include "js_dbg_engine.hpp"

#include <map>

namespace JSR {

class Debugger;

struct ActionResult {
    enum Result {
        DA_FAILED = 0,
        DA_OK = 1
    };
    Result result;
    DebuggerStateHint hint;
};

class DebuggerAction : private Utils::NonCopyable {
public:
    DebuggerAction();
    DebuggerAction( const DebuggerAction &cpy );
    virtual ~DebuggerAction();
    virtual ActionResult execute( JSContext *ctx, Debugger &debugger ) = 0;
protected:
    Utils::Logger &_log;
};

typedef Utils::BlockingQueue<DebuggerAction*> action_queue;

typedef std::map< JSContext*, action_queue > map_action;
typedef std::map< JSContext*, action_queue >::iterator map_action_iterator;

class Debugger : protected JSEngineEventHandler {
public:
    Debugger();
    virtual ~Debugger();
public:
    virtual int install( JSContext *cx, const std::string &contextName, const JSDbgEngineOptions &options ) = 0;
    virtual int uninstall( JSContext *cx ) = 0;
    virtual int interrupt( JSContext *cx ) = 0;
    virtual int registerDebuggee( JSContext *cx, JS::HandleObject debuggee ) = 0;
    virtual int unregisterDebuggee( JSContext *cx, JS::HandleObject debuggee ) = 0;
    virtual JSDebuggerEngine* getEngine( JSContext *cx ) const;
};

}

#endif /* SRC_DEBUGGERS_H_ */
