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

#ifndef SRC_ERRORS_HPP_
#define SRC_ERRORS_HPP_

#define JDB_ERROR_NO_ERROR                          0
#define JDB_ERROR_SELECT_FAILED                     1
#define JDB_ERROR_BUFFER_IS_FULL                    2
#define JDB_ERROR_WOULD_BLOCK                       3
#define JDB_ERROR_READ_ERROR                        4
#define JDB_ERROR_WRITE_ERROR                       5
#define JDB_ERROR_FILE_DESCRIPTOR_CLOSED            6
#define JDB_ERROR_CANNOT_CREATE_SOCKET              7
#define JDB_ERROR_CANNOT_RESOLVE_HOST_NAME          8
#define JDB_ERROR_CANNOT_CONNECT                    9
#define JDB_ERROR_CANNOT_SET_SOCKET_NONBLOCK        10
#define JDB_ERROR_JS_ENGINE_FAILED                  11
#define JDB_ERROR_JS_CANNOT_CREATE_CONTEXT          12
#define JDB_ERROR_JS_CANNOT_CREATE_GLOBAL           13
#define JDB_ERROR_JS_CODE_NOT_FOUND                 14
#define JDB_ERROR_JS_DEBUGGER_SCRIPT_FAILED         15
#define JDB_ERROR_JS_DEBUGGER_PENDING_EXC           16
#define JDB_ERROR_OUT_OF_MEMORY                     17
#define JDB_ERROR_JS_CANNOT_CREATE_OBJECT           18
#define JDB_ERROR_JS_CANNOT_SET_PROPERTY            19
#define JDB_ERROR_JS_CANNOT_DEFINE_FUNCTION         20
#define JDB_ERROR_JS_FUNCTION_FAILED                21
#define JDB_ERROR_MALICIOUS_DATA                    22
#define JDB_ERROR_JS_JSON_PARSING_FAILED            23

#endif /* SRC_ERRORS_HPP_ */
