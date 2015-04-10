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

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

#include "events.hpp"
#include "readline.hpp"

using namespace std;

#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else /* !defined(HAVE_READLINE_H) */
extern char *readline ();
#  endif /* !defined(HAVE_READLINE_H) */
char *cmdline = NULL;
#else /* !defined(HAVE_READLINE_READLINE_H) */
  /* no readline */
#endif /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  else /* !defined(HAVE_HISTORY_H) */
extern void add_history ();
extern int write_history ();
extern int read_history ();
#  endif /* defined(HAVE_READLINE_HISTORY_H) */
  /* no history */
#endif /* HAVE_READLINE_HISTORY */

const char *prompt = "jrdb> ";

IEventHandler *_JDB_eventHandler = NULL;

ReadLine *_JDB_consumer;

static void gnu_rl_cb_linehandler( char *line ) {
    if( _JDB_eventHandler && line && *line ) {
#ifdef HAVE_READLINE_HISTORY
        ::add_history (line);
#endif
        StringEvent *event = new StringEvent( line );
        _JDB_eventHandler->handle( event );
    }
    ::rl_free(line);
}

ReadLine::ReadLine() {
}

ReadLine::~ReadLine() {
}

void ReadLine::restoreEditor( EditorState &state) {
    // Restore editor sate.
    ::rl_set_prompt(prompt);
    ::rl_replace_line(state.getLine().c_str(), 0);
    rl_point = state.getPoint();
    // Show the editor.
    ::rl_redisplay();
}

EditorState ReadLine::hideEditor() {
    // Save editor state.
    char* savedLine;
    int savedPoint;
    savedPoint = rl_point;
    savedLine = rl_copy_text(0, rl_end);
    EditorState state(savedLine, savedPoint);
    ::rl_free( savedLine );
    // Hide editor's prompt.
    ::rl_set_prompt("");
    ::rl_replace_line("", 0);
    ::rl_redisplay();
    return state;
}

void ReadLine::print( string str, ... ) {
    va_list args;
    va_start( args, str );
    ::vprintf( str.c_str(), args );
    va_end( args );
}

void ReadLine::print( std::string str, va_list args ) {
    vprintf( str.c_str(), args );
}

void ReadLine::registerReadline( IEventHandler *handler ) {
    // Installs line handler.
    ::rl_callback_handler_install ( prompt, gnu_rl_cb_linehandler );
    _JDB_eventHandler = handler;
}

void ReadLine::unregisterReadline() {
    // Clear the current prompt and edited line, just to
    // not leave any garbages in the terminal.
    ::rl_callback_handler_remove();
    _JDB_eventHandler = NULL;
}

ReadLine &ReadLine::getInstance() {
    if( !_JDB_consumer ) {
        _JDB_consumer = new ReadLine();
    }
    return *_JDB_consumer;
}

void ReadLine::dispose() {
    if( _JDB_consumer ) {
        delete _JDB_consumer;
        _JDB_consumer = NULL;
    }
}

// IEventConsumer
bool ReadLine::consume( IEvent *event ) {
    if( _JDB_eventHandler ) {
        _JDB_eventHandler->handle(event);
    }
}

void ReadLine::closeConsumer( int error ) {
    // Do nothing, this is singleton based on the global
    // unique standard input file descriptor.
    unregisterReadline();
}

int ReadLine::getConsumerFD() {
    return fileno (rl_instream);
}

int ReadLine::read() {
    ::rl_callback_read_char();
    return 0;
}

EditorState::EditorState() {
    _point = -1;
}

EditorState::EditorState( const std::string &line, int point )
    : _line(line),
      _point(point) {
}

EditorState::~EditorState() {
}

const std::string& EditorState::getLine() {
    return _line;
}

int EditorState::getPoint() {
    return _point;
}

bool EditorState::isEmpty() {
    return _point == -1;
}

void EditorState::clear() {
    _point = -1;
    _line = "";
}

ReadLineEditor::ReadLineEditor() {
}

ReadLineEditor::~ReadLineEditor() {
}
