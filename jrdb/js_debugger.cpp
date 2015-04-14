/*
 * A Remote Debugger Client for SpiderMonkey Java Script Engine.
 * Copyright (C) 2014-2015 Slawomir Wojtasiak
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "js_debugger.hpp"

#include <string>

#include <encoding.hpp>
#include <js_utils.hpp>

#include "resources.hpp"
#include "errors.hpp"

using namespace std;
using namespace JS;
using namespace Utils;

/* The class of the global object. */
static JSClass JS_DBG_Global = { "global", JSCLASS_GLOBAL_FLAGS, JS_PropertyStub,
        JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
        JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub };

/* Prints arguments to the console. */
static JSBool JDB_fn_print_core( JSContext *cx, unsigned int argc, Value *vp, bool endline ) {

    JSDebugger *dbg = static_cast<JSDebugger*>( JS_GetContextPrivate( cx ) );
    if( !dbg ) {
        JS_ReportError( cx, "Debugger object not found." );
        return JS_FALSE;
    }

    CallArgs args = CallArgsFromVp(argc, vp);

    string string;

    MozJSUtils jsUtils(cx);

    if( !jsUtils.argsToString( argc, JS_ARGV(context, vp), string ) ) {
        JS_ReportError( cx, "Cannot convert arguments to a string." );
        return JS_FALSE;
    }

    if( endline ) {
        string.append("\n");
    }

    DebuggerCtx &ctx = dbg->getContext();
    ctx.print( string );

    return JS_TRUE;
}

static JSBool JDB_fn_print( JSContext *cx, unsigned int argc, Value *vp ) {
    return JDB_fn_print_core( cx, argc, vp, false );
}

static JSBool JDB_fn_println( JSContext *cx, unsigned int argc, Value *vp ) {
    return JDB_fn_print_core( cx, argc, vp, true );
}

/* Prints arguments to the console. */
static JSBool JDB_fn_sendCommand( JSContext *cx, unsigned int argc, Value *vp ) {

    JSDebugger *dbg = static_cast<JSDebugger*>( JS_GetContextPrivate( cx ) );
    if( !dbg ) {
        JS_ReportError( cx, "Debugger object not found." );
        return JS_FALSE;
    }

    CallArgs args = CallArgsFromVp(argc, vp);

    MozJSUtils jsUtils(cx);

    string commandStr;

    Value jsCommand = args.get(1);
    if( jsCommand.isString() ) {

        // Converts JS string into UTF8 native multibyte.
        if( !jsUtils.toUTF8( jsCommand, commandStr ) ) {
            JS_ReportError( cx, "Cannot convert UTF-16LE to UTF-8." );
            return JS_FALSE;
        }

    } else {

        if( !jsUtils.stringifyToUtf8( jsCommand, commandStr ) ) {
            if( jsUtils.getLastError() == MozJSUtils::ERROR_JS_STRINGIFY_FAILED ) {
                if( !JS_IsExceptionPending( cx ) ) {
                    // Only if there is no exception from the stringify.
                    JS_ReportError( cx, "Cannot stringify debugger command." );
                }
                return JS_FALSE;
            } else if( jsUtils.getLastError() == MozJSUtils::ERROR_CHAR_ENCODING_FAILED ) {
                // Only if there is no exception from the stringify.
                JS_ReportError( cx, "Cannot convert UTF-16LE to UTF-8." );
                return JS_FALSE;
            }
        }

    }

    DebuggerCommand command( args.get(0).toInt32(), commandStr );

    DebuggerCtx &ctx = dbg->getContext();
    ctx.sendCommand(command);

    return JS_TRUE;
}

static JSFunctionSpec JDB_EnvironmentFuncs[] = {
   { "print", JSOP_WRAPPER ( JDB_fn_print ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
   { "println", JSOP_WRAPPER ( JDB_fn_println ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
   { "sendCommand", JSOP_WRAPPER ( JDB_fn_sendCommand ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
   { NULL },
};

DebuggerCtx &JSDebugger::getContext() {
   return _ctx;
}

JSDebugger::JSDebugger( DebuggerCtx &ctx )
    : DebuggerEngine(ctx),
      _rt(NULL),
      _cx(NULL),
      _global(NULL),
      _log(LoggerFactory::getLogger()) {
}

JSDebugger::~JSDebugger() {
    destroy();
}

int JSDebugger::init() {

    // Creates new inner compartment in the runtime.
    _rt = JS_NewRuntime(8L * 1024 * 1024, JS_NO_HELPER_THREADS);
    if (!_rt) {
        return JDB_ERROR_JS_ENGINE_FAILED;
    }

    JS_SetNativeStackQuota(_rt, 1024 * 1024);
    JS_SetGCParameter(_rt, JSGC_MAX_BYTES, 0xffffffff);

    _cx = JS_NewContext(_rt, 8192);
    if (!_cx) {
        return JDB_ERROR_JS_CANNOT_CREATE_CONTEXT;
    }

    JS_SetContextPrivate( _cx, this );

    // New global object gets it's own compartments too.
    CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);
    JS::RootedObject global(_cx, JS_NewGlobalObject(_cx, &JS_DBG_Global, nullptr, options));
    if (!global) {
        return JDB_ERROR_JS_CANNOT_CREATE_GLOBAL;
    }

    JSAutoRequest req(_cx);
    JSAutoCompartment ac(_cx, global);

    /* Set the context's global */
    JS_InitStandardClasses(_cx, global);

    StringResource script = Resources::getStringResource(Resources::MOZJS_DEBUGGER_CLIENT);
    if( script.isEmpty() ) {
        return JDB_ERROR_JS_CODE_NOT_FOUND;
    }

    RootedObject env( _cx, JS_NewObject( _cx, NULL, NULL, NULL ) );
    if( !env ) {
        return JDB_ERROR_JS_CANNOT_CREATE_OBJECT;
    }

    MozJSUtils jsUtils(_cx);

    if( !jsUtils.setPropertyObj(global, "env", env ) ) {
        return JDB_ERROR_JS_CANNOT_SET_PROPERTY;
    }

    if( !jsUtils.setPropertyInt32(env, "engineMajorVersion", static_cast<uint32_t>(MOZJS_MAJOR_VERSION) ) ) {
        return JDB_ERROR_JS_CANNOT_SET_PROPERTY;
    }

    if( !jsUtils.setPropertyInt32(env, "engineMinorVersion", static_cast<uint32_t>(MOZJS_MINOR_VERSION) ) ) {
        return JDB_ERROR_JS_CANNOT_SET_PROPERTY;
    }

    if( !jsUtils.setPropertyStr(env, "packageVersion", PACKAGE_VERSION ) ) {
        return JDB_ERROR_JS_CANNOT_SET_PROPERTY;
    }

    if ( !JS_DefineFunctions( _cx, env, &JDB_EnvironmentFuncs[0] ) ) {
        return JDB_ERROR_JS_CANNOT_DEFINE_FUNCTION;
    }

    JS::Value retval = JSVAL_VOID;
    if( !jsUtils.evaluateUtf8Script( global, script.getValue(), "mozjs_dbg_client.js", &retval ) ) {
        return JDB_ERROR_JS_DEBUGGER_SCRIPT_FAILED;
    }

    _debugger.set(&retval.toObject());

    _global = global.get();

    return JDB_ERROR_NO_ERROR;
}

int JSDebugger::destroy() {

    if( _cx ) {
        JS_DestroyContext(_cx);
        _cx = NULL;
    }

    if( _rt ) {
        JS_DestroyRuntime(_rt);
        _rt = NULL;
    }

    JS_ShutDown();

    return JDB_ERROR_NO_ERROR;
}

int JSDebugger::sendCtrlCommand( const string &command ) {

    JSAutoCompartment compartment(_cx,_global);
    JSAutoRequest request(_cx);

    bool unicode = true;

    // Try to convert the source string to UTF-16LE,
    // just in order not to loss any multibyte characters.

    RootedString jsSourceCode(_cx);
    MozJSUtils jsUtils(_cx);
    if( !jsUtils.fromUTF8(command, &jsSourceCode) ) {
        _log.error("Cannot convert string to UTF-16LE: %d", jsUtils.getLastError());
        return false;
    }

    Value argv[] = { STRING_TO_JSVAL( jsSourceCode ) };
    Value result;
    if( !JS_CallFunctionName( _cx, _debugger, "handleCtrlCommand", 1, argv, &result ) ) {
        _log.error("JS function 'handleCtrlCommand' failed: %s", jsUtils.getPendingExceptionMessage().c_str());
        return JDB_ERROR_JS_FUNCTION_FAILED;
    }

    return JDB_ERROR_NO_ERROR;
}

int JSDebugger::sendCommand( const DebuggerCommand &dbgCommand ) {

    JSAutoCompartment compartment(_cx,_global);
    JSAutoRequest request(_cx);

    const string &command = dbgCommand.getContent();

    RootedObject jsCommand(_cx);
    MozJSUtils jsUtils(_cx);
    if( !jsUtils.parseUtf8JSON(command, &jsCommand) ) {
        return JDB_ERROR_JS_JSON_PARSING_FAILED;
    }

    Value argv[] = {
        INT_TO_JSVAL( dbgCommand.getContextId() ),
        OBJECT_TO_JSVAL( jsCommand )
    };

    Value result;
    if( !JS_CallFunctionName( _cx, _debugger, "handleDbgCommand", 2, argv, &result ) ) {
        _log.error("JS function 'handleDbgCommand' failed: %s", jsUtils.getPendingExceptionMessage().c_str());
        return JDB_ERROR_JS_FUNCTION_FAILED;
    }

    return JDB_ERROR_NO_ERROR;
}

DebuggerCtx &JSDebugger::getDebuggerCtx() {
    return _ctx;
}
