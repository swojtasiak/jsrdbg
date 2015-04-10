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

#ifndef SRC_DEBUGGER_HPP_
#define SRC_DEBUGGER_HPP_

#include <string>

#include "readline.hpp"

/**
 * Event used to send incoming debugger command to the engine.
 */
class DebuggerCommandEvent : public StringEvent {
public:
    DebuggerCommandEvent( const std::string &str );
    virtual ~DebuggerCommandEvent();
};

/**
 * Whole communication between the debugger engine and the remote
 * JS debugger is done using this command class.
 */
class DebuggerCommand {
public:
    DebuggerCommand( int contextId, const std::string content );
    ~DebuggerCommand();
public:
    const std::string& getContent() const;
    int getContextId() const;
private:
    int _contextId;
    const std::string _content;
};

/**
 * Prepares console for printing messages.
 */
class IConsoleDriver {
public:
    IConsoleDriver();
    virtual ~IConsoleDriver();
public:
    virtual void setEditor( ReadLineEditor &editor ) = 0;
    virtual void prepareConsole() = 0;
    virtual void print( std::string str, va_list args ) = 0;
    virtual void restoreConsole() = 0;
};

/**
 * Provides a gate to the environment for the debugger implementation.
 */
class DebuggerCtx {
public:
    DebuggerCtx();
    virtual ~DebuggerCtx();
    virtual ReadLineEditor &getEditor() = 0;
    virtual void sendCommand( const DebuggerCommand &command ) = 0;
    virtual void print( std::string str, ... ) = 0;
    virtual void registerConsoleDriver( IConsoleDriver *driver ) = 0;
    virtual void deleteConsoleDriver() = 0;
};

/**
 * Debugger engine implementation.
 */
class DebuggerEngine {
public:
    DebuggerEngine( DebuggerCtx &ctx );
    virtual ~DebuggerEngine();
    /**
     * Initialized the debugger engine.
     * @return Error code.
     */
    virtual int init() = 0;
    /**
     * Destroys the debugger engine.
     * @return Error code.
     */
    virtual int destroy() = 0;
public:
    /**
     * Sends command to the debugger engineer.
     */
    virtual int sendCommand( const DebuggerCommand &command ) = 0;
    /**
     * Sends control command to the debugger.
     */
    virtual int sendCtrlCommand( const std::string &command ) = 0;
    /**
     * Returns debugger context.
     */
    virtual DebuggerCtx &getDebuggerCtx() = 0;
protected:
    DebuggerCtx &_ctx;
};

#endif /* SRC_DEBUGGER_HPP_ */
