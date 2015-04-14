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

#ifndef PUBLIC_JSDBG_COMMON_H_
#define PUBLIC_JSDBG_COMMON_H_

#define JSR_ERROR_NO_ERROR                              0
#define JSR_ERROR_OUT_OF_MEMORY                         1
#define JSR_ERROR_UNKNOWN_PROTOCOL                      2
#define JSR_ERROR_UNSPECIFIED_ERROR                     3
#define JSR_ERROR_BAD_IP                                4
#define JSR_ERROR_PORT_IN_USE                           5
#define JSR_ERROR_CANNOT_CREATE_SOCKET                  6
#define JSR_ERROR_CANNOT_CHANGE_SOCKET_OPTS             7
#define JSR_ERROR_CANNOT_RESOLVE_HOST_NAME              8
#define JSR_ERROR_CANNOT_BIND_SOCKET                    9
#define JSR_ERROR_CANNOT_LISTEN_TO_SOCKET               10
#define JSR_ERROR_RECV_FAILED                           11
#define JSR_ERROR_INTERNAL_PIPE_FAILED                  12
#define JSR_ERROR_WOULD_BLOCK                           13
#define JSR_ERROR_FILE_NOT_FOUND                        14
#define JSR_ERROR_CONNECTION_CLOSED                     15
#define JSR_ERROR_ILLEGAL_ARGUMENT                      16
#define JSR_ERROR_CANNOT_READ_FILE                      17
#define JSR_ERROR_CANNOT_REMOVE_CONNECTIONS             18
#define JSR_ERROR_DEBUGGER_ALREADY_INSTALLED            19
#define JSR_ERROR_DEBUGGER_NOT_INSTALLED                20
#define JSR_ERROR_DEBUGGER_ALREADY_STARTED              21
#define JSR_ERROR_DEBUGGER_NOT_STARTED                  22

#define JSR_ERROR_SM_CANNOT_CREATE_GLOBAL_OBJECT        100
#define JSR_ERROR_SM_CANNOT_WRAP_OBJECT                 101
#define JSR_ERROR_SM_FAILED_TO_INITIALIZE_STD_CLASSES   102
#define JSR_ERROR_SM_FAILED_TO_INITIALIZE_REFLECT       103
#define JSR_ERROR_SM_FAILED_TO_INITIALIZE_DEBUGGER      104
#define JSR_ERROR_SM_CANNOT_DEFINE_FUNCTION             105
#define JSR_ERROR_SM_CANNOT_CREATE_OBJECT               106
#define JSR_ERROR_SM_CANNOT_SET_PROPERTY                107
#define JSR_ERROR_SM_UNEXPECTED_PENDING_EXCEPTION       108
#define JSR_ERROR_SM_CANNOT_EVALUATE_SCRIPT             109
#define JSR_ERROR_SM_CANNOT_REGISTER_DEBUGGEE           110
#define JSR_ERROR_SM_DEBUGGER_IS_NOT_INSTALLED          111
#define JSR_ERROR_SM_DEBUGGER_ALREADY_INSTALLED         112
#define JSR_ERROR_SM_CANNOT_SHUTDOWN_DEBUGGER           113

namespace JSR {

class JSDbgEngineOptions {
public:
    JSDbgEngineOptions();
    ~JSDbgEngineOptions();
public:
    // Makes the debugger to suspend as soon as possible
    // when new debuggee instance is added. The script and
    // the line doesn't matter, it will suspend in the first
    // possible location which is a part of the debuggee added.
    JSDbgEngineOptions &suspended();
    bool isSuspended() const;
    // Ignore breakpoints if none of the clients is connected.
    JSDbgEngineOptions &continueWhenNoConnections();
    bool isContinueWhenNoConnections() const;
    // Sets displacement value for source code. Useful if you would like
    // to use one-based source code lines.
    JSDbgEngineOptions &setSourceCodeDisplacement( int displacement );
    int getSourceCodeDisplacement() const;
private:
    bool _suspended;
    bool _continue;
    int _displacement;
};

// Describes state change hint. When command is
// sent to the debugger it can have significant impact
// on the debugger state. This is a hint for the user
// to inform him how the debugger state should change.
// For instance when 'step' command is called, engine
// should return 'ES_CONTINUE' state. It means that
enum DebuggerStateHint {
    // Debugger should continue if it's paused.
    ES_CONTINUE = 1,
    // Leave the debugger in the state it is in.
    ES_IGNORE = 2,
    // Debugger has been interrupted, do whatever you want
    // but the debugger is shutting down.
    ES_INTERRUPTED = 3
};

}

#endif /* PUBLIC_JSDBG_COMMON_H_ */
