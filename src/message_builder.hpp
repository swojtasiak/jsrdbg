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

#ifndef SRC_MESSAGE_BUILDER_HPP_
#define SRC_MESSAGE_BUILDER_HPP_

#include <vector>

namespace JSR {

struct JSContextState {
    // Name of the JS context.
    std::string contextName;
    // Generated id of the context.
    int contextId;
    // True if context is paused.
    bool paused;
};

// Factory which can be used to create JSON messages.
class MessageFactory {
public:
    enum ErrorCode {
        CE_COMMAND_FAILED = 1,
        CE_UNKNOWN_CONTEXT_ID = 2
    };
    enum WarnCode {
        CW_ENGINE_PAUSED = 1
    };
    static MessageFactory *getInstance();
    std::string prepareContextList( const std::vector<JSContextState> &ctxList, const std::string &requestId );
    std::string prepareServerVersion( const std::string &version, const std::string &requestId );
    std::string prepareErrorMessage( ErrorCode errorCode, const std::string &msg );
    std::string prepareWarningMessage( WarnCode warnCode, const std::string &msg );
private:
    MessageFactory();
    ~MessageFactory();
    static MessageFactory FACTORY;
};

}

#endif /* SRC_MESSAGE_BUILDER_HPP_ */
