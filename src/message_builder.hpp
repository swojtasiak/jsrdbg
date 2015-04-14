/*
 * message_builder.hpp
 *
 *  Created on: Mar 24, 2015
 *      Author: tas
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
    std::string prepareContextList( const std::vector<JSContextState> &ctxList );
    std::string prepareErrorMessage( ErrorCode errorCode, const std::string &msg );
    std::string prepareWarningMessage( WarnCode warnCode, const std::string &msg );
private:
    MessageFactory();
    ~MessageFactory();
    static MessageFactory FACTORY;
};

}

#endif /* SRC_MESSAGE_BUILDER_HPP_ */
