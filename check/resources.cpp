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

#include "resources.hpp"

#include <string>

using namespace std;
using namespace JSR;

resource_map Resources::_resources;

// Debugger code for SpiderMonkey.
extern char _binary_ts_simple_calculations_js_start[];
extern char _binary_ts_simple_calculations_js_end[];
extern char _binary_ts_simple_calculations_deb_js_start[];
extern char _binary_ts_simple_calculations_deb_js_end[];
extern char _binary_ts_simple_calculations_functions_js_start[];
extern char _binary_ts_simple_calculations_functions_js_end[];
extern char _binary_ts_debugger_statements_js_start[];
extern char _binary_ts_debugger_statements_js_end[];
extern char _binary_ts_deep_stacktrace_js_start[];
extern char _binary_ts_deep_stacktrace_js_end[];

#define TEST_RC(symbol) createResource( _binary_##symbol##_start, _binary_##symbol##_end )

std::string Resources::createResource( const char *start, const char *end ) {
    return string( start, end - start );
}

std::string *Resources::getStringResource( const std::string key ) {

    std::string *result = NULL;

    if( _resources.empty() ) {
        _resources.insert( resource_pair( "ts_simple_calculations.js", TEST_RC( ts_simple_calculations_js ) ) );
        _resources.insert( resource_pair( "ts_simple_calculations_deb.js", TEST_RC( ts_simple_calculations_deb_js ) ) );
        _resources.insert( resource_pair( "ts_simple_calculations_functions.js", TEST_RC( ts_simple_calculations_functions_js ) ) );
        _resources.insert( resource_pair( "ts_debugger_statements.js", TEST_RC( ts_debugger_statements_js ) ) );
        _resources.insert( resource_pair( "ts_deep_stacktrace.js", TEST_RC( ts_deep_stacktrace_js ) ) );
    }

    resource_map::iterator it = _resources.find( key );
    if( it != _resources.end() ) {
        result = &it->second;
    }

    return result;
}
