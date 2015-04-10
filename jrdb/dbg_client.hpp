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

#ifndef SRC_DBG_CLIENT_HPP_
#define SRC_DBG_CLIENT_HPP_

#include <string>

#include "events.hpp"
#include "readline.hpp"
#include "debugger.hpp"

class ApplicationCtx {
public:
    ApplicationCtx();
    virtual ~ApplicationCtx();
    // Closes the application.
    virtual void closeApplication() = 0;
    virtual ReadLineEditor &getReadLineEditor() = 0;
    virtual DebuggerEngine &getDebuggerEngine() = 0;
};

class AsyncCommandConsoleDriver : public IConsoleDriver {
public:
    AsyncCommandConsoleDriver();
    virtual ~AsyncCommandConsoleDriver();
public:
    void setEditor( ReadLineEditor &editor );
    void prepareConsole();
    void print( std::string str, va_list args );
    void restoreConsole();
private:
    EditorState _state;
    ReadLineEditor *_editor;
};

class MainEventHandler : public IEventHandler {
public:
    MainEventHandler( ApplicationCtx &ctx );
    ~MainEventHandler();
    void handle( IEvent *event );
private:
    ApplicationCtx &_ctx;
};

#endif /* SRC_DBG_CLIENT_HPP_ */
