/*
 * Unit tests for SpiderMonkey Java Script Engine Debugger.
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

(function() {

    const ES_CONTINUE = 1;
    const ES_IGNORE = 2;
    const ES_INTERRUPTED = 3;

    try {

        env.print("[INFO]: Starting unit tests.");

        PrepareMockingEngine( env );

        /**************************
         * operation: 'get_source'
         **************************/

        env.test( 'ts_simple_calculations_deb.js', function( dbg ) {
            
            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 2, source: 'debugger;' } );
            dbg.one().pause().fn( function() {
            
                // No URL.
                dbg.one().error().id(2).code(5);
                dbg.sendCommand( { id: 2, type: 'command', name: 'get_source' } );
                
                dbg.one().info().id(3).props([
                    "var result = 0;",
                    "result += 1;",
                    "debugger;",
                    "result += 2;",
                    "result += result;",
                    ""], 'source' );
                dbg.sendCommand( { id: 3, type: 'command', name: 'get_source', url: 'test_script.js' } );
                
                dbg.one().error().id(4).code(10);
                dbg.sendCommand( { id: 4, type: 'command', name: 'get_source', url: 'test_script-none.js' } );
                
            });

            dbg.start();

            dbg.checkSatisfied();
        });

        /***********************************
         * operation: 'get_all_source_urls'
         ***********************************/

        env.test( 'ts_simple_calculations_deb.js', function(dbg) {
            
            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 2, source: 'debugger;' } );
            dbg.one().pause().fn( function() {
            
                dbg.one().info().id(2).subtype('all_source_urls').props([
                    'test_script.js'
                ], 'urls');
                dbg.sendCommand( { id: 2, type: 'command', name: 'get_all_source_urls' } );
                
            });

            dbg.start();

            dbg.checkSatisfied();
        });

        /*****************************
         * operation: 'evaluate'
         *****************************/

        env.test( 'ts_deep_stacktrace.js', function(dbg) {

            dbg.one().error().code(4);
            dbg.sendCommand( { id: 1, type: 'command', name: 'evaluate' } );

            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 24, source: '                        debugger;' } );
            dbg.one().pause().fn( function() {
            
                // Delaring function.
                dbg.one().info().id(1);
                dbg.sendCommand( { id: 1, type: 'command', name: 'evaluate', path: 'global_function = function() { return 5; }' } );
                
                dbg.one().info().id(2).subtype('evaluated').props({result: 5});
                dbg.sendCommand( { id: 2, type: 'command', name: 'evaluate', path: 'global_function();' } );
                
            });

            dbg.start();

            dbg.checkSatisfied();

        } );

        env.test( 'ts_deep_stacktrace.js', function(dbg) {

            dbg.one().error().code(4);
            dbg.sendCommand( { id: 1, type: 'command', name: 'evaluate' } );

            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 24, source: '                        debugger;' } );
            dbg.one().pause().fn( function() {
            
                // No path specified.
                dbg.one().error().id(2).code(5);
                dbg.sendCommand( { id: 2, type: 'command', name: 'evaluate' } );
            
                dbg.one().info().id(3).subtype('evaluated').props({result: 'test5'});
                dbg.sendCommand( { id: 3, type: 'command', name: 'evaluate', path: 'l_5_2' } );
                
                dbg.one().info().id(4).subtype('evaluated').props({info: 'test1'}, 'result');
                dbg.sendCommand( { id: 4, type: 'command', name: 'evaluate', path: 'l_1_3' } );
                
                // Recursive pause.
                dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 24, source: '                        debugger;' } );
                dbg.one().pause();
                dbg.one().info().id(5).subtype('evaluated').props({result: 5});
                
                dbg.sendCommand( { id: 5, type: 'command', name: 'evaluate', path: 'level_5();' } );
                
            });

            dbg.start();

            dbg.checkSatisfied();

        } );

        /*****************************
         * operation: 'get_variables'
         *****************************/

        env.test( 'ts_deep_stacktrace.js', function(dbg) {

            dbg.one().error().code(4);
            dbg.sendCommand( { id: 1, type: 'command', name: 'get_stacktrace' } );

            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 24, source: '                        debugger;' } );
            dbg.one().pause().fn( function() {
            
                // No query specified.
                dbg.one().info().id(2).subtype('variables').fn( function( packet ) {
                    // Only a simple test, checking the whole variables tree would be 
                    // too complex and to much susceptible to JS engine version changes.
                    return packet && packet.variables && Array.isArray( packet.variables ) && packet.variables.length === 7;
                });
                dbg.sendCommand( { id: 2, type: 'command', name: 'get_variables' } );
                
                // Empty query specified.
                dbg.one().info().id(4).subtype('variables').fn( function( packet ) {
                    // Only a simple test, checking the whole variables tree would be 
                    // too complex and to much susceptible to JS engine version changes.
                    return packet && packet.variables && Array.isArray( packet.variables ) && packet.variables.length === 7;
                });
                dbg.sendCommand( { id: 4, type: 'command', name: 'get_variables', query: {} } );
                
                // Get only one frame.
                dbg.one().info().id(3).subtype('variables').fn( function( packet ) {
                    return JSON.stringify(packet.variables) === JSON.stringify([
                        {"stackElement":
                            {"url":"test_script.js","line":20,"rDepth":1},
                            "variables":[
                                {"name":"l_4_1","value":4},
                                {"name":"l_4_2","value":"test4"},
                                {"name":"l_4_3","value":{"info":"test4"}},
                                {"name":"arguments","value":{"length":0,"callee":{"___jsrdbg_collapsed___":true}}}
                            ]
                        }]);
                });
                dbg.sendCommand( { id: 3, type: 'command', name: 'get_variables', query: { depth: 1 } } );
            });

            dbg.start();

            dbg.checkSatisfied();

        } );

        /**************************
         * operation: 'stacktrace'
         **************************/

        env.test( 'ts_deep_stacktrace.js', function(dbg) {

            dbg.one().error().code(4);
            dbg.sendCommand( { id: 1, type: 'command', name: 'get_stacktrace' } );

            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 24, source: '                        debugger;' } );
            dbg.one().pause().fn( function() {
                dbg.one().info().id(2).subtype('stacktrace').props( [
                        {"url":"test_script.js","line":24,"rDepth":0},
                        {"url":"test_script.js","line":20,"rDepth":1},
                        {"url":"test_script.js","line":16,"rDepth":2},
                        {"url":"test_script.js","line":12,"rDepth":3},
                        {"url":"test_script.js","line":8,"rDepth":4},
                        {"url":"test_script.js","line":4,"rDepth":5},
                        {"url":"test_script.js","line":0,"rDepth":6}
                    ], 'stacktrace' );
                dbg.sendCommand( { id: 2, type: 'command', name: 'get_stacktrace' } );
            });

            dbg.start();

            dbg.checkSatisfied();

        } );

        /******************
         * operation: 'pc'
         ******************/

        env.test( 'ts_simple_calculations_deb.js', function(dbg) {

            dbg.one().error().code(4);
            dbg.sendCommand( { id: 1, type: 'command', name: 'pc' } );

            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 2, source: 'debugger;' } );
            dbg.one().pause().fn( function() {
            
                // Ask for PC without source.
                dbg.one().info().id(2).subtype('pc').props({
                    script: "test_script.js",
                    line: 2,
                    source: null
                });
                dbg.sendCommand( { id: 2, type: 'command', name: 'pc' } );
                
                // Ask for PC with source.
                dbg.one().info().id(2).subtype('pc').props({
                    script: "test_script.js",
                    line: 2,
                    source: 'debugger;'
                });
                dbg.sendCommand( { id: 2, type: 'command', name: 'pc', source: true } );
            });

            dbg.start();

            dbg.checkSatisfied();

        } );

        /************************
         * operation: 'continue'
         ************************/

        // Only one breakpoint is allowed for given line.
        env.test( 'ts_debugger_statements.js', function(dbg) {

            dbg.cont(0, 'debugger;')
               .cont(2, 'debugger;')
               .cont(4, 'debugger;')
               .cont(6, 'debugger;');

            dbg.start();

            dbg.checkSatisfied();

        } );

        /********************
         * operation: 'next'
         ********************/

        // Only one breakpoint is allowed for given line.
        env.test( 'ts_simple_calculations_functions.js', function(dbg) {

            dbg.step(  0, 'debugger;' )
               .step(  5, 'Utils.sum = function(x, y) {' )
               .step( 12, 'Manager.prototype = {' )
               .step( 13, '    calculate: function( x ) {' )
               .step( 20, '    show: function(x) {' )
               .step( 25, 'var manager = new Manager();' )
               .step( 9,  'function Manager() {' )
               .step( 9,  'function Manager() {' )
               .step( 27, 'manager.calculate(5);' )
               .step( 14, '        return (function() {' )
               .next( 18, '        }).call(this);' )
               .next( 18, '        }).call(this);' )
               .next( 27, 'manager.calculate(5);' )
               ;

            dbg.start();

            dbg.checkSatisfied();

        } );

        // Only one breakpoint is allowed for given line.
        env.test( 'ts_simple_calculations_functions.js', function(dbg) {

            dbg.step(  0, 'debugger;' )
               .step(  5, 'Utils.sum = function(x, y) {' )
               .step( 12, 'Manager.prototype = {' )
               .step( 13, '    calculate: function( x ) {' )
               .step( 20, '    show: function(x) {' )
               .step( 25, 'var manager = new Manager();' )
               .step( 9,  'function Manager() {' )
               .step( 9,  'function Manager() {' )
               .next( 27, 'manager.calculate(5);' )
               .next( 27, 'manager.calculate(5);' )
               ;

            dbg.start();

            dbg.checkSatisfied();

        } );

        /********************
         * operation: 'step'
         ********************/

        // Only one breakpoint is allowed for given line.
        env.test( 'ts_simple_calculations_functions.js', function(dbg) {

            dbg.step(  0, 'debugger;' )
               .step(  5, 'Utils.sum = function(x, y) {' )
               .step( 12, 'Manager.prototype = {' )
               .step( 13, '    calculate: function( x ) {' )
               .step( 20, '    show: function(x) {' )
               .step( 25, 'var manager = new Manager();' )
               .step( 9,  'function Manager() {' )
               .step( 9,  'function Manager() {' )
               .step( 27, 'manager.calculate(5);' )
               .step( 14, '        return (function() {' )
               .step( 18, '        }).call(this);' )
               .step( 15, '            return {' )
               .step( 16, '                result: this.show(Utils.sum(10, x))' )
               .step( 6,  '    return x+y;' )
               .step( 6,  '    return x+y;' )
               .step( 21, '        return x;' )
               .step( 21, '        return x;' )
               .step( 16, '                result: this.show(Utils.sum(10, x))' )
               .step( 18, '        }).call(this);' )           
               .step( 27, 'manager.calculate(5);' )
               ;

            dbg.start();

            dbg.checkSatisfied();

        } );

        // Only one breakpoint is allowed for given line.
        env.test( 'ts_simple_calculations_functions.js', function(dbg) {

            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 0, source: 'debugger;' } );
            dbg.one().pause().fn( function() {
                dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 5, source: 'Utils.sum = function(x, y) {' });
                dbg.one().pause().fn( function() {
                    dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 12, source: 'Manager.prototype = {' });
                    dbg.one().pause().fn( function() {
                    } );
                    dbg.sendCommand( { id: 1, type: 'command', name: 'step' } );
                } );
                dbg.sendCommand( { id: 1, type: 'command', name: 'step' } );
            } );
            
            dbg.start();

            dbg.checkSatisfied();

        } );

        /******************************
         * operation: 'breakpoint_set'
         ******************************/

        // Only one breakpoint is allowed for given line.
        env.test( 'ts_simple_calculations.js', function(dbg) {

            // Registering pending breakpoints.
            dbg.one().info().id(1).subtype('breakpoint_set').props( { url: 'test_script.js', line: 1, pending: true, bid: 0 });
            dbg.one().info().id(2).subtype('breakpoint_set').props( { url: 'test_script.js', line: 1, pending: true, bid: 0 });
            
            // Pending breakpoints are changed to set ones.
            dbg.one().info().subtype('breakpoint_set').props( { url: 'test_script.js', line: 1, pending: false, bid: 0 });

            dbg.sendCommand( { id: 1, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 1, pending: true } } );
            dbg.sendCommand( { id: 2, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 1, pending: true } } );
            
            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 1, source: 'result += 1;' } );
            dbg.one().pause();

            dbg.start();

            dbg.checkSatisfied();

        } );

        // Delete all breakpoints by their ids.
        env.test( 'ts_simple_calculations.js', function(dbg) {

            // Registering pending breakpoints.
            dbg.one().info().id(1).subtype('breakpoint_set').props( { url: 'test_script.js', line: 1, pending: true, bid: 0 });
            dbg.one().info().id(2).subtype('breakpoint_set').props( { url: 'test_script.js', line: 2, pending: true, bid: 1 });
            dbg.one().info().id(3).subtype('breakpoint_set').props( { url: 'test_script.js', line: 3, pending: true, bid: 2 });
            
            // Pending breakpoints are changed to set ones.
            dbg.one().info().subtype('breakpoint_set').props( { url: 'test_script.js', line: 1, pending: false, bid: 0 });
            dbg.one().info().subtype('breakpoint_set').props( { url: 'test_script.js', line: 2, pending: false, bid: 1 });
            dbg.one().info().subtype('breakpoint_set').props( { url: 'test_script.js', line: 3, pending: false, bid: 2 });

            dbg.sendCommand( { id: 1, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 1, pending: true } } );
            dbg.sendCommand( { id: 2, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 2, pending: true } } );
            dbg.sendCommand( { id: 3, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 3, pending: true } } );
            
            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 1, source: 'result += 1;' } );
            
            dbg.one().pause().fn( function() {

                // Check if set breakpoint is available on the list of registered breakpoints and has the right type.
                dbg.one().info().id(4).subtype('breakpoints_list').props( [
                    {bid:0,url:"test_script.js",line:1,pending:false},
                    {bid:1,url:"test_script.js",line:2,pending:false},
                    {bid:2,url:"test_script.js",line:3,pending:false}
                ], 'breakpoints');

                // Get all registered breaskpoints and remove them.
                dbg.sendCommand( { id: 4, type: 'command', name: 'get_breakpoints'} );

                // Error ids not specified.
                dbg.one().error().id(5).code(5);
                dbg.sendCommand( { id: 5, type: 'command', name: 'delete_breakpoint' } );

                // It works just nothing will be removed at all.
                dbg.one().info().id(5).subtype('breakpoint_deleted').props( [], 'ids' );
                dbg.sendCommand( { id: 5, type: 'command', name: 'delete_breakpoint', ids: [] } );

                // Delete all of them. Unexisting breakpoints are just ignored.
                dbg.one().info().id(5).subtype('breakpoint_deleted').props( [0,1,2], 'ids' );
                dbg.sendCommand( { id: 5, type: 'command', name: 'delete_breakpoint', ids: [0,1,2,4,5] } );
                
                dbg.one().info().id(6).subtype('breakpoints_list').props( [], 'breakpoints');
                dbg.sendCommand( { id: 6, type: 'command', name: 'get_breakpoints'} );
            });
            
            dbg.start();

            dbg.checkSatisfied();

        } );

        // Delete all breakpoints.
        env.test( 'ts_simple_calculations.js', function(dbg) {

            // Registering pending breakpoints.
            dbg.one().info().id(1).subtype('breakpoint_set').props( { url: 'test_script.js', line: 1, pending: true, bid: 0 });
            dbg.one().info().id(2).subtype('breakpoint_set').props( { url: 'test_script.js', line: 2, pending: true, bid: 1 });
            dbg.one().info().id(3).subtype('breakpoint_set').props( { url: 'test_script.js', line: 3, pending: true, bid: 2 });
            
            // Pending breakpoints are changed to set ones.
            dbg.one().info().subtype('breakpoint_set').props( { url: 'test_script.js', line: 1, pending: false, bid: 0 });
            dbg.one().info().subtype('breakpoint_set').props( { url: 'test_script.js', line: 2, pending: false, bid: 1 });
            dbg.one().info().subtype('breakpoint_set').props( { url: 'test_script.js', line: 3, pending: false, bid: 2 });

            dbg.sendCommand( { id: 1, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 1, pending: true } } );
            dbg.sendCommand( { id: 2, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 2, pending: true } } );
            dbg.sendCommand( { id: 3, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 3, pending: true } } );
            
            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 1, source: 'result += 1;' } );
            
            dbg.one().pause().fn( function() {

                // Check if set breakpoint is available on the list of registered breakpoints and has the right type.
                dbg.one().info().id(4).subtype('breakpoints_list').props( [
                    {bid:0,url:"test_script.js",line:1,pending:false},
                    {bid:1,url:"test_script.js",line:2,pending:false},
                    {bid:2,url:"test_script.js",line:3,pending:false}
                ], 'breakpoints');

                // Get all registered breaskpoints and remove them.
                dbg.sendCommand( { id: 4, type: 'command', name: 'get_breakpoints'} );

                // Delete all of them.
                dbg.one().info().id(5).subtype('all_breakpoints_deleted');
                dbg.sendCommand( { id: 5, type: 'command', name: 'delete_all_breakpoints'} );

                dbg.one().info().id(6).subtype('breakpoints_list').props( [], 'breakpoints');
                dbg.sendCommand( { id: 6, type: 'command', name: 'get_breakpoints'} );
            });
            
            dbg.start();

            dbg.checkSatisfied();

        } );

        // Deleting breakpoint before it is hit.
        env.test( 'ts_simple_calculations_deb.js', function(dbg) {

            // Pending breakpoint is registered.
            dbg.one().info().id(1).subtype('breakpoint_set').props( {
                url: 'test_script.js',
                line: 3,
                pending: true,
                bid: 0
            });

            dbg.sendCommand( { id: 1, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 3, pending: true } } );
            
            // When script is being loaded breakpoint is set.
            dbg.one().info().subtype('breakpoint_set').props( {
                url: 'test_script.js',
                line: 3,
                pending: false,
                bid: 0
            });

            // Then breakpoint is removed when debugger pauses on 'debugger' statement, so it's not further hit.
            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 2, source: 'debugger;' } );
            dbg.one().pause().fn( function() {
                dbg.one().info().id(1).subtype('breakpoint_deleted').props( [0], 'ids' );
                dbg.sendCommand( { id: 1, type: 'command', name: 'delete_breakpoint', ids: [0] } );
            });

            dbg.start();

            dbg.checkSatisfied();

        } );

        // Adding breakpoint before it's hit.
        env.test( 'ts_simple_calculations_deb.js', function(dbg) {

            // Pending breakpoint is registered.
            dbg.one().info().id(1).subtype('breakpoint_set').props( {
                url: 'test_script.js',
                line: 2,
                pending: true,
                bid: 0
            });

            dbg.sendCommand( { id: 1, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 2, pending: true } } );
            
            // When script is being loaded.
            dbg.one().info().subtype('breakpoint_set').props( {
                url: 'test_script.js',
                line: 2,
                pending: false,
                bid: 0
            });

            // One from breakpoint and the another one from the 'debugger' statement.
            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 2, source: 'debugger;' } );
            dbg.one().pause();
            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 2, source: 'debugger;' } );
            dbg.one().pause();

            dbg.start();

            dbg.checkSatisfied();

        } );

        // Setting breakpoint when paused.
        env.test( 'ts_simple_calculations.js', function(dbg) {
            
            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 0, source: 'var result = 0;' } );
            dbg.one().pause(true).fn( function() {
            
                // Set breakpoint to the line 1.
                dbg.one().info().id(1).subtype('breakpoint_set').props( {
                    url: 'test_script.js',
                    line: 1,
                    pending: false,
                    bid: 0
                });
                
                // Breakpoint hit.
                dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 1, source: 'result += 1;' } );
                dbg.one().pause().fn( function() {

                    // Ok, so now set breakpoint on the line 2.
                    dbg.one().info().id(2).subtype('breakpoint_set').props( {
                        url: 'test_script.js',
                        line: 2,
                        pending: false,
                        bid: 1
                    });
                    
                    // Breakpoint hit.
                    dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 2, source: 'result += 2;' } );
                    dbg.one().pause();
                    
                    // ** Command: Set breakpoint 2.
                    dbg.sendCommand( { id: 2, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 2, pending: false } } );

                });
                
                // ** Command: Set breakpoint 1.
                dbg.sendCommand( { id: 1, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 1, pending: false } } );
            });
            
            dbg.start();

            dbg.checkSatisfied();
            
        }, true );

        // Starting debugger in 'suspended' mode.
        env.test( 'ts_simple_calculations.js', function(dbg) {
            
            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 0, source: 'var result = 0;' } );
            dbg.one().pause(true); // We are waiting for the first pause being a result of the 'suspended' option.
            
            dbg.start();

            dbg.checkSatisfied();
            
        }, true );

        // Setting pending breakpoint.
        env.test( 'ts_simple_calculations.js', function(dbg) {
            
            // Pending breakpoint has to be registered.
            dbg.one().info().id(1).subtype('breakpoint_set').props( {
                url: 'test_script.js',
                line: 1,
                pending: true,
                bid: 0
            });
            dbg.sendCommand( { id: 1, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 1, pending: true } } );
            
            // Check if set breakpoint is available on the list of registered breakpoints and has the right type.
            dbg.one().info().id(2).subtype('breakpoints_list').props( [{
                url: 'test_script.js',
                line: 1,
                pending: true,
                bid: 0
            }], 'breakpoints');
            dbg.sendCommand( { id: 2, type: 'command', name: 'get_breakpoints'} );

            // After the script is loaded, the breakpoint is installed and the appropriate information is broadcasted.
            dbg.one().info().subtype('breakpoint_set').props( {
                url: 'test_script.js',
                line: 1,
                pending: false, // Important!
                bid: 0
            });
            
            // As soon as the execution is broken on the breakpoint information about this fact is broadcasted.
            dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: 1, source: 'result += 1;' } );
            dbg.one().pause();

            dbg.start();
            
            dbg.checkSatisfied();
        });
        
        // Setting not pending breakpoint for unknown script.
        env.test( 'ts_simple_calculations.js', function(dbg) {
            dbg.one().error().code(8);
            dbg.sendCommand( { id: 1, type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 1, pending: false } } );
            dbg.checkSatisfied();
            dbg.start();
        });
        
        // Checks mandatory parameters.
        env.test( 'ts_simple_calculations.js', function(dbg) {
            dbg.one().error().code(5);
            dbg.sendCommand( { type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', pending: true } } );
            dbg.one().error().code(5);
            dbg.sendCommand( { type: 'command', name: 'set_breakpoint', breakpoint: { line:1, pending: true } } );
            dbg.one().error().code(5);
            dbg.sendCommand( { type: 'command', name: 'set_breakpoint' } );
            dbg.checkSatisfied();
        });
        
        // Lines are numbered from 1.
        env.test( 'ts_simple_calculations.js', function(dbg) {
            dbg.one().error().code(10);
            dbg.sendCommand( { type: 'command', name: 'set_breakpoint', breakpoint: { url: 'test_script.js', line: 0, pending: true } } );
            dbg.checkSatisfied();
        });

        env.report();
        
        env.print("[INFO]: Unit tests finished.");
        
    } catch( exc ) {
        env.print( '[ERROR]: ' + exc.message );
        if( exc instanceof Error ) {
            env.print('Stack: ' + exc.stack);
        }
        return 1;
    }
    
    return 0;

})();

function AssertError(expectation, msg) {
    this.message = msg;
    this.expectation = expectation;
    this.stack = new Error().stack;
}

function PrepareMockingEngine( environment ) {
    
    var fn_test = environment.test;
    
    var assertsSuccess = 0;
    var assertsFailure = 0;
    
    environment.report = function() {
        env.print( 'Overall number of asserts satisfied: ' + assertsSuccess );
        env.print( 'Overall number of asserts failed: ' + assertsFailure );
    };
    
    environment.test = function( script, fn, suspended ) {
    
        return fn_test( script, function(dbg) {
        
            var expectations = [];
            var failures = [];
            var assertsSatisfied = 0;
        
            dbg._failures = [];
        
            dbg.one = function() {
                return this.expect(1);
            };

            dbg.sendCommand = (function() {
                let fn = dbg.sendCommand;
                return function( command ) {
                    env.print('Sending command: ' + JSON.stringify(command));
                    return fn( command );
                };
            })();

            var controlTransferFn = function( line, code ) {
                let holder = {};
                let pause = function( line, code, holder ) {
                    var builder = dbg.one().info().subtype('paused').props( { url: 'test_script.js', line: line, source: code } );
                    dbg.one().pause().fn( function() {
                        if( holder.fn ) {
                            holder.fn();
                        }
                    } );
                };
                pause( line, code, holder );
                return (function builder(holder, id) {
                    var ctf = function( command, line, code ) {
                        let currHolder = {};
                        holder.fn = function() {
                            pause( line, code, currHolder );
                            dbg.sendCommand( { id: id, type: 'command', name: command } );
                        };
                        return builder( currHolder, id + 1 );
                    };
                    let next = {};
                    next.step = ctf.bind( next, 'step' );
                    next.next = ctf.bind( next, 'next' );
                    next.cont = ctf.bind( next, 'continue' );
                    return next;
                })( holder, 1 );
            };

            dbg.step = controlTransferFn.bind( dbg );
            dbg.next = dbg.step;
            dbg.cont = dbg.step;

            dbg.expect = function(n) {
                var expection = {
                
                    assert: null,
                    counter: 0,
                    stack: new Error().stack,
                    
                    info: function() {
                        return this._command_exp( 'info', Array.prototype.slice.call(arguments) );
                    },
                    
                    error: function( id, name, fn ) {
                        return this._command_exp( 'error', Array.prototype.slice.call(arguments) );
                    },
                    
                    pause: function( suspended ) {
                        if( !suspended ) {
                            suspended = false;
                        }
                        var fn;
                        this.stack = new Error().stack;
                        this.toString = function() {
                            return '[Assert(pause)]';
                        };
                        this.assert = function( packet ) {
                            if( suspended !== packet.suspended ) {
                                throw new AssertError( this, "Expected suspended: (" + suspended + ") pause, got: " + JSON.stringify( packet ) );
                            }
                            if( ++this.counter > n ) {
                                throw new AssertError( this, "Expected " + n + " invocations, got: " + this.counter );
                            }
                            return {
                                fn: fn,
                                last: this.counter === n
                            };
                        };
                        return {
                            fn: function(fn_) {
                                fn = fn_;
                            }
                        };
                    },
                    
                    check: function( packet ) {
                        if( this.assert ) {
                            return this.assert( packet );
                        }
                        return false;
                    },
                    
                    _command_exp: function( type, args ) {
                    
                        var id;
                        var subtype;
                        var fn;
                        var code;
                        var props;
                        var propsPath;
                        
                        this.stack = new Error().stack;
                        
                        args.forEach( function( arg ) {
                            // Code as function argument is not supported, use builder instead.
                            if( arg instanceof Number ) {
                                id = arg;
                            } else if ( arg instanceof String ) {
                                subtype = arg;
                            } else if( arg instanceof Function ) {
                                fn = arg;
                            }
                        } );
                        
                        this.assert = function( packet ) {
                            env.print( 'Got packet: ' + JSON.stringify( packet ) );
                            if( ++this.counter > n ) {
                                throw new AssertError( this, "Expected " + n + " invocations, got: " + this.counter );
                            }
                            if( type && packet.type !== type ) {
                                throw new AssertError( this, "Expected response type " + type + ", got: " + packet.type );
                            }
                            if( subtype && packet.subtype !== subtype ) {
                                throw new AssertError( this, "Expected response subtype " + subtype + ", got: " + packet.subtype );
                            }
                            if( id && packet.id !== id ) {
                                throw new AssertError( this, "Expected response id " + id + ", got: " + packet.id );
                            }
                            if( code && packet.code !== code ) {
                                throw new AssertError( this, "Expected response code " + code + ", got: " + packet.code );
                            }
                            if( props ) {
                                var src = packet;
                                if( propsPath ) {
                                    let separators = propsPath.split('.');
                                    while( separators.length > 0 ) {
                                        let separator = separators.splice(0,1)[0];
                                        if( src[separator] ) {
                                            src = src[separator];
                                        }
                                    }
                                }
                                let handle = (function( props, src ) {
                                    for( let prop in props ) {
                                        if( props[prop] !== src[prop] ) {
                                            throw new AssertError( this, "Expected response property " + prop + "=" + props[prop] + ", got: " + src[prop] );
                                        }
                                    }
                                }).bind(this);
                                if( Array.isArray( props ) ) {
                                    if( !Array.isArray( src ) || src.length !== props.length ) {
                                        throw new AssertError( this, "Response property is not an array or have different length." );
                                    }
                                    let i = 0;
                                    props.forEach( function( props ) {
                                        handle( props, src[i++] );
                                    });
                                } else {
                                    handle(props, src);
                                }
                            }
                            if( fn ) {
                                if( !fn( packet ) )  {
                                    throw new AssertError( this, "Provided checker action failed." );
                                }
                            }
                            return {
                                last: this.counter === n
                            };
                        };
                        
                        this.toString = function() {
                            return '[Assert(id: ' + id + ' type: ' +  type + ' subtype: ' + subtype + ' code: ' + code + ' fn: ' + (fn?'provided':'undefined') + ')]';
                        };
                        
                        // Assert builder.
                        return {
                            id: function(id_) {
                                id = id_;
                                return this;
                            },
                            subtype: function(subtype_) {
                                subtype = subtype_;
                                return this;
                            },
                            fn: function(fn_) {
                                fn = fn_;
                                return this;
                            },
                            code: function(code_) {
                                code = code_;
                                return this;
                            },
                            props: function(props_, propsPath_) {
                                props = props_;
                                propsPath = propsPath_;
                                return this;
                            }
                        };
                    },
                    
                    toString: function() {
                        return '[Assert(No assert)]';
                    }
                };
                expectations.push(expection);
                return expection;
            };
        
            dbg.checkSatisfied = function() {
            
                env.print( 'Test results for script: ' + script );
                env.print( 'Asserts satisfied: ' + assertsSatisfied );
            
                assertsSuccess += assertsSatisfied;
            
                var success = true;
            
                // Check if there are any collected failures.
                if( failures.length > 0 ) {
                    env.print('Failures:');
                    var counter = 0;
                    failures.forEach( function(exc) {
                        env.print( '' + counter++ + ':' );
                        env.print('Assert failed: ' + exc.message);
                        if( exc.expectation ) {
                            env.print('Assert details: ' + exc.expectation);
                        }
                        env.print(exc.stack);
                    });
                    success = false;
                }

                // Check if there are any unsatisfied assertions.
                if( expectations.length > 0 ) {
                    env.print( 'Not all expectations have been meet:' );
                    expectations.forEach( function(expectation) {
                        env.print('Expectation details: ' + expectation);
                        env.print(expectation.stack);
                    });
                    success = false;
                }
                
                if( !success ) {
                    throw new Error( 'Test failed for script: ' + script );
                }
            };

            // Checks asserts basing on the packet.
            let checkAsserts = function( packet ) {
                try {
                    if( expectations.length > 0 ) {
                        let result = expectations[0].check( packet ); 
                        if( result.last ) {
                            expectations.splice(0, 1);
                        }
                        assertsSatisfied++;
                        if( result.fn ) {
                            let res = result.fn();
                            if( typeof(res) === 'boolean' && !res ) {
                                assertsFailure++;
                                throw new AssertError( this, "Post handler returned false." );
                            }
                        }
                    } else {
                        throw new AssertError( null, 'Unexpected remote packet: ' + JSON.stringify( packet ) + '. No more assertions!' );
                    }
                } catch( exc ) {
                    assertsFailure++;
                    failures.push(exc);
                }
                return true;
            };
        
            dbg.onCommand = checkAsserts;
            dbg.onPause = function( suspended ) {
                return checkAsserts.call(dbg, { suspended: suspended } );
            };
            
            return fn(dbg);
        }, suspended );
    };
}

