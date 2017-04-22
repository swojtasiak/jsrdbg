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

#include "res_manager.hpp"
#include "utils.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Utils {

ResourceManager::ResourceManager() {
}

ResourceManager::~ResourceManager() {
}

ResourceManager::ResourceManager(ResourceDef *defs) {
    while ( defs && defs->addr ) {
        _resources.insert( resource_pair( defs->name, Resource( defs->addr, defs->len ) ) );
        defs++;
    }
}

void ResourceManager::addResource(const std::string& name, void* addr, size_t len) {
    if ( addr ) {
        _resources.insert( resource_pair( name, Resource( addr, len ) ) );
    } else {
        _resources.erase( name );
    }
}

void ResourceManager::addResource(const std::string& name, const Resource& resource) {
    _resources.insert( resource_pair( name, resource ) );
}

Resource const *ResourceManager::getResource(const std::string& name) const {
    resource_map_const_iterator it = _resources.find( name );
    if ( it != _resources.end() ) {
        return &(it->second);
    }
    return nullptr;
}

#ifdef _WIN32

Resource loadResourceWin32(int name) {
    HMODULE handle = GetModuleHandle(L"jsrdbg");
    HRSRC rc = FindResourceW(handle, MAKEINTRESOURCE(name), RT_RCDATA);
    if (rc == nullptr) {
        auto errnum = GetLastError();
        throw std::runtime_error("Failed loading resource: " + systemErrorString(errnum));
    }
    HGLOBAL rcData = LoadResource(handle, rc);
    auto len = SizeofResource(handle, rc);
    auto addr = static_cast<void*>(LockResource(rcData));
    return Resource(addr, len);
}

#endif

}

