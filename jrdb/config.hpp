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

#ifndef SRC_CONFIGURATION_HPP_
#define SRC_CONFIGURATION_HPP_

#include <string>

#define JDB_CONFIG_DEFAULT_PORT    8089

class Configuration {
public:

    Configuration() {
        _port = JDB_CONFIG_DEFAULT_PORT;
        _host = "127.0.0.1";
        _verbose = false;
    }

    ~Configuration() {}

    const std::string &getHost() {
        return _host;
    }

    void setHost( const std::string &host ) {
        _host = host;
    }

    int getPort() const {
        return _port;
    }

    void setPort(int port) {
        _port = port;
    }

    bool isVerbose() const {
        return _verbose;
    }

    void setVerbose(bool verbose) {
        _verbose = verbose;
    }

private:
    std::string _host;
    int _port;
    bool _verbose;
};

#endif /* SRC_CONFIGURATION_HPP_ */
