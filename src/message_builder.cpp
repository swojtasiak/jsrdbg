/*
 * message_builder.cpp
 *
 *  Created on: Mar 24, 2015
 *      Author: tas
 */

#include <string>
#include <sstream>

#include "message_builder.hpp"

using namespace std;
using namespace JSR;

MessageFactory MessageFactory::FACTORY;

MessageFactory::MessageFactory() {
}

MessageFactory::~MessageFactory() {
}

MessageFactory *MessageFactory::getInstance() {
    return &FACTORY;
}

std::string MessageFactory::prepareContextList( const std::vector<JSContextState> &ctxList ) {
    stringstream ss;
    ss << "{\"type\":\"info\",\"subtype\":\"contexts_list\",\"contexts\":[";
    for( std::vector<JSContextState>::const_iterator it = ctxList.begin(); it != ctxList.end(); it++ ) {
        const JSContextState &desc = *it;
        if( it != ctxList.begin() ) {
            ss << ',';
        }
        ss << "{\"contextId\":" << desc.contextId << ','
           << "\"contextName\":\"" << desc.contextName << "\"" << ','
           << "\"paused\":" << ( desc.paused ? "true" : "false" ) << "}";
    }
    ss << "]}";
    return ss.str();
}

string MessageFactory::prepareErrorMessage( ErrorCode errorCode, const string &msg ) {
    stringstream ss;
    ss << "{\"type\":\"error\",\"code\":" << errorCode << ",\"message\":\"" << msg << "\"}";
    return ss.str();
}

string MessageFactory::prepareWarningMessage( WarnCode warnCode, const string &msg ) {
    stringstream ss;
    ss << "{\"type\":\"warn\",\"code\":" << warnCode << ",\"message\":\"" << msg << "\"}";
    return ss.str();
}

