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

#include "resources.hpp"

#include <string>

using namespace std;
using namespace JSR;

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

bool Resource::isEmpty() const {
    return _length == 0;
}

size_t Resource::getLength() const {
    return _length;
}

StringResource::StringResource()
    : Resource( 0 ) {
}

StringResource::StringResource( size_t length, const string &value )
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

const std::string& StringResource::getValue() const {
    return _value;
}

Resources::Resources() {
}

// Debugger code for SpiderMonkey.
extern char _binary_mozjs_dbg_js_start[];
extern char _binary_mozjs_dbg_js_end[];

StringResource Resources::getStringResource( Keys key ) {
    StringResource result;
    switch( key ) {
    case MOZJS_DEBUGGER: {
        size_t len = _binary_mozjs_dbg_js_end - _binary_mozjs_dbg_js_start;
        result = StringResource( len, string( _binary_mozjs_dbg_js_start,  len ) );
        break;
    }
    default:
        break;
    }
    return result;
}
