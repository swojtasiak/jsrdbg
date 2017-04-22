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

#include <iostream>

#include <js_utils.hpp>

#include "dbg_client.hpp"
#include "tcp_client.hpp"

using namespace std;
using namespace Utils;

AsyncCommandConsoleDriver::AsyncCommandConsoleDriver() {
    _editor = nullptr;
}

AsyncCommandConsoleDriver::~AsyncCommandConsoleDriver() {
}

void AsyncCommandConsoleDriver::setEditor( ReadLineEditor &editor ) {
    _editor = &editor;
}

void AsyncCommandConsoleDriver::prepareConsole() {
    if( _state.isEmpty() ) {
        _state =_editor->hideEditor();
    }
}

void AsyncCommandConsoleDriver::print( std::string str, va_list args ) {
    _editor->print( str, args );
}

void AsyncCommandConsoleDriver::restoreConsole() {
    if( !_state.isEmpty() ) {
        _editor->restoreEditor(_state);
        _state.clear();
    }
}

ApplicationCtx::ApplicationCtx() {
}

ApplicationCtx::~ApplicationCtx() {
}

MainEventHandler::MainEventHandler( ApplicationCtx &ctx )
    : _ctx( ctx ) {
}

MainEventHandler::~MainEventHandler() {
}

void MainEventHandler::handle( IEvent *event ) {

    ReadLineEditor &editor = _ctx.getReadLineEditor();

    // Command from debugger engine.
    if( dynamic_cast<DebuggerCommandEvent*>( event ) ) {

        // Asynchronous commands need a console driver just
        // in order to handle console hiding and restoring.
        DebuggerEngine& debugger = _ctx.getDebuggerEngine();
        DebuggerCtx &ctx = debugger.getDebuggerCtx();
        ctx.registerConsoleDriver( new AsyncCommandConsoleDriver() );

        DebuggerCommandEvent *debuggerEvent = dynamic_cast<DebuggerCommandEvent*>( event );
        string commandStr = debuggerEvent->str();

        int contextId = -1;
        MozJSUtils::splitCommand( commandStr, contextId, commandStr );

        DebuggerCommand command( contextId, commandStr );

        debugger.sendCommand(command);

        ctx.deleteConsoleDriver();

    // Control command from GUI.
    } else if( dynamic_cast<StringEvent*>( event ) ) {
        StringEvent *commandEvent = dynamic_cast<StringEvent*>( event );
        string command = commandEvent->str();
        if( command == "q" || command == "quit" || command == "exit" ) {
            _ctx.closeApplication();
            editor.unregisterReadline();
        } else {
            DebuggerEngine& debugger = _ctx.getDebuggerEngine();
            int error = debugger.sendCtrlCommand(command);
            if( error ) {
                ReadLineEditor &editor = _ctx.getReadLineEditor();
                editor.print("Cannot execute command: %d\n", error);
            }
        }

    // Client has been disconnected.
    } else if( dynamic_cast<ClientDisconnectedEvent*>( event ) ) {
        ClientDisconnectedEvent *cde = dynamic_cast<ClientDisconnectedEvent*>( event );
        ReadLineEditor &editor = _ctx.getReadLineEditor();
        editor.hideEditor();
        if( cde->getReason() == JDB_ERROR_NO_ERROR || cde->getReason() == JDB_ERROR_FILE_DESCRIPTOR_CLOSED ) {
            editor.print("Remote connection closed.\n");
        } else {
            editor.print("Remote connection closed due to: %d\n", cde->getReason());
        }
        _ctx.closeApplication();
        editor.unregisterReadline();
    }

    delete event;
}
