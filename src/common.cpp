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

#include <jsdbg_common.h>

using namespace JSR;

JSDbgEngineOptions::JSDbgEngineOptions()
    : _suspended(false),
      _continue(false),
      _displacement(0) {
}

JSDbgEngineOptions::~JSDbgEngineOptions() {
}

JSDbgEngineOptions &JSDbgEngineOptions::suspended() {
    _suspended = true;
    return *this;
}

bool JSDbgEngineOptions::isSuspended() const {
    return _suspended;
}

JSDbgEngineOptions &JSDbgEngineOptions::continueWhenNoConnections() {
    _continue = true;
    return *this;
}

bool JSDbgEngineOptions::isContinueWhenNoConnections() const {
    return _continue;
}

JSDbgEngineOptions &JSDbgEngineOptions::setSourceCodeDisplacement( int displacement ) {
    _displacement = displacement;
    return *this;
}

int JSDbgEngineOptions::getSourceCodeDisplacement() const {
    return _displacement;
}
