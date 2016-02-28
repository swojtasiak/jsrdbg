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

#include <stdlib.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#include <unistd.h>
#include <signal.h>

#include "getopt_config.hpp"
#include "tcp_client.hpp"
#include "fsevents.hpp"
#include "readline.hpp"
#include "dbg_client.hpp"
#include "js_debugger.hpp"

using namespace std;

static void signal_handler(int signo) {
    ReadLine &rl = ReadLine::getInstance();
    EditorState state("quit", 5);
    rl.restoreEditor(state);
}

#ifdef HAVE_TERMIOS_H
class SaveTerminalAttributes {
public:
    SaveTerminalAttributes() {
        tcgetattr( STDIN_FILENO, &_attrs );
    }
    ~SaveTerminalAttributes() {
        restore();
    }
    void restore() {
        tcsetattr( STDIN_FILENO, TCSANOW, &_attrs );
    }

private:
    termios _attrs;
};
#endif

class ApplicationCtxImpl : public ApplicationCtx {
public:
    ApplicationCtxImpl()
        : _mainLoop(NULL),
          _readLine(NULL),
          _debugger(NULL) {
    };
    ~ApplicationCtxImpl() {
    };
public:
    // ApplicationCtx
    void closeApplication() {
        _mainLoop->abort();
    }
    ReadLineEditor &getReadLineEditor() {
        return *_readLine;
    }
    DebuggerEngine &getDebuggerEngine() {
        return *_debugger;
    }
public:
    void setMainLoop( IEventLoop *mainLoop ) {
        _mainLoop = mainLoop;
    }
    void setReadLineEditor( ReadLineEditor *rl ) {
        _readLine = rl;
    }
    void setDebuggerEngine( DebuggerEngine *debuggerEngine) {
        _debugger = debuggerEngine;
    }
private:
    IEventLoop *_mainLoop;
    ReadLineEditor *_readLine;
    DebuggerEngine *_debugger;
};

class DebuggerCtxImpl : public DebuggerCtx {
public:
    DebuggerCtxImpl( ReadLineEditor &readLine, TCPClient &client )
        : _readLine(readLine),
          _client(client),
          _consoleDriver(NULL) {
    }
    ~DebuggerCtxImpl() {
    }
    void sendCommand( const DebuggerCommand &command ) {
        _client.sendCommand(command);
    }
    ReadLineEditor& getEditor() {
        return _readLine;
    }
    void print( std::string str, ... ) {
        va_list args;
        va_start( args, str );
        if( _consoleDriver ) {
            _consoleDriver->prepareConsole();
            _consoleDriver->print(str, args);
        } else {
            _readLine.print(str, args);
        }
        va_end( args );
    }
    void registerConsoleDriver(IConsoleDriver *driver) {
        // Just in case.
        deleteConsoleDriver();
        // Register new driver and connect it to a readline editor.
        _consoleDriver = driver;
        _consoleDriver->setEditor(_readLine);
    }
    void deleteConsoleDriver() {
        if( _consoleDriver ) {
            _consoleDriver->restoreConsole();
            delete _consoleDriver;
            _consoleDriver = NULL;
        }
    }
private:
    ReadLineEditor &_readLine;
    TCPClient &_client;
    IConsoleDriver *_consoleDriver;
};

int main(int argc, char **argv) {

#ifdef HAVE_TERMIOS_H
    // Store terminal attributes.
    SaveTerminalAttributes termAttrs;
#endif

    // Sets the environment's default locale.
    setlocale(LC_ALL, "");

    // Register signal handler.
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = signal_handler;
    if (sigaction(SIGINT, &sa, NULL)) {
        cout << "Could not register signal handler" << endl;
        exit(1);
    }

    // Parse configuration.
    Configuration configuration;
    GetoptConfigParser parser( argc, argv );
    if( !parser.parse( configuration ) ) {
        exit(1);
    }

    // Register read line implementation.
    ReadLine &rlConsumer = ReadLine::getInstance();

    // Connect to a debugger instance.
    TCPClient *client;
    int error = TCPClient::Connect( configuration.getHost(), configuration.getPort(), &client );
    if( error ) {
        cout << strerror( errno ) << endl;
        exit(1);
    }

    // Initialize JS engine.
    DebuggerCtxImpl dbgCtx(rlConsumer, *client);
    JSDebugger dbg(dbgCtx);
    error = dbg.init();
    if( error ) {
        dbg.destroy();
        cout << "Cannot initialize JS engine: " << error << endl;
        exit(1);
    }

    // Information for the client.
    cout << "JavaScript Remote Debugger Client connected to a remote debugger.\nWaiting for a list of JavaScript contexts being debugged.\nType \"help context\" for more information." << endl;

    ApplicationCtxImpl ctx;

    MainEventHandler mainEventHandler( ctx );

    client->setEventHandler( &mainEventHandler );

    // Prepare FS Events loop.
    std::vector<IFSEventProducer*> producers;
    producers.push_back( client );

    std::vector<IFSEventConsumer*> consumers;
    consumers.push_back(client);

    rlConsumer.registerReadline( &mainEventHandler );
    consumers.push_back( &rlConsumer );

    // The main debugger's loop.
    FSEventLoop eventsLoop(producers, consumers);

    // Initialize the main context.
    ctx.setMainLoop( &eventsLoop );
    ctx.setReadLineEditor( &rlConsumer );
    ctx.setDebuggerEngine( &dbg );

    error = eventsLoop.loop();
    if( error ) {
        cout << "Debugger interrupted with error: " << error << endl;
    }

    client->setEventHandler(NULL);

    client->disconnect(JDB_ERROR_NO_ERROR);
    delete client;

    dbg.destroy();

    exit(0);
}
