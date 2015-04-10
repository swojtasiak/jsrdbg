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

#ifndef SRC_RESOURCES_H_
#define SRC_RESOURCES_H_

#include <string>
#include "stddef.h"

namespace JSR {

class Resource {
public:
    Resource( size_t length );
    Resource( const Resource &cpy );
    Resource &operator=( const Resource &cpy );
    virtual ~Resource();
    size_t getLength() const;
    bool isEmpty() const;
private:
    size_t _length;
};

class StringResource : public Resource {
public:
    StringResource();
    StringResource( size_t length, const std::string &value );
    StringResource( const StringResource &cpy );
    StringResource &operator=( const StringResource &cpy );
    virtual ~StringResource();
    const std::string& getValue() const;
private:
    std::string _value;
};

/**
 * Gets static resource as string.
 */
std::string getStringResource();

class Resources {
public:

    enum Keys {
        MOZJS_DEBUGGER
    };

    static StringResource getStringResource( Keys key );

private:
    Resources();
};

}

#endif /* SRC_RESOURCES_H_ */
