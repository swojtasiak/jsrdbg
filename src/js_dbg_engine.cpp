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

#include "js_dbg_engine.hpp"

#include <iostream>
#include <jsdbg_common.h>
#include <js_utils.hpp>

#include "js/js_resources.hpp"

using namespace JSR;
using namespace Utils;
using namespace JS;
using namespace std;

#define JSENG_WFC_RES_INTERRUPTED   1
#define JSENG_WFC_RES_CONTINUE      2

namespace MozJS {

   // Definition of the global object for the debugger compartment.
   static JSClass JSR_DebuggerEngineGlobalGlass = { "JSRDebuggerGlobal",
       JSCLASS_GLOBAL_FLAGS,
       JS_PropertyStub,
       JS_DeletePropertyStub,
       JS_PropertyStub,
       JS_StrictPropertyStub,
       JS_EnumerateStub,
       JS_ResolveStub,
       JS_ConvertStub,
       NULL,
       NULL,
       NULL,
       NULL,
       NULL,
       NULL,
       { NULL }
   };

   /**
    * Gets source code of the script in a safe way. The default implementation
    * returns critical error which makes the script to end execution if there
    * is no source available. This function returns null instead.
    */
    static JSBool JSR_fn_getSourceSafe( JSContext *context, unsigned int argc, Value *vp ) {

        if( argc != 1 ) {
            JS_ReportError( context, "JSR_fn_getSourceSafe:: Bad args." );
            return JS_FALSE;
        }

        CallArgs args = CallArgsFromVp(argc, vp);

        Value jsTextProperty;
        if( JS_GetProperty( context, args.get(0).toObjectOrNull(), "text", &jsTextProperty ) ) {
            args.rval().set(jsTextProperty);
        } else {
            args.rval().setNull();
        }

        return JS_TRUE;
    }

   /**
    * Prints function arguments into the console.
    */
   static JSBool JSR_fn_print( JSContext *context, unsigned int argc, Value *vp ) {

       if( argc == 0 ) {
           JS_ReportError( context, "JSR_fn_print:: Bad args." );
           return JS_FALSE;
       }

       CallArgs args = CallArgsFromVp(argc, vp);

       string val;

       MozJSUtils jsUtils(context);
       if(!jsUtils.argsToString(argc, JS_ARGV(context, vp), val)) {
           JS_ReportError( context, "JSDebuggerEngine:: Cannot convert arguments to C string." );
           return JS_FALSE;
       }

       // Environment's encoding is used here.
       cout << val << endl;

       args.rval().setNull();

       return JS_TRUE;
   }

   /**
    * Loads script by given script name.
    */
   static JSBool JSR_fn_loadScript( JSContext *cx, unsigned argc, Value *vp ) {

       Logger &log = LoggerFactory::getLogger();

       if( argc != 1 ) {
           JS_ReportError( cx, "Only one argument is allowed." );
           return JS_FALSE;
       }

       // Gets engine for context.
       JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext(cx);
       if( !engine ) {
           log.error( "JSR_fn_loadScript:: There is no engine installed for given context." );
           JS_ReportError( cx, "There is no engine installed for given context." );
           return JS_FALSE;
       }

       MozJSUtils jsUtils(cx);

       CallArgs args = CallArgsFromVp(argc, vp);

       string filePath;
       RootedString filePathArg( cx, args.get(0).toString() );
       if( !jsUtils.toString(filePathArg, filePath) ) {
           log.error("JSR_fn_loadScript:: Cannot convert file name to C string.");
           JS_ReportError( cx, "Cannot convert file name to C string." );
           return JS_FALSE;
       }

       JSEngineEventHandler &handler = engine->getEngineEventHandler();

       string script;

       // Pass processing to a custom handler.
       int rc;
       if( ( rc = handler.loadScript( cx, filePath, script ) ) ) {
           log.error( "JSR_fn_loadScript:: Cannot read string using provided callback: %d", rc );
           string msg;
           if( rc == JSR_ERROR_CANNOT_READ_FILE ) {
               msg = "Cannot read string using provided callback. Source file not found: " + filePath;
           } else {
               msg = "Cannot read string using provided callback.";
           }
           JS_ReportError( cx, msg.c_str() );
           return JS_FALSE;
       }

       try {

           if( !script.empty() ) {

               JCharEncoder encoder;
               jstring jscript = encoder.utf8ToWide(script);

               RootedString jsScript(cx);
               if( !jsUtils.fromString( jscript, &jsScript ) ) {
                   log.error("JSR_fn_loadScript:: Cannot read string using provided callback.");
                   JS_ReportError( cx, "Cannot read string using provided callback." );
               }

               args.rval().setString( jsScript );

           } else {
               args.rval().setNull();
           }

       } catch( EncodingFailedException &exc ) {
           log.error("JSR_fn_loadScript:: Encoding failed, cannot encode script to UTF-16.");
           JS_ReportError( cx, "Encoding failed, cannot encode script to UTF-16." );
           return JS_FALSE;
       }

       return JS_TRUE;
   }

   /**
    * Waits for incoming command.
    */
   static JSBool JSR_fn_waitForCommand( JSContext *cx, unsigned argc, Value *vp ) {

       if( argc != 1 ) {
          JS_ReportError( cx, "Function should be called with one argument." );
          return JS_FALSE;
       }

       // Gets engine for context.
       JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext(cx);
       if( !engine ) {
           JS_ReportError( cx, "There is no engine installed for given context." );
           return JS_FALSE;
       }

       // Inform the debugger what to do next.
       CallArgs args = CallArgsFromVp(argc, vp);

       // Enter the command loop.
       bool result = engine->getEngineEventHandler().waitForCommand( cx, args.get(0).toBoolean() );

       args.rval().setInt32( result ? JSENG_WFC_RES_CONTINUE : JSENG_WFC_RES_INTERRUPTED );

       return JS_TRUE;
   }

   /**
    * Sends command to the remote client.
    */
   static JSBool JSR_fn_sendCommand( JSContext *cx, unsigned argc, Value *vp ) {

       if( argc != 2 ) {
           JS_ReportError( cx, "Function should be called with exactly two arguments." );
           return JS_FALSE;
       }

       Logger &log = LoggerFactory::getLogger();

       // Gets engine for context.
       JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext(cx);
       if( !engine ) {
           log.error("JSR_fn_sendCommand: There is no engine installed for given context." );
           JS_ReportError( cx, "There is no engine installed for given context." );
           return JS_FALSE;
       }

       CallArgs args = CallArgsFromVp(argc, vp);

       int clientId = args.get(0).toInt32();

       // Convert object into UTF-16 string using standard stringify logic.
       Value jsCommandStr = args.get(1);

       MozJSUtils jsUtils(cx);

       string commandStr;
       if( !jsUtils.stringifyToUtf8( jsCommandStr, commandStr ) ) {
           if( jsUtils.getLastError() == MozJSUtils::ERROR_JS_STRINGIFY_FAILED ) {
               log.error("JSR_fn_sendCommand: Cannot stringify debugger command.");
               if( !JS_IsExceptionPending( cx ) ) {
                   // Only if there is no exception from the stringify.
                   JS_ReportError( cx, "Cannot stringify debugger command." );
               }
               return JS_FALSE;
           } else if( jsUtils.getLastError() == MozJSUtils::ERROR_CHAR_ENCODING_FAILED ) {
               // Only if there is no exception from the stringify.
               log.error("JSR_fn_sendCommand: Cannot convert command string into UTF-8." );
               JS_ReportError( cx, "Cannot convert command string into UTF-8." );
               return JS_FALSE;
           } else {
               // Unknown error.
               log.error("JSR_fn_sendCommand: Stringify failed with error: %d", jsUtils.getLastError() );
               JS_ReportError( cx, "Stringify failed with unsupported error code." );
               return JS_FALSE;
           }
       }

       // Send command using provided handler.
       if( !engine->getEngineEventHandler().sendCommand( clientId, engine->getContextId(), commandStr ) ) {
           log.warn( "JSR_fn_sendCommand: Engine couldn't send a command for client: %d", clientId );
           if( !JS_IsExceptionPending( cx ) ) {
               JS_ReportError( cx, "Cannot send command, probably client has already been disconnected." );
           }
           return JS_FALSE;
       }

       return JS_TRUE;
   }

   static JSFunctionSpec JSR_EngineEnvironmentFuntions[] = {
       { "getSourceSafe", JSOP_WRAPPER ( JSR_fn_getSourceSafe ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
       { "print", JSOP_WRAPPER ( JSR_fn_print ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
       { "loadScriptSource", JSOP_WRAPPER (JSR_fn_loadScript), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
       { "waitForCommand", JSOP_WRAPPER (JSR_fn_waitForCommand), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
       { "sendCommand", JSOP_WRAPPER (JSR_fn_sendCommand), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
       JS_FS_END
   };

}

/*******************
 * JSDebuggerEngine
 *******************/

std::map<JSContext*,JSDebuggerEngine*> JSDebuggerEngine::_ctxToEngine;

Utils::Mutex JSDebuggerEngine::_mutex;

JSDebuggerEngine::JSDebuggerEngine( JSEngineEventHandler &handler, JSContext *ctx, int contextId, const JSDbgEngineOptions &options )
    : _eventHandler(handler),
      _ctx(ctx),
      _contextId(contextId),
      _debuggerModule(NULL),
      _debuggerGlobal(NULL),
      _log(LoggerFactory::getLogger()),
      _tag(NULL),
      _options(options) {
}

JSDebuggerEngine::~JSDebuggerEngine() {
    if( _debuggerGlobal ) {
        _log.error( "JSDebuggerEngine:: Debugger hasn't been uninstalled correctly." );
    }
}

int JSDebuggerEngine::install() {

    if( _debuggerModule ) {
        return JSR_ERROR_SM_DEBUGGER_ALREADY_INSTALLED;
    }

    // Gets global object which is to be a debuggee in our context.
    JSAutoRequest req(_ctx);

    // Create a new compartment for the debugger instance.
    CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);
    RootedObject debuggerGlobal( _ctx, JS_NewGlobalObject( _ctx, &MozJS::JSR_DebuggerEngineGlobalGlass, NULL, options ) );
    if( !debuggerGlobal ) {
       _log.error( "JSDebuggerEngine::install: Cannot create new JS global object (JS_NewGlobalObject failed)." );
       return JSR_ERROR_SM_CANNOT_CREATE_GLOBAL_OBJECT;
    }

    // Enter into the new compartment.
    JSAutoCompartment cr(_ctx, debuggerGlobal);

    // Initialize global object.
    if (!JS_InitStandardClasses(_ctx, debuggerGlobal)) {
        _log.error( "JSDebuggerEngine::install: JS_InitStandardClasses failed." );
        return JSR_ERROR_SM_FAILED_TO_INITIALIZE_STD_CLASSES;
    }

    if (!JS_InitReflect(_ctx, debuggerGlobal)) {
        _log.error( "JSDebuggerEngine::install: JS_InitReflect failed." );
        return JSR_ERROR_SM_FAILED_TO_INITIALIZE_REFLECT;
    }

    if (!JS_DefineDebuggerObject(_ctx, debuggerGlobal)) {
        _log.error( "JSDebuggerEngine::install: JS_DefineDebuggerObject failed." );
        return JSR_ERROR_SM_FAILED_TO_INITIALIZE_DEBUGGER;
    }

    // Creates a new object for the "environment" property.
    RootedObject env( _ctx, JS_NewObject( _ctx, NULL, NULL, NULL ) );
    if( !env ) {
        _log.error( "JSDebuggerEngine::install: Cannot create new JS object (JS_NewObject failed)." );
        return JSR_ERROR_SM_CANNOT_CREATE_OBJECT;
    }

    Value jsvalEnv = OBJECT_TO_JSVAL( env );
    if( !JS_SetProperty( _ctx, debuggerGlobal, "env", &jsvalEnv ) ) {
        _log.error( "JSDebuggerEngine::install: Cannot set object property (JS_SetProperty failed)." );
        return JSR_ERROR_SM_CANNOT_SET_PROPERTY;
    }

    MozJSUtils jsUtils(_ctx);

#ifdef JSRDBG_DEBUG
    if( !jsUtils.setPropertyInt( env, "debug", 1 ) ) {
        _log.error( "JSDebuggerEngine::install: Cannot set object property (JS_SetProperty failed)." );
        return JSR_ERROR_SM_CANNOT_SET_PROPERTY;
    }
#endif

    if( !jsUtils.registerModuleLoader( debuggerGlobal ) ) {
        _log.error( "JSDebuggerEngine::install: Cannot install module loader." );
        return JSR_ERROR_SM_CANNOT_REGISTER_MODULE_LOADER;
    }

    if ( !JS_DefineFunctions( _ctx, env, &MozJS::JSR_EngineEnvironmentFuntions[0] ) ) {
        _log.error( "JSDebuggerEngine::install: Cannot define JS functions (JS_DefineFunctions failed)." );
        return JSR_ERROR_SM_CANNOT_DEFINE_FUNCTION;
    }

    // Load debugger bootstrap script.
    ResourceManager &manager = GetResourceManager();

    if( !jsUtils.addResourceManager( debuggerGlobal, "dbg", manager ) ) {
        _log.error( "JSDebuggerEngine::install: Cannot add ResourceManager." );
        return JSR_ERROR_SM_CANNOT_REGISTER_MODULE_LOADER;
    }

    Resource const *resource = manager.getResource("mozjs_dbg");
    if( !resource ) {
        _log.error( "JSDebuggerEngine::install: Cannot get the main module: mozjs_dbg." );
        return JSR_ERROR_SM_CANNOT_DEFINE_FUNCTION;
    }

    const string script = resource->toString();

    // Prepares 'engine.options' object.
    RootedObject envOptions( _ctx, JS_NewObject( _ctx, NULL, NULL, NULL ) );
    if( envOptions ) {
        bool rs = false;
        rs |= !jsUtils.setPropertyBool( envOptions, "suspended", _options.isSuspended() );
        rs |= !jsUtils.setPropertyInt( envOptions, "sourceDisplacement", _options.getSourceCodeDisplacement() );
        rs |= !jsUtils.setPropertyObj( env, "options", envOptions );
        if( rs ) {
            _log.error( "JSDebuggerEngine::install: Cannot create options for JS engine." );
            return JSR_ERROR_SM_CANNOT_CREATE_OBJECT;
        }
    } else {
        _log.error( "JSDebuggerEngine::install: Cannot create new JS object (JS_NewObject failed)." );
        return JSR_ERROR_SM_CANNOT_CREATE_OBJECT;
    }

    Value retval;
    if( !jsUtils.evaluateUtf8Script( debuggerGlobal, script,  "mozjs_dbg.js", &retval ) ) {
        _log.error( "JSDebuggerEngine::install: Cannot evaluate hosted debugging code." );
        return JSR_ERROR_SM_CANNOT_EVALUATE_SCRIPT;
    }

    _debuggerGlobal = debuggerGlobal;
    if( !JS_AddObjectRoot( _ctx, &_debuggerGlobal ) ) {
        _log.error( "JSDebuggerEngine::install: Cannot root debugger global." );
        return JSR_ERROR_SM_CANNOT_EVALUATE_SCRIPT;
    }

    _debuggerModule = &retval.toObject();
    if( !JS_AddObjectRoot( _ctx, &_debuggerModule ) ) {
        JS_RemoveObjectRoot( _ctx, &_debuggerModule );
        _log.error( "JSDebuggerEngine::install: Cannot root debugger module." );
        return JSR_ERROR_SM_CANNOT_EVALUATE_SCRIPT;
    }

    _env = env;

    // Map context to the engine instance.
    setEngineForContext(_ctx, this);

    return JSR_ERROR_NO_ERROR;
}

int JSDebuggerEngine::uninstall() {

    if( !_debuggerModule ) {
        return JSR_ERROR_SM_DEBUGGER_IS_NOT_INSTALLED;
    }

    // Enter into the debugger compartment.
    JSAutoRequest req(_ctx);
    JSAutoCompartment cr(_ctx, _debuggerGlobal);

    // Executes "shutdown" methods of debugger module.
    Value result;
    if( !JS_CallFunctionName( _ctx, _debuggerModule, "shutdown", 0, NULL, &result ) ) {
        _log.error( "JSDebuggerEngine::Cannot invoke 'shutdown' function (JS_CallFunctionName failed)." );
        return JSR_ERROR_SM_CANNOT_SHUTDOWN_DEBUGGER;
    }

    if( _debuggerModule ) {
        JS_RemoveObjectRoot( _ctx, &_debuggerModule );
        _debuggerModule = NULL;
    }

    if( _debuggerGlobal ) {
        JS_RemoveObjectRoot( _ctx, &_debuggerGlobal );
        _debuggerGlobal = NULL;
    }

    _env = NULL;

    setEngineForContext(_ctx, NULL);

    return JSR_ERROR_NO_ERROR;

}

bool JSDebuggerEngine::sendCommand( int clientId, const std::string &command, DebuggerStateHint &engineState ) {

    bool result = true;

    try {

        // Enter into the debugger compartment.
        JSAutoRequest req(_ctx);
        JSAutoCompartment cr(_ctx, _debuggerGlobal);

        JCharEncoder encoder;
        jstring jcommand = encoder.utf8ToWide( command );

        // We are not interested in exceptions inside
        // the command handler, they are silently ignored
        // just because we do not have any logging facility yet.
        JSExceptionState *excState = JS_SaveExceptionState(_ctx);

        RootedValue parsedCommand(_ctx);
        if( !JS_ParseJSON( _ctx, jcommand.c_str(), jcommand.size(), &parsedCommand ) ) {
            _log.error( "CommandAction:: Cannot parse debugger command. Syntax error in the JSON structure." );
            result = false;
        }

        if( result ) {

            Value argv[] = {
               JS_NumberValue( clientId ),
               parsedCommand
            };

            Value jsResult;
            if( !JS_CallFunctionName( _ctx, _debuggerModule, "handleCommand", 2, argv, &jsResult ) ) {
               _log.error("CommandAction:: Cannot invoke 'handleCommand' method.");
               result = false;
            } else {
                // Check if command handler should block and wait for incoming commands.
                engineState = static_cast<DebuggerStateHint>( jsResult.toInt32() );
            }

        }

        if( JS_IsExceptionPending( _ctx ) ) {
            MozJSUtils jsUtils(_ctx);
            _log.error( "CommandAction:: Pending exception found: %s : %s", jsUtils.getPendingExceptionMessage().c_str(), jsUtils.getPendingExceptionStack().c_str() );
            result = false;
        }

        JS_RestoreExceptionState(_ctx, excState);

    } catch( EncodingFailedException &exc ) {
        // Probably out of memory or not supported encoding, wait for the next execution.
        _log.error("CommandAction:: Cannot convert incoming command to UTF-16.");
        result = false;
    }

    return result;
}

int JSDebuggerEngine::registerDebuggee( const JS::HandleObject debuggee ) {

    if( !_debuggerModule ) {
        return JSR_ERROR_SM_DEBUGGER_IS_NOT_INSTALLED;
    }

    // Enter into the debugger compartment.
    JSAutoRequest req(_ctx);
    JSAutoCompartment cr(_ctx, _debuggerGlobal);

    // Pass debuggee into the debugger's compartment.
    RootedObject wrappedDebuggee( _ctx, debuggee );
    if( !JS_WrapObject( _ctx, wrappedDebuggee.address() ) ) {
        _log.error( "JSDebuggerEngine::Cannot wrap JS object (JS_WrapObject failed)." );
        return JSR_ERROR_SM_CANNOT_WRAP_OBJECT;
    }

    // After registering the debuggee object it will be rooted by
    // the debugger itself, so it's not needed to root it here.
    RootedValue wrappedDebuggeeValue( _ctx, ObjectValue( *debuggee ) );
    Value argv[] = { wrappedDebuggeeValue };
    Value result;
    if( !JS_CallFunctionName( _ctx, _debuggerModule, "addDebuggee", 1, argv, &result ) ) {
        _log.error( "JSDebuggerEngine::Cannot invoke 'addDebuggee' function (JS_CallFunctionName failed)." );
        return JSR_ERROR_SM_CANNOT_REGISTER_DEBUGGEE;
    }

    return JSR_ERROR_NO_ERROR;

}

int JSDebuggerEngine::unregisterDebuggee( const JS::HandleObject debuggee ) {

    if( !_debuggerModule ) {
       return JSR_ERROR_SM_DEBUGGER_IS_NOT_INSTALLED;
    }

    // Enter into the debugger compartment.
    JSAutoRequest req(_ctx);
    JSAutoCompartment cr(_ctx, _debuggerGlobal);

    // Pass debuggee into the debugger's compartment.
    RootedObject wrappedDebuggee( _ctx, debuggee );
    if( !JS_WrapObject( _ctx, wrappedDebuggee.address() ) ) {
        _log.error( "JSDebuggerEngine::Cannot wrap JS object (JS_WrapObject failed)." );
        return JSR_ERROR_SM_CANNOT_WRAP_OBJECT;
    }

    // After registering the debuggee object it will be rooted by
    // the debugger itself, so it's not needed to root it here.
    RootedValue wrappedDebuggeeValue( _ctx, ObjectValue( *debuggee ) );
    Value argv[] = { wrappedDebuggeeValue };
    Value result;
    if( !JS_CallFunctionName( _ctx, _debuggerModule, "removeDebuggee", 1, argv, &result ) ) {
        _log.error( "JSDebuggerEngine::Cannot invoke 'removeDebuggee' function (JS_CallFunctionName failed)." );
        return JSR_ERROR_SM_CANNOT_REGISTER_DEBUGGEE;
    }

    return JSR_ERROR_NO_ERROR;

}

bool JSDebuggerEngine::isInstalled() const {
    return _debuggerGlobal != NULL;
}

JS::HandleObject JSDebuggerEngine::getEnv() const {
    return _env;
}

JSEngineEventHandler &JSDebuggerEngine::getEngineEventHandler() {
    return _eventHandler;
}

JSObject *JSDebuggerEngine::getEngineModule() const {
    return _debuggerModule;
}

JSObject *JSDebuggerEngine::getDebuggerGlobal() const {
    return _debuggerGlobal;
}

JSContext *JSDebuggerEngine::getJSContext() const {
    return _ctx;
}

void *JSDebuggerEngine::getTag() const {
    return _tag;
}

void JSDebuggerEngine::setTag( void *tag ) {
    _tag = tag;
}

const JSDbgEngineOptions &JSDebuggerEngine::getEngineOptions() const {
    return _options;
}

int JSDebuggerEngine::getContextId() const {
    return _contextId;
}

JSDebuggerEngine *JSDebuggerEngine::getEngineForContext( JSContext *ctx ) {
    MutexLock lock(_mutex);
    JSDebuggerEngine *engine = NULL;
    std::map<JSContext*,JSDebuggerEngine*>::iterator it = _ctxToEngine.find( ctx );
    if( it != _ctxToEngine.end() ) {
        engine = it->second;
    }
    return engine;
}

void JSDebuggerEngine::setEngineForContext( JSContext *ctx, JSDebuggerEngine *engine ) {
    MutexLock lock(_mutex);
    if( engine ) {
        _ctxToEngine.insert( pair<JSContext*,JSDebuggerEngine*>( ctx, engine ) );
    } else {
        _ctxToEngine.erase(ctx);
    }
}

/***********************
 * JSEngineEventHandler
 ***********************/

JSEngineEventHandler::JSEngineEventHandler() {
}

JSEngineEventHandler::~JSEngineEventHandler() {
}
