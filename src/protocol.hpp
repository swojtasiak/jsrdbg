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


#ifndef JSR_SRC_PROTOCOL_H_
#define JSR_SRC_PROTOCOL_H_

#include "client.hpp"

namespace JSR {

class Protocol {
public:
    Protocol();
    virtual ~Protocol();
public:
    virtual int init() = 0;
    virtual int startProtocol() = 0;
    virtual int stopProtocol() = 0;
    virtual ClientManager& getClientManager() = 0;
};

}


#endif /* JSR_SRC_PROTOCOL_H_ */
