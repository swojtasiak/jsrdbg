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

using namespace Utils;

#ifdef __unix__
extern char _binary_mozjs_dbg_js_start[];
extern char _binary_mozjs_dbg_js_end[];

extern char _binary_module_js_start[];
extern char _binary_module_js_end[];
#endif

namespace JSR {

#ifdef __unix__
ResourceDef _jsrdbg_res_defs[] = {
    { "mozjs_dbg", _binary_mozjs_dbg_js_start, _binary_mozjs_dbg_js_end - _binary_mozjs_dbg_js_start },
    { "module", _binary_module_js_start, _binary_module_js_end - _binary_module_js_start },
    RES_NULL
};
#endif

ResourceManager &GetResourceManager() {

#ifdef __unix__
    static ResourceManager _jsrdbgResourceManager( _jsrdbg_res_defs );
#elif defined(_WIN32)
    static ResourceManager _jsrdbgResourceManager = [] () -> ResourceManager {
        ResourceManager mgr;
        mgr.addResource("mozjs_dbg", loadResourceWin32("mozjs_dbg"));
        mgr.addResource("module", loadResourceWin32("module"));
        return mgr;
    }();
#endif
    return _jsrdbgResourceManager;
}

}
