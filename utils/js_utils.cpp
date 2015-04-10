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

#include "js_utils.hpp"

#include <string>

#include "encoding.hpp"
#include "log.hpp"

using namespace Utils;
using namespace JS;
using namespace std;

ExceptionState::ExceptionState( JSContext *context ) {
    _context = context;
    _state = JS_SaveExceptionState(context);
}

ExceptionState::~ExceptionState() {
    restore();
}

void ExceptionState::restore() {
    if(_state) {
        JS_RestoreExceptionState( _context, _state );
        _state = NULL;
    }
}

MozJSUtils::MozJSUtils(JSContext *context)
    : _context(context),
      _lastError(0) {
}

MozJSUtils::~MozJSUtils() {
}

bool MozJSUtils::toString( JS::HandleString str, std::string &dest ) {

    JSAutoRequest req( _context );

    jstring jstr;
    if( !toString( str, jstr ) ) {
        return false;
    }

    try {
        JCharEncoder encoder;
        dest = encoder.wideToEnv(jstr);
    } catch( EncodingFailedException &exc ) {
        LoggerFactory::getLogger().error( "MozJSUtils::toString - %s", exc.getMsg().c_str() );
        return false;
    }

    return true;

}

bool MozJSUtils::toString( JS::HandleString str, jstring &dest ) {

    JSAutoRequest req( _context );

    const jschar *chars = JS_GetStringCharsZ( _context, str );
    if( !chars ) {
        return false;
    }

    dest = chars;

    return true;
}

bool MozJSUtils::toUTF8( JS::Value str, std::string &dest ) {

    RootedString jsStr( _context, JSVAL_TO_STRING( str ) );

    return toUTF8( jsStr, dest );

}

bool MozJSUtils::toUTF8( JS::HandleString str, std::string &dest ) {

    JSAutoRequest req( _context );

    jstring jstr;
    if( !toString( str, jstr ) ) {
        return false;
    }

    try {
        JCharEncoder encoder;
        dest = encoder.wideToUtf8(jstr);
    } catch( EncodingFailedException &exc ) {
        LoggerFactory::getLogger().error( "MozJSUtils::toString - %s", exc.getMsg().c_str() );
        return false;
    }

    return true;

}

bool MozJSUtils::fromString( const std::string &str, JS::MutableHandleString dest ) {

    JSAutoRequest req( _context );

    jstring wideStr;

    // Convert MBCS into wide characters using UTF-16.
    try {
        JCharEncoder encoder;
        wideStr = encoder.envToWide(str);
    } catch( EncodingFailedException &exc ) {
        LoggerFactory::getLogger().error( "MozJSUtils::fromString - %s", exc.getMsg().c_str() );
        return false;
    }

    return fromString( wideStr, dest );

}

bool MozJSUtils::fromString( const jstring &str, JS::MutableHandleString dest ) {

    JSAutoRequest req( _context );

    // Creates JS String object instance using prepared wide characters.
    JSString *jstr = JS_NewUCStringCopyN(_context, str.c_str(), str.length() );
    if( !jstr ) {
        _lastError = ERROR_JS_STRING_FAILED;
        LoggerFactory::getLogger().error( "MozJSUtils::fromString - Cannot create new JS string object." );
        return false;
    }

    dest.set( jstr );

    _lastError = 0;

    return true;

}

bool MozJSUtils::fromUTF8( const std::string &str, JS::MutableHandleString dest ) {

    JSAutoRequest req( _context );

    jstring wideStr;

    // Convert UTF-8 into wide characters using UTF-16.
    try {
        JCharEncoder encoder;
        wideStr = encoder.utf8ToWide(str);
    } catch( EncodingFailedException &exc ) {
        LoggerFactory::getLogger().error( "MozJSUtils::fromString - %s", exc.getMsg().c_str() );
        _lastError = ERROR_CHAR_ENCODING_FAILED;
        return false;
    }

    return fromString( wideStr, dest );

}

bool MozJSUtils::argsToString(unsigned int argc, JS::Value *argv, std::string &out) {

    jstring jout;

    if( !argsToString( argc, argv, jout ) ) {
        return false;
    }

    try {
        JCharEncoder encoder;
        out = encoder.wideToEnv(jout);
    } catch( EncodingFailedException &exc ) {
        LoggerFactory::getLogger().error( "MozJSUtils::argsToString - %s", exc.getMsg().c_str() );
        return false;
    }

    return true;
}

bool MozJSUtils::argsToString(unsigned int argc, JS::Value *argv, jstring &out) {

    bool result = true;

    JSAutoRequest req(_context);

    try {

        jstringstream sb;

        // We need space as UTF-16LE encoded string just to
        // use it as an arguments separator.
        JCharEncoder encoder;
        jstring space = encoder.envToWide(" ");

        for (int n = 0; n < argc; ++n) {

            JSExceptionState *excState;

            /* JS_ValueToString might throw, in which we will only
             * log that the value could be converted to string */
            ExceptionState state(_context);

            RootedString jstr( _context, JS_ValueToString( _context, argv[n] ) );

            state.restore();

            if (jstr) {

                jstring s;
                if( !toString(jstr, s) ) {
                    LoggerFactory::getLogger().error( "MozJSUtils::argsToString - Cannot convert JSString into a jstring." );
                    result = false;
                    break;
                }

                sb << s;

                if (n < (argc - 1)) {
                    sb << jstring(space, 1);
                }

            } else {
                LoggerFactory::getLogger().error( "MozJSUtils::argsToString - JS_ValueToString failed." );
                result = false;
                break;
            }

        }

        out = sb.str();

    } catch( EncodingFailedException &exc ) {
        LoggerFactory::getLogger().error( "MozJSUtils::argsToString - %s", exc.getMsg().c_str() );
        result = false;
    }

    return result;
}

bool MozJSUtils::evaluateUtf8Script( JSObject *global, const string &script, const char *fileName, jsval *outRetval ) {

    jstring jscript;
    try {
        JCharEncoder encoder;
        jscript = encoder.utf8ToWide(script);
    } catch( EncodingFailedException &exc ) {
        _lastError = ERROR_CHAR_ENCODING_FAILED;
        return false;
    }

   return evaluateScript( global, jscript, fileName, outRetval );
}

bool MozJSUtils::parseUtf8JSON(const std::string &str, JS::MutableHandleObject dest) {

    // Convert UTF8 string to wide.
    jstring jcommand;
    try {
        JCharEncoder encoder;
        jcommand = encoder.utf8ToWide( str );
    } catch( EncodingFailedException &exc ) {
        _lastError = ERROR_CHAR_ENCODING_FAILED;
        return false;
    }

    // We are not interested in exceptions inside
    // the command handler, they are silently ignored
    // just because we do not have any logging facility yet.
    ExceptionState exState(_context);

    RootedValue parsedJSON(_context);
    if( !JS_ParseJSON( _context, jcommand.c_str(), jcommand.size(), &parsedJSON ) ) {
        _lastError = ERROR_PARSING_FAILED;
        return false;
    }

    dest.set(&parsedJSON.toObject());

    return true;
}

bool MozJSUtils::evaluateScript( JSObject *global, const jstring &script, const char *fileName, jsval *outRetval ) {

    JSAutoRequest ar(_context);
    JSAutoCompartment cm(_context, global);

    if(JS_IsExceptionPending(_context)) {
        LoggerFactory::getLogger().error( "evaluateScript:: Unexpected pending exception." );
        _lastError = ERROR_PENDING_EXCEPTION;
        return false;
    }

    ExceptionState state(_context);

    JS::CompileOptions options(_context);
    options.setUTF8(true)
            .setFileAndLine(fileName, 0)
            .setSourcePolicy(JS::CompileOptions::LAZY_SOURCE);

    js::RootedObject rootedObj(_context, global);

    jsval retval = JSVAL_VOID;
    if (!JS::Evaluate(_context, rootedObj, options, script.c_str(), script.size(), &retval)) {
        _lastError = ERROR_EVALUATION_FAILED;
        return false;
    }

    if ( JS_IsExceptionPending(_context) ) {
        MozJSUtils jsUtils(_context);
        string msg = jsUtils.getPendingExceptionMessage();
        LoggerFactory::getLogger().error( "evaluateScript:: Exception: %s.", msg.c_str() );
        _lastError = ERROR_EVALUATION_FAILED;
        return false;
    }

    if(outRetval) {
        *outRetval = retval;
    }

    _lastError = 0;

    return true;
}

JSCompartment* MozJSUtils::getCurrentCompartment(JSObject *global) {
   // Gets the current compartment address, this a bit tricky,
   // but there is no API to get the address of the current compartment.
   JSCompartment *compartment = JS_EnterCompartment( _context, global );
   JS_LeaveCompartment( _context, compartment );
   return compartment;
}

namespace Utils {

    // Copies all 16-bit UNICODE characters into the provided buffer.
    JSBool JSONCommandWriteCallback(const jschar *buf, uint32_t len, void *data) {
        if( !data ) {
            return JS_FALSE;
        }
        basic_string<jschar> *buffer = reinterpret_cast< basic_string<jschar>* >( data );
        buffer->append( buf, len );
        return JS_TRUE;
    }

}

bool MozJSUtils::stringifyToUtf8( JS::Value value, std::string &result ) {

    basic_string<jschar> stringifiedValue;
    if( !JS_Stringify( _context, &value, NULL, JS::NullHandleValue, &JSONCommandWriteCallback, &stringifiedValue ) ) {
        _lastError = ERROR_JS_STRINGIFY_FAILED;
        return false;
    }

    try {
        JCharEncoder encoder;
        result = encoder.wideToUtf8( stringifiedValue );
    } catch( EncodingFailedException &exc ) {
        _lastError = ERROR_CHAR_ENCODING_FAILED;
        return false;
    }

    return true;
}

bool MozJSUtils::setPropertyObj( JS::HandleObject obj, const std::string &property, JS::HandleObject jsObj ) {
    Value jsvalObj = OBJECT_TO_JSVAL( jsObj );
    if( !JS_SetProperty( _context, obj, property.c_str(), &jsvalObj ) ) {
        return false;
    }
    return true;
}

bool MozJSUtils::setPropertyInt( JS::HandleObject obj, const std::string &property, int value ) {
    Value jsval;
    jsval.setInt32(value);
    if( !JS_SetProperty( _context, obj, property.c_str(), &jsval ) ) {
        return false;
    }
    return true;
}

bool MozJSUtils::setPropertyInt32( JS::HandleObject obj, const std::string &property, uint32_t value ) {
    Value jsval;
    jsval.setInt32(value);
    if( !JS_SetProperty( _context, obj, property.c_str(), &jsval ) ) {
        return false;
    }
    return true;
}

bool MozJSUtils::setPropertyBool( JS::HandleObject obj, const std::string &property, bool value ) {
    Value jsval;
    jsval.setBoolean(value);
    if( !JS_SetProperty( _context, obj, property.c_str(), &jsval ) ) {
        return false;
    }
    return true;
}

bool MozJSUtils::setPropertyStr( JS::HandleObject obj, const std::string &property, const std::string &value ) {

    RootedString jsString(_context);
    if( !fromString(value, &jsString) ) {
        return false;
    }

    Value jsValue = STRING_TO_JSVAL( jsString );
    if( !JS_SetProperty( _context, obj, property.c_str(), &jsValue ) ) {
        return false;
    }

    return true;
}

std::string MozJSUtils::getPendingExceptionMessage() {
    string message = "No message";
    Value exc;
    if( JS_GetPendingException(_context, &exc) ) {
        Value jsMessage;
        RootedString jsMessageStr(_context);
        if( JS_GetProperty( _context, &exc.toObject(), "message", &jsMessage ) ) {
            jsMessageStr.set(jsMessage.toString());
        } else {
            // No message property.
            ExceptionState state(_context);
            RootedString jsExcStr( _context, JS_ValueToString( _context, exc ) );
            jsMessageStr.set(jsExcStr);
        }
        if( !toString(jsMessageStr, message) ) {
            message = "Cannot retrieve message from the exception.";
        }
    }
    return message;
}

std::string MozJSUtils::getPendingExceptionStack() {
    string message = "No stack";
    Value exc;
    if( JS_GetPendingException(_context, &exc) ) {
        Value jsMessage;
        RootedString jsMessageStr(_context);
        if( JS_GetProperty( _context, &exc.toObject(), "stack", &jsMessage ) ) {
            jsMessageStr.set(jsMessage.toString());
        } else {
            // No stack property.
            ExceptionState state(_context);
            RootedString jsExcStr( _context, JS_ValueToString( _context, exc ) );
            jsMessageStr.set(jsExcStr);
        }
        if( !toString(jsMessageStr, message) ) {
            message = "Cannot retrieve stack from the exception.";
        }
    }
    return message;
}

int MozJSUtils::getLastError() {
    return _lastError;
}

bool MozJSUtils::isFunctionValue(JS::Value fn) {
    return isFunctionObject( JSVAL_TO_OBJECT( fn ) );
}

bool MozJSUtils::isFunctionObject(JSObject* fn) {
    return JS_ObjectIsFunction( _context, fn );
}

bool MozJSUtils::splitCommand( const std::string &packet, int &contextId, std::string &jsonCommand ) {
    // Check if there is context ID in the command string.
    bool result = true;
    size_t sep = packet.find( '/' );
    if( sep != string::npos ) {
        size_t json = packet.find( '{' );
        if( sep != string::npos && sep < json ) {
            char *end;
            contextId = static_cast<int>( strtol( packet.c_str(), &end, 10 ) );
            if( end != packet.c_str() + sep ) {
                // Broken context ID.
                result = false;
                contextId = -1;
            } else {
                jsonCommand = packet.substr( sep + 1 );
            }
        } else {
            jsonCommand = packet;
        }
    } else {
        jsonCommand = packet;
    }
    return result;
}
