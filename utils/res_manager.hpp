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

#ifndef UTILS_RESOURCES_HPP_
#define UTILS_RESOURCES_HPP_

#include <stddef.h>
#include <string>
#include <map>

namespace Utils {

/* Describes one resource element. */
struct ResourceDef {
    const std::string name;
    void *addr;
    size_t len;
};

/* Holds resource element in the map. */
struct Resource {
    Resource(void *addr, size_t len) {
        this->addr = addr;
        this->len = len;
    }
    std::string toString() const {
        return std::string( reinterpret_cast<char*>( this->addr ), len );
    }
    /* Location where a resource is placed in the memory. */
    void *addr;
    /* It's length in bytes. */
    size_t len;
};

#define RES_NULL        { "", NULL, 0 }

typedef std::pair<std::string, Resource> resource_pair;
typedef std::map<std::string, Resource> resource_map;
typedef std::map<std::string, Resource>::iterator resource_map_iterator;
typedef std::map<std::string, Resource>::const_iterator resource_map_const_iterator;

class ResourceManager {
public:
    ResourceManager(ResourceDef *defs);
    ResourceManager();
    virtual ~ResourceManager();
public:
    void addResource(const std::string name, void *addr, size_t len);
    Resource const *getResource(const std::string name) const;
private:
    resource_map _resources;
};

}

#endif /* UTILS_RESOURCES_HPP_ */
