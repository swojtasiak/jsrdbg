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

#include "js_resources.hpp"

extern char _binary_mozjs_dbg_client_js_start[];
extern char _binary_mozjs_dbg_client_js_end[];

using namespace Utils;

namespace JRDB {

ResourceDef _utils_res_defs[] = {
    { "mozjs_dbg_client", _binary_mozjs_dbg_client_js_start, _binary_mozjs_dbg_client_js_end - _binary_mozjs_dbg_client_js_start },
    RES_NULL
};

ResourceManager _utilsResourceManager( _utils_res_defs );

ResourceManager &GetResourceManager() {
    return _utilsResourceManager;
}

}
