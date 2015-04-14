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

