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

#ifndef SRC_RESOURCES_H_
#define SRC_RESOURCES_H_

#include <string>
#include "stddef.h"

class Resource {
public:
    Resource(size_t length);
    Resource(const Resource &cpy);
    Resource &operator=(const Resource &cpy);
    virtual ~Resource();
    size_t getLength();
    bool isEmpty();
private:
    size_t _length;
};

class StringResource : public Resource {
public:
    StringResource();
    StringResource( size_t length, const std::string value );
    StringResource( const StringResource &cpy );
    StringResource &operator=(const StringResource &cpy);
    virtual ~StringResource();
    const std::string& getValue();
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
        MOZJS_DEBUGGER_CLIENT
    };

    static StringResource getStringResource( Keys key );

private:
    Resources();
};

#endif /* SRC_RESOURCES_H_ */
