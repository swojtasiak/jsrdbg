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
#include <stdlib.h>
#include <getopt.h>

#include "getopt_config.hpp"

GetoptConfigParser::GetoptConfigParser( int argc, char **argv )
    : _argc( argc ),
      _argv( argv ) {
}

GetoptConfigParser::~GetoptConfigParser() {
}

bool GetoptConfigParser::parse( Configuration &configuration ) {

    static const char help_message[] =
            "Usage: jdb [OPTIONS]\n" \
            "Connects to a remote java script debugger.\n\n"
            "  -v,  --verbose          enable verbose output\n" \
            "  -p,  --port             remote TCP port\n" \
            "  -h,  --host             remote domain or IP address\n" \
            "       --help             display this help and exit\n\n" \
            "By default it tries to connect to 127.0.0.1 using port 8089.\n\n" \
            "Examples:\n" \
            "  jdb --port=8080 --host=example.com   Connects to the debugger\n" \
            "                                       exposed by example.com on\n" \
            "                                       port 8080.\n\n" \
            "Report bugs to: slawomir@wojtasiak.com\n" \
            "pkg home page: <https://github.com/swojtasiak/jsrdbg>\n";

    bool result = true;

    int is_help = 0;

    while(result) {

        int option_index = 0;
        int c;

        int this_option_optind = optind ? optind : 1;

        static struct option long_options[] = {
            /* These options set a flag. */
            {"verbose", no_argument,       0,        'v'},
            {"port",    required_argument, 0,        'p'},
            {"host",    required_argument, 0,        'h'},
            {"help",    no_argument,       &is_help, 1},
            {0, 0, 0, 0}
        };

        c = getopt_long(_argc, _argv, "vp:h:", long_options, &option_index);
        if (c == -1) {
            /* No more options. */
            break;
        }

        switch( c ) {
        case 0:
            if( is_help ) {
                result = false;
            }
            break;
        case 'v':
            configuration.setVerbose(true);
            break;
        case 'p':
            configuration.setPort(atoi(optarg));
            break;
        case 'h':
            configuration.setHost(optarg);
            break;
        case '?':
            /* Unrecognized option. */
            result = false;
            break;
        default:
            printf( "Unsupported option returned: -%c\n", optopt );
            result = false;
            break;
        }

    }

    if( is_help ) {
        // Prints help message.
        printf(help_message);
        return false;
    }

    if ( result && optind < _argc ) {
        printf ("Unknown arguments: ");
        while(optind < _argc ) {
            printf ("%s ", _argv[optind++]);
        }
        putchar ('\n');
    }

    if( !result ) {
        printf("Try 'jdb --help' for more information.\n");
    }

    return result;
}
