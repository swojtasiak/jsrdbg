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

#ifndef UTILS_JS_UTILS_HPP_
#define UTILS_JS_UTILS_HPP_

#include <string>
#include <jsapi.h>
#include <jsdbgapi.h>

#include "encoding.hpp"
#include "res_manager.hpp"

namespace Utils {

class ExceptionState {
public:
    ExceptionState( JSContext *context );
    ~ExceptionState();
    void restore();
private:
    JSContext *_context;
    JSExceptionState *_state;
};

class MozJSUtils {
public:
    MozJSUtils(JSContext *context);
    ~MozJSUtils();
public:
    const static int ERROR_CHAR_ENCODING_FAILED = 1;
    const static int ERROR_EVALUATION_FAILED = 2;
    const static int ERROR_PENDING_EXCEPTION = 3;
    const static int ERROR_JS_STRING_FAILED = 4;
    const static int ERROR_PARSING_FAILED = 5;
    const static int ERROR_JS_STRINGIFY_FAILED = 6;
public:
    // Character converting.
    bool toString( JS::HandleString str, std::string &dest );
    bool toString( JS::HandleString str, jstring &dest );
    bool toUTF8( JS::HandleString str, std::string &dest );
    bool toUTF8( JS::Value str, std::string &dest );
    bool fromString( const std::string &str, JS::MutableHandleString dest );
    bool fromString( const jstring &str, JS::MutableHandleString dest );
    bool fromUTF8( const std::string &str, JS::MutableHandleString dest );
    // Arguments interpretation.
    bool argsToString(JS::CallArgs &args, std::string &out);
    bool argsToString(JS::CallArgs &args, jstring &out);
    // Scripts evaluation.
    bool evaluateUtf8Script( JSObject *global, const std::string &script, const char *fileName, jsval *outRetval );
    bool evaluateScript( JSObject *global, const jstring &script, const char *fileName, jsval *outRetval );
    // JSON support.
    bool parseUtf8JSON(const std::string &str, JS::MutableHandleObject dest);
    // Compartments.
    JSCompartment* getCurrentCompartment(JSObject *global);
    // Exception handling.
    std::string getPendingExceptionMessage();
    std::string getPendingExceptionStack();
    // Object properties.
    bool setPropertyInt( JS::HandleObject obj, const std::string &property, int value );
    bool setPropertyInt32( JS::HandleObject obj, const std::string &property, uint32_t value );
    bool setPropertyBool( JS::HandleObject obj, const std::string &property, bool value );
    bool setPropertyStr( JS::HandleObject obj, const std::string &property, const std::string &value );
    bool setPropertyObj( JS::HandleObject obj, const std::string &property, JS::HandleObject jsObj );
    // Parsing and stringifying.
    bool stringifyToUtf8( JS::Value value, std::string &result );
    // Errors.
    int getLastError();
    // Object types checking.
    bool isFunctionValue( JS::Value fn );
    bool isFunctionObject( JSObject *fn );
    // Command parsing.
    static bool splitCommand( const std::string &packet, int &contextId, std::string &jsonCommand );
    // Support for module loading.
    bool registerModuleLoader( JSObject *global );
    bool addResourceManager( JSObject *global, const std::string &prefix, ResourceManager &resourceManager );
private:
    JSContext *_context;
    int _lastError;
};

}

#endif /* UTILS_JS_UTILS_HPP_ */
