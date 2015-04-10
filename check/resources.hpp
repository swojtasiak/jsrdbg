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

#ifndef SRC_RESOURCES_H_
#define SRC_RESOURCES_H_

#include <string>
#include <map>
#include "stddef.h"

namespace JSR {

typedef std::map<std::string, std::string> resource_map;
typedef std::pair<std::string, std::string> resource_pair;

class Resources {
public:
    static std::string *getStringResource( const std::string key );
private:
    static std::string createResource( const char *start, const char *end );
    static resource_map _resources;
private:
    Resources();
};

}

#endif /* SRC_RESOURCES_H_ */
