/*
 * Unit tests for the SpiderMonkey Java Script Engine Debugger.
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

#include <string>
#include <iostream>
#include <exception>
#include <stdexcept>

// SpiderMonkey.
#include <jsapi.h>
#include <jsdbgapi.h>

// JSLDBG.
#include <jsldbg.h>

// Common utilities.
#include <encoding.hpp>
#include <js_utils.hpp>

#include "resources.hpp"

using namespace JSR;
using namespace JS;
using namespace std;
using namespace Utils;

#define INIT_ERROR      1

// Definition of the global object for the debugger compartment.
static JSClass JSR_TestEngineGlobalGlass = { "JSRTestGlobal",
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

// Test script embedded inside executable.
extern char _binary_dbg_check_js_start[];
extern char _binary_dbg_check_js_end[];

/**
 * Prints function arguments into the console.
 */
static JSBool JS_common_fn_print( JSContext *context, unsigned int argc, Value *vp ) {

    CallArgs args = CallArgsFromVp(argc, vp);

    string val;

    MozJSUtils jsUtils(context);
    if(!jsUtils.argsToString(argc, JS_ARGV(context, vp), val)) {
        JS_ReportError( context, "JS_common_fn_print:: Cannot convert arguments to C string." );
        return JS_FALSE;
    }

    // Environment's encoding is used here.
    cout << val << endl;

    args.rval().setNull();

    return JS_TRUE;
}

static JSFunctionSpec JS_TestGlobalFuntions[] = {
    { "print", JSOP_WRAPPER ( JS_common_fn_print ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
    JS_FS_END
};

// Local debugger implementation used to debug test scripts. This class is partially exposed
// to the main script which is responsible for controlling unit tests.
class DebuggeeScript : public JSLocalDebugger {
public:

    DebuggeeScript( HandleObject dbgFacade, string script, JSContext *ctx, JSDbgEngineOptions &options )
        : JSLocalDebugger( create(ctx), options ),
          _script(script),
          _cxCheck(ctx),
          _dbgFacade(dbgFacade) {

        // This is a new context which has been created
        // as a place for test script.
        _cxTest = getCtx();

        try {

            JSAutoRequest req(_cxTest);

            // Global object for test script.
            CompartmentOptions options;
            options.setVersion(JSVERSION_LATEST);
            _globalTest = JS_NewGlobalObject( _cxTest, &JSR_TestEngineGlobalGlass, NULL, options );
            if( !_globalTest ) {
                throw runtime_error( "Cannot create global object for test script." );
            }

            JSAutoCompartment ac(_cxTest, _globalTest);

            // Register global functions for test script.
            if ( !JS_DefineFunctions( _cxTest, _globalTest, &JS_TestGlobalFuntions[0] ) ) {
                throw runtime_error( "Cannot register global functions for test script." );
            }

            // Install debugger.
            if( install() ) {
                throw runtime_error( "Cannot create global object for test script." );
            }

            // Add tests compartment as a debuggee.
            if( addDebuggee( _globalTest ) ) {
                throw runtime_error( "Cannot create global object for test script." );
            }

        } catch( exception &exc ) {
            if( _globalTest ) {
                removeDebuggee( _globalTest );
            }
            uninstall();
            JS_DestroyContext(_cxTest);
            throw runtime_error( exc.what() );
        }

    }

    ~DebuggeeScript() {
        if( _cxTest ) {
            // Before destroying the context make sure that debugger is uninstalled.
            removeDebuggee(_globalTest);
            uninstall();
            JS_DestroyContext(_cxTest);
        }
    }

    // Evaluates test script and returns its result.
    Value start() {

        JSAutoRequest req(_cxTest);
        JSAutoCompartment ac(_cxTest, _globalTest);

        MozJSUtils jsUtils(_cxTest);

        Value retVal;
        if( !jsUtils.evaluateUtf8Script( _globalTest, _script, "test_script.js", &retVal ) ) {
            string msg( "Test script failed: " );
            msg += jsUtils.getPendingExceptionMessage();
            throw runtime_error( msg.c_str() );
        }

        return retVal;
    }

    // Loads script which is not available directly to the script.
    int loadScript( const std::string &file, std::string &outScript ) {
        if( file == "test_script.js" ) {
            outScript = _script;
            return JSR_ERROR_NO_ERROR;
        }
        return JSR_ERROR_FILE_NOT_FOUND;
    }

    // Call onPause method on the debugger facade instance.
    bool handlePause( bool suspended ) {

        JSAutoRequest req(_cxCheck);
        JSAutoCompartment ac( _cxCheck, JS_GetGlobalForObject( _cxCheck, _dbgFacade ) );

        MozJSUtils jsUtils(_cxCheck);

        Value jsFunction;
        if( !JS_GetProperty( _cxCheck, _dbgFacade, "onPause", &jsFunction ) ) {
            throw runtime_error( "Cannot get callback function." );
        }

        bool result = false;

        if( !jsFunction.isUndefined() && jsUtils.isFunctionValue( jsFunction ) ) {

            Value jsRet;

            Value argv[1];
            argv[0].setBoolean( suspended );

            if( JS_CallFunctionValue( _cxCheck, _dbgFacade, jsFunction, 1, argv, &jsRet ) ) {
                result = jsRet.toBoolean();
            } else {
                // Function is probably undefined or failed.
                result = false;
            }
        }

        return result;

    }

    // Call onCommand method on the debugger facade instance.
    bool handleCommand( const std::string &command ) {

        JSAutoRequest req(_cxCheck);
        JSAutoCompartment ac( _cxCheck, JS_GetGlobalForObject( _cxCheck, _dbgFacade ) );

        MozJSUtils jsUtils(_cxCheck);

        RootedObject jsCommand(_cxCheck);
        if( !jsUtils.parseUtf8JSON( command, &jsCommand ) ) {
            cout << "Cannot parse command string." << endl;
            return false;
        }

        Value jsFunction;
        if( !JS_GetProperty( _cxCheck, _dbgFacade, "onCommand", &jsFunction ) ) {
            cout << "Cannot get function from property." << endl;
            return false;
        }

        bool result = false;

        // The 'onCommand' property has to be both defined and a function.
        if( !jsFunction.isUndefined() && jsUtils.isFunctionValue( jsFunction ) ) {

            Value argv[] = {
                OBJECT_TO_JSVAL( jsCommand )
            };

            Value jsRet;
            if( !JS_CallFunctionValue( _cxCheck, _dbgFacade, jsFunction, 1, argv, &jsRet ) ) {
                // Function execution failed.
                cout << "Function failed with exception: " << jsUtils.getPendingExceptionMessage() << endl;
                result = false;
            } else {
                result = jsRet.toBoolean();
            }
        }

        return result;
    }

    // Creates new JS Context for test script.
    static JSContext *create( JSContext *ctx ) {
        JSContext *cx = JS_NewContext(JS_GetRuntime(ctx), 8192);
        if (!cx) {
            throw runtime_error( "Cannot initialize context for test script." );
        }
        return cx;
    }

private:
    string _script;
    JSContext *_cxCheck;
    JSContext *_cxTest;
    Heap<JSObject*> _dbgFacade;
    Heap<JSObject*> _globalTest;
};

// Sends command to a debugger engine.
static JSBool JS_dbgFacade_fn_sendCommand( JSContext *context, unsigned int argc, Value *vp ) {

    CallArgs args = CallArgsFromVp(argc, vp);

    if( argc != 1 ) {
        JS_ReportError( context, "Bad args!" );
        return JS_FALSE;
    }

    MozJSUtils jsUtils(context);

    DebuggeeScript *dbgScript = static_cast<DebuggeeScript*>(JS_GetContextPrivate( context ));
    if( !dbgScript ) {
        JS_ReportError( context, "There is no debuggee in the context's private data." );
        return JS_FALSE;
    }

    string command;

    if( !jsUtils.stringifyToUtf8( args.get(0), command ) ) {
        JS_ReportError( context, "Cannot stringify command." );
        return JS_FALSE;
    }

    DebuggerStateHint hint;
    if( !dbgScript->sendCommand( command, hint ) ) {
        JS_ReportError( context, "Command couldn't be called." );
        return JS_FALSE;
    }

    args.rval().setInt32( static_cast<int32_t>(hint) );

    return JS_TRUE;
}

// Evaluates test script.
static JSBool JS_dbgFacade_fn_start( JSContext *context, unsigned int argc, Value *vp ) {

    CallArgs args = CallArgsFromVp(argc, vp);

    MozJSUtils jsUtils(context);

    DebuggeeScript *dbgScript = static_cast<DebuggeeScript*>(JS_GetContextPrivate( context ));
    if( !dbgScript ) {
        JS_ReportError( context, "There is no debuggee in the context's private data." );
        return JS_FALSE;
    }

    try {

        Value value = dbgScript->start();

        // The main scripts run in a different context, so wrap the test script result.
        if( !JS_WrapValue( context, &value ) ) {
            JS_ReportError( context, "Cannot wrap result value." );
            return JS_FALSE;
        }

        args.rval().set( value );

    } catch( runtime_error &exc ) {
        cout << exc.what() << endl;
        JS_ReportError( context, "Test script failed." );
        return JS_FALSE;
    }

    return JS_TRUE;
}

// Functions defined for debugger facade.
static JSFunctionSpec JS_DbgFuntions[] = {
    { "sendCommand", JSOP_WRAPPER ( JS_dbgFacade_fn_sendCommand ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
    { "start", JSOP_WRAPPER ( JS_dbgFacade_fn_start ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
    JS_FS_END
};

/**
 * Run test with given ID.
 */
static JSBool JSR_fn_test( JSContext *ctx, unsigned int argc, Value *vp ) {

    CallArgs args = CallArgsFromVp(argc, vp);

    if( argc < 2 ) {
        cout << "JSR_fn_test:: Function 'test' called with bad arguments." << endl;
        JS_ReportError( ctx, "JSR_fn_test:: Bad args." );
        return JS_FALSE;
    }

    bool suspended = false;
    if( argc > 2 ) {
        suspended = args.get(2).toBoolean();
    }

    MozJSUtils jsUtils(ctx);

    if( !jsUtils.isFunctionValue( args.get(1) ) ) {
        cout << "The second argument has to be a function." << endl;
        JS_ReportError( ctx, "JSR_fn_test:: Bad args." );
        return JS_FALSE;
    }

    string testID;

    JS::RootedString jsTestID(ctx, args.get(0).toString());
    if( !jsUtils.toString( jsTestID, testID ) ) {
        JS_ReportError( ctx, "JSR_fn_test:: Cannot get test ID." );
        return JS_FALSE;
    }

    // Creates a new object for the "environment" property.
    RootedObject dbg( ctx, JS_NewObject( ctx, NULL, NULL, NULL ) );
    if( !dbg ) {
        JS_ReportError( ctx, "JSR_fn_test:: Cannot create debugger script." );
        return JS_FALSE;
    }

    if ( !JS_DefineFunctions( ctx, dbg, &JS_DbgFuntions[0] ) ) {
        JS_ReportError( ctx, "JSR_fn_test:: Cannot install debugger functions." );
        return JS_FALSE;
    }

    try {

        string *scriptSource = Resources::getStringResource( testID );
        if( !scriptSource ) {
            string msg = "JSR_fn_test:: Script not found: " + testID;
            JS_ReportError( ctx, msg.c_str() );
            return JS_FALSE;
        }

        JSDbgEngineOptions options;

        if( suspended ) {
            options.suspended();
        }

        DebuggeeScript script(dbg, *scriptSource, ctx, options);

        Value argv[] = {
            OBJECT_TO_JSVAL( dbg )
        };

        JS_SetContextPrivate( ctx, &script );

        Value jsResult;
        if( !JS_CallFunctionValue( ctx, NULL, args.get(1), 2, argv, &jsResult ) ) {
            // Just pass it through.
            if( !JS_IsExceptionPending( ctx ) ) {
                JS_ReportError( ctx, "JSR_fn_test:: Cannot invoke test callback function." );
            }
            return JS_FALSE;
        }

        args.rval().setNull();

    } catch( exception &exc ) {
        string msg("JSR_fn_test:: Cannot install debugger: ");
        msg += exc.what();
        JS_ReportError( ctx, msg.c_str() );
        return JS_FALSE;
    }

    return JS_TRUE;
}

static JSFunctionSpec JS_EnvironmentFuntions[] = {
    { "print", JSOP_WRAPPER ( JS_common_fn_print ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
    { "test", JSOP_WRAPPER ( JSR_fn_test ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
    JS_FS_END
};

// Runs main test control script.
int runTests( JSContext *cx ) {

    CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);
    JS::RootedObject global(cx, JS_NewGlobalObject(cx, &JSR_TestEngineGlobalGlass, nullptr, options));

    if ( !global ) {
        return INIT_ERROR;
    }

    // Gets global object which is to be a debuggee in our context.
    {
        JSAutoRequest req(cx);
        JSAutoCompartment ac(cx, global);

        MozJSUtils jsUtils(cx);

        /* Set the context's global */
        JS_InitStandardClasses(cx, global);

        // Creates a new object for the "environment" property.
        RootedObject env( cx, JS_NewObject( cx, NULL, NULL, NULL ) );
        if( !env ) {
            return INIT_ERROR;
        }

        Value jsvalEnv = OBJECT_TO_JSVAL( env );
        if( !JS_SetProperty( cx, global, "env", &jsvalEnv ) ) {
            return INIT_ERROR;
        }

        if ( !JS_DefineFunctions( cx, env, &JS_EnvironmentFuntions[0] ) ) {
            return INIT_ERROR;
        }

        string script( _binary_dbg_check_js_start, _binary_dbg_check_js_end - _binary_dbg_check_js_start );

        jsval jsRes;
        if( !jsUtils.evaluateUtf8Script( global, script, "dbg_check.js", &jsRes ) ) {
            cout << "Cannot evaluate test script." << endl;
            return INIT_ERROR;
        } else {
            return jsRes.toInt32();
        }
    }
}

int main(int argc, char **argv) {

    setlocale( LC_ALL, "" );

    // Creates new inner compartment in the runtime.
    JSRuntime *rt = JS_NewRuntime( 8L * 1024 * 1024, JS_NO_HELPER_THREADS );
    if (!rt) {
        return INIT_ERROR;
    }

    JS_SetNativeStackQuota( rt, 1024*1024 );
    JS_SetGCParameter( rt, JSGC_MAX_BYTES, 0xffffffff );

    JSContext *cx = JS_NewContext( rt, 8192 );
    if (!cx) {
        return INIT_ERROR;
    }

    int result = runTests( cx );

    JS_DestroyContext( cx );
    JS_DestroyRuntime( rt );
    JS_ShutDown();

    return result;
}


