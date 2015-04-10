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

#include "resources.hpp"

#include <string>

using namespace std;

Resource::Resource(size_t length)
    : _length( length ) {
}

Resource::Resource(const Resource &cpy)
    : _length( cpy._length) {
}

Resource &Resource::operator=(const Resource &cpy) {
    if( &cpy != this ) {
        _length = cpy._length;
    }
    return *this;
}

Resource::~Resource() {
}

bool Resource::isEmpty() {
    return _length == 0;
}

size_t Resource::getLength() {
    return _length;
}

StringResource::StringResource()
    : Resource( 0 ) {
}

StringResource::StringResource( size_t length, const string value )
    : Resource( length ), _value( value ) {
}

StringResource::StringResource( const StringResource &cpy )
    : Resource( cpy ), _value( cpy._value ) {
}

StringResource &StringResource::operator=(const StringResource &cpy) {
    if( &cpy != this ) {
        Resource::operator = (cpy);
        _value  = cpy._value;
    }
    return *this;
}

StringResource::~StringResource() {
}

const std::string& StringResource::getValue() {
    return _value;
}

Resources::Resources() {
}

// Debugger for SpiderMonkey.
extern char _binary_mozjs_dbg_client_js_start[];
extern char _binary_mozjs_dbg_client_js_end[];

StringResource Resources::getStringResource( Keys key ) {
    StringResource result;
    switch( key ) {
    case MOZJS_DEBUGGER_CLIENT: {
        size_t len = _binary_mozjs_dbg_client_js_end - _binary_mozjs_dbg_client_js_start;
        result = StringResource( len, string( _binary_mozjs_dbg_client_js_start,  len ) );
        break;
    }
    default:
        break;
    }
    return result;
}
