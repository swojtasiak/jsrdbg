/*
 * A Remote Debugger Example for SpiderMonkey Java Script Engine.
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

/**
 * Character encoding support is definitely broken here. It's quite complex
 * problem, so if you are interested how to handle it correctly head over
 * to the common utility code in $(top_srcdir)/utils
 */

#include <iostream>
#include <locale.h>
#include <stdlib.h>

// SpiderMonkey.
#include <jsapi.h>
#include <jsdbgapi.h>

// JSRDBG.
#include <jsrdbg/jsrdbg.h>

using namespace std;
using namespace JS;
using namespace JSR;

static JSClass global_class = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

// Example script.
extern char _binary_example_dog_js_start[];
extern char _binary_example_dog_js_end[];
extern char _binary_example_print_js_start[];
extern char _binary_example_print_js_end[];

struct JsResource {
    char *source;
    int length;
    string name;
};

JsResource JS_Resources[] = {
    {
        _binary_example_dog_js_start,
        static_cast<int>(_binary_example_dog_js_end - _binary_example_dog_js_start),
        "example_dog.js"
    },
    {
        _binary_example_print_js_start,
        static_cast<int>(_binary_example_print_js_end - _binary_example_print_js_start),
        "example_print.js"
    },
    {
        NULL, 0, ""
    }
};

// Naive implementation of the printing to the standard output.
JSBool JS_fn_print( JSContext *cx, unsigned int argc, Value *vp ) {

    CallArgs args = CallArgsFromVp(argc, vp);

    char *chars = JS_EncodeStringToUTF8( cx, args.get(0).toString() );
    if( !chars ) {
        JS_ReportError( cx, "Cannot convert JS string into a native UTF8 one." );
        return JS_FALSE;
    }

    cout << chars << endl;

    JS_free( cx, chars );

    return JS_TRUE;
}

static JSFunctionSpec JDB_Funcs[] = {
   { "print", JSOP_WRAPPER ( JS_fn_print ), 0, JSPROP_PERMANENT | JSPROP_ENUMERATE },
   { nullptr },
};

// Component responsible for loading script's source code if the JS engine cannot provide it.
class ScriptLoader : public IJSScriptLoader {
public:

    ScriptLoader() { }

    ~ScriptLoader() { }

    int load( JSContext *cx, const std::string &path, std::string &script ) {

        JsResource *jsResources = &JS_Resources[0];

        while(jsResources->source) {
            if(path == jsResources->name) {
                script = string(jsResources->source, jsResources->length);
                return JSR_ERROR_NO_ERROR;
            }
            jsResources++;
        }

        return JSR_ERROR_FILE_NOT_FOUND;
    }

};

ScriptLoader loader;

// Runs example script.
bool RunScript(JSContext *cx, JSRemoteDebugger &dbg, unsigned int scriptNumber) {

    // New global object gets it's own compartments too.
    CompartmentOptions options;
    options.setVersion(JSVERSION_LATEST);
    JS::RootedObject global(cx, JS_NewGlobalObject( cx, &global_class, nullptr,
            options ));
    if( !global ) {
        cout << "Cannot create global object." << endl;
        return false;
    }

    JSAutoRequest req( cx );
    JSAutoCompartment ac( cx, global );

    if( !JS_InitStandardClasses( cx, global ) ) {
        cout << "Cannot initialize standard classes." << endl;
        return false;
    }

    if ( !JS_DefineFunctions( cx, global, &JDB_Funcs[0] ) ) {
        cout << "Cannot initialize utility functions." << endl;
        return false;
    }

    // Register newly created global object into the debugger,
    // in order to make it debuggable.
    if( dbg.addDebuggee( cx, global ) != JSR_ERROR_NO_ERROR ) {
        cout << "Cannot add debuggee." << endl;
        return false;
    }

    Value rval;
    JSBool result;

    cout << "Use jrdb command in order to connect to the debugger." << endl;
    cout << "Application is suspended." << endl;

    // Run Garbage collector.
    JS_GC( JS_GetRuntime( cx ) );

    cout << "Evaluating chosen script: " << JS_Resources[scriptNumber].name << endl;

    // Runs JS script.
    result = JS_EvaluateScript( cx, global.get(),
            JS_Resources[scriptNumber].source,
            JS_Resources[scriptNumber].length,
            JS_Resources[scriptNumber].name.c_str(), 0, &rval);

    cout << "Application has been finished." << endl;

    return static_cast<bool>( result );
}

// Initializes debugger and runs script into its scope.
bool RunDbgScript(JSContext *cx, bool suspect, unsigned int scriptNumber) {

    // Initialize debugger.

    // Configure remote debugger.
    JSRemoteDebuggerCfg cfg;
    cfg.setTcpHost(JSR_DEFAULT_TCP_BINDING_IP);
    cfg.setTcpPort(JSR_DEFAULT_TCP_PORT);
    cfg.setScriptLoader(&loader);

    // Configure debugger engine.
    JSDbgEngineOptions dbgOptions;
    // Suspend script just after starting it.
    if (suspect) {
        dbgOptions.suspended();
    }

    JSRemoteDebugger dbg( cfg );

    if( dbg.install( cx, "example-JS", dbgOptions ) != JSR_ERROR_NO_ERROR ) {
        cout << "Cannot install debugger." << endl;
        return false;
    }

    if( dbg.start() != JSR_ERROR_NO_ERROR ) {
        dbg.uninstall( cx );
        cout << "Cannot start debugger." << endl;
        return false;
    }

    cin.ignore(std::numeric_limits<streamsize>::max(),'\n');

    cout << "Debugger has been installed. Press ENTER to continue...";

    cin.get();

    bool result = RunScript( cx, dbg, scriptNumber);

    dbg.stop();
    dbg.uninstall( cx );

    return result;
}

bool ReadYesNo() {
    string yn;
    cin >> yn;
    return yn == "y";
}

int main(int argc, char **argv) {

    setlocale(LC_ALL, "");

    // Creates new inner compartment in the runtime.
    JSRuntime *rt = JS_NewRuntime( 8L * 1024 * 1024, JS_NO_HELPER_THREADS );
    if( !rt ) {
        cout << "Cannot initialize runtime." << endl;
        exit(1);
    }

    JS_SetNativeStackQuota(rt, 1024 * 1024);

    JS_SetGCParameter(rt, JSGC_MAX_BYTES, 0xffffffff);

    JSContext *cx = JS_NewContext( rt, 8192 );
    if( !cx ) {
        JS_DestroyRuntime( rt );
        JS_ShutDown();
        cout << "Cannot initialize JS context." << endl;
        exit(1);
    }

    cout << "Suspect the script after loading? (y/n) ";
    bool suspect = ReadYesNo();

    for (int i = 1; JS_Resources[i-1].length > 0; i++) {
        cout << i << ". Script: " << JS_Resources[i-1].name << endl;
    }

    cout << "Choose script to run: ";

    unsigned int script;

    cin >> script;

    if (script >= sizeof(JS_Resources)/sizeof(JsResource)) {
        cout << "Wrong script number: " << script;
        exit(1);
    }

    if (suspect) {
        cout << "Script will be suspected after loading." << endl;
    }

    if( !RunDbgScript(cx, suspect, script - 1) ) {
        cout << "Application failed." << endl;
    }

    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();

}
