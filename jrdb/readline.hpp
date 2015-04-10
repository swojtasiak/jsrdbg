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

#ifndef SRC_READLINE_HPP_
#define SRC_READLINE_HPP_

#include <string>
#include <stdarg.h>

#include "fsevents.hpp"

class EditorState {
public:
    EditorState( const std::string& line, int position );
    EditorState();
    ~EditorState();
    const std::string& getLine();
    int getPoint();
    bool isEmpty();
    void clear();
private:
    std::string _line;
    int _point;
};

class ReadLineEditor {
public:
    ReadLineEditor();
    virtual ~ReadLineEditor();
public:
    virtual void registerReadline( IEventHandler *handler ) = 0;
    virtual void unregisterReadline() = 0;
    virtual void print( std::string str, ... ) = 0;
    virtual void print( std::string str, va_list args ) = 0;
    virtual void restoreEditor( EditorState &state) = 0;
    virtual EditorState hideEditor() = 0;
};

class ReadLine : public IFSEventConsumer, public ReadLineEditor {
public:
    // IEventConsumer
    virtual bool consume( IEvent *event );
    virtual void closeConsumer(int error);
    // IFSEventConsumer
    virtual int getConsumerFD();
    virtual int read();
    // ReadLineEditor
    void registerReadline( IEventHandler *handler );
    void unregisterReadline();
    void print( std::string str, ... );
    void print( std::string str, va_list args );
    void restoreEditor( EditorState &state);
    EditorState hideEditor();
public:
    static ReadLine &getInstance();
    static void dispose();
private:
    ReadLine();
    virtual ~ReadLine();
};

#endif /* SRC_READLINE_HPP_ */
