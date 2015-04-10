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

#include "debugger.hpp"

DebuggerCommandEvent::DebuggerCommandEvent( const std::string &str )
    : StringEvent(str) {
}

DebuggerCommandEvent::~DebuggerCommandEvent() {
}

DebuggerCommand::DebuggerCommand( int contextId, const std::string content )
    : _contextId( contextId ),
      _content( content ) {
}

DebuggerCommand::~DebuggerCommand() {
}

const std::string& DebuggerCommand::getContent() const {
    return _content;
}

int DebuggerCommand::getContextId() const {
    return _contextId;
}

IConsoleDriver::IConsoleDriver() {
}

IConsoleDriver::~IConsoleDriver() {
}

DebuggerCtx::DebuggerCtx() {
}

DebuggerCtx::~DebuggerCtx() {
}

DebuggerEngine::DebuggerEngine( DebuggerCtx &ctx )
    : _ctx(ctx) {
}

DebuggerEngine::~DebuggerEngine() {
}
