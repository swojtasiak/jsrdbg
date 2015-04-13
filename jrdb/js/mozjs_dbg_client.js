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

(function() {

    const HELP_VERSION = ""+
        "Shows application version.";

    const HELP_DELETE = ""+
        "Arguments are breakpoint numbers with spaces in between.\n"+
        "To delete all breakpoints, give no argument.\n"+
        "\n"+
        "List of delete subcommands:\n\n"+
        "delete breakpoints -- Delete some breakpoints";

    const HELP_DELETE_BREAKPOINTS = ""+
        "Delete some breakpoints.\n"+
        "Arguments are breakpoint numbers with spaces in between.\n"+
        "To delete all breakpoints, give no argument.\n"+
        "This command may be abbreviated 'delete'.\n\n"+
        "Usage:\n"+
        "delete breakpoints 1 2 3 4 5\n"+
        "delete breakpoints";

    const HELP_SET = ""+
        "Sets environments variable.\n\n"+
        "Usage:\n"+
        "set debug=true";
        
    const HELP_ENV = ""+
        "Shows all environment variables.";
        
    const HELP_PC = ""+
        "Shows the location where debuggee is currently paused.";

    const HELP_STEP = ""+
        "Steps program until it reaches a different source line.\n"+
        "Usage:\n"+
        "step\n"+
        "s";
        
    const HELP_NEXT = ""+
        "Steps program proceeding through subroutines. It doesn't enter\n"+
        "subroutine calls but executes them instead and pauses in the line just\n"+
        "after the subroutine call.\n"+
        "Usage:\n"+
        "next\n"+
        "n";
        
    const HELP_CONTINUE = ""+
        "Continues program being debugged\n"+
        "Usage:\n"+
        "continue\n"+
        "c";

    const HELP_LIST = ""+
        "Lists number of source code lines starting from the\n"+
        "location where debuggee is paused. When run first time\n"+
        "it requests for the whole source code of the script being\n"+
        "debugged and caches it. It's why first call for a particular\n"+
        "script can be a bit slower.\n"+
        "Usage:\n"+
        "list [n]\n"+
        "l [n]\n"+
        "[n] - Number of source lines to print.";

    const HELP_HELP = ""+
        "Prints list of command and their short descriptions.\n"+
        "Usage:\n"+
        "help\n"+
        "help [instruction]\n"+
        "instruction - The instruction we are interested in.";

    const HELP_BREAK = ""+
        "Pauses the debuggee or sets a new breakpoint.\n"+
        "Usage:\n"+
        "break,b,pause - pauses the debuggee.\n"+
        "break,b,pause [script] [line] - sets new breakpoint for a script and line.\n"+
        "[script] - Name of the script.\n"+
        "[line] - Number of the line in the script.\n"+
        "For instance:\n"+
        "break /utils/encoding.js 145";
   
    const HELP_INFO = ""+
        "Prints various information about the program being debugged.\n\n"+
        "List of info subcommands:\n\n"+
        "info breakpoints - Prints set breakpoints\n"+
        "info locals - Prints local variables for a frame";
   
    const HELP_INFO_LOCALS = ""+
        "Lists local variables.\n"+
        "The process of printing can be controlled using the following environemnt variables:\n"+
        "show-hierarchy - Shows properties from prototypes\n"+
        "evaluation-depth - How deep variables have to be traversed in terms of prototypal inheritance.\n"+
        "They can be set using 'set' command. For instance:\n"+
        "set show-hierarchy=true\n"+
        "set evaluation-depth=5\n"+
        "Usage:\n"+
        "info locals\n"+
        "i l";
        
    const HELP_INFO_BREAKPOINTS = ""+
        "Prints all set breakpoints.\n"+
        "Usage:\n"+
        "info breakpoints\n"+
        "i b";
        
    const HELP_BACKTRACE = ""+
        "Prints backtrace of all stack frames.\n"+
        "Usage:\n"+
        "backtrace\n"+
        "bt";
        
    const HELP_SOURCE = ""+
        "Prints list of script files managed by the debugger or loads source code of given script\n"+
        "file if script url is provided in arguments.\n"+
        "Usage:\n"+
        "source [url]\n"+
        "[url] - Script url.";
        
    const HELP_CONTEXT = ""+
        "Every SpiderMonkey engine can run more than one JSRuntime (typically one for a\n"+
        "process) which in turn manages multiple JSContext instances. Every piece of\n"+
        "JavaScript code is run in the scope of such a JSContext. JSContext has its own\n"+
        "global object and an execution stack. From the debugger point of view\n"+
        "JSContext is treated like a thread. The more that it should be accessed by the\n"+
        "same thread during its lifetime. Every JSContext is a debuggeable unit. That\n"+
        "is, in order to debug anything you have to choose the JSContext you would like\n"+
        "to debug. After picking out the appropriate context every typed command is sent\n"+
        "to that JSContext. The list of all managed JSContext can be obtained using \"context\"\n"+
        "command. In order to choose one JSContext from the returned list you have to use the\n"+
        "same command supplying the context's numerical identifier from the first column.\n"+
        "Usage:\n"+
        "context [id] - Chooses a context.\n"+
        "context - Asks for a list of all available contexts.\n"+
        "[id] - JS Context ID.";
        
    const HELP_PRINT = ""+
        "Prints value of given expression.\n"+
        "Expressions are evaluated in the scope of a frame the debugger is paused in.\n"+
        "Usage:\n"+
        "p [exp]\n"+
        "[exp] - An expression to evaluate.\n"+
        "Examples:\n"+
        ">p 1+2\n"+
        "3\n"+
        ">p user['name']\n"+
        "John\n"+
        ">p ({name:'John',surname:'Doe'})\n"+
        "{\n"+
        "  name\": \"John\"\n"+
        "  surname\": \"Doe\"\n"+
        "}\n"+
        ">p (function(){return 1+2})()\n"+
        "3";

    const HELP = ""+
        "List of available commands.\n\n"+
        "set - Sets environment variables\n"+
        "env - Prints environment variables\n"+
        "pc - Prints information about current frame\n"+
        "step - Steps to a next instruction\n"+
        "next - Steps to a next instructions going through subroutines\n"+
        "continue - Continues execution of the program being debugged\n"+
        "break,pause - Pauses the program being debugged or sets breakpoins\n"+
        "delete - Deletes breakpoints\n"+
        "info - List information about program being debugged\n"+
        "list - Prints source code of the script being debugged\n"+
        "backtrace - Prints backtrace of stack frames\n"+
        "print - Evaluates expressions\n"+
        "version - Shows application version\n"+
        "source - Loads script source code\n"+
        "animation - Enables/disables stepping animation";
        
    const HELP_ANIMATION = ""+
        'Enables or disables stepping animation. When enables it shows a bigger\n'+
        'context of the source code.';

    const ID_CHARS = [
        'A','B','C','D','E','F',
        '1','2','3','4','5','6','7','8','9','0'
    ];

    // Map of all functions which waits for responses from the remote debugger.
    var waitingResponseHandlers = {};

    // Context variables.
    
    const CV_DEBUG = 'debug';
    const CV_CURRENT_CONTEXT_ID = 'current-context-id';
    const CV_ANIMATION = 'animation';
    const CV_SOURCE_CONTEXT = 'source-context';
    const CV_SHOW_HIERARCHY = 'show-hierarchy';
    const CV_EVALUATION_DEPTH = 'evaluation-depth';
    const CV_COLORS = 'colors';

    /*************************/
    /* Decorate environment. */
    /*************************/
    
    // Wraps sendCommand in order to add some logs.
    
    env.sendCommand = (function() {
        var sendCommandCoreFn = env.sendCommand;
        var sendCommandFn = function( command ) {
            if( context.isEnv( CV_DEBUG ) ) {
                env.println('Sending:' + JSON.stringify( command ) );
            }
            var contextId = context.getEnvInt( CV_CURRENT_CONTEXT_ID );
            if( contextId === undefined ) {
                throw new Error( 'Current JavaScript context is not set or it not an integer value. Type "help context" for more information.' );
            }
            return sendCommandCoreFn( contextId, command );
        };
        return function( command, fn ) {
            if( fn ) {
                // Random request ID.
                let now = Date.now();
                var id = "";
                for( let i = 0; i < 16; i++ ) {
                    id += ID_CHARS[Math.floor(Math.random() * (ID_CHARS.length - 1))];
                } 
                command.id = id;
                sendCommandFn( command );
                if( fn ) {
                    waitingResponseHandlers[id] = {
                        fn: fn,
                        time: now
                    };
                }
                // Clean all outdated handlers (30 sec).
                for( let key in waitingResponseHandlers ) {
                    let responseHandler = waitingResponseHandlers[key];
                    if( responseHandler.time < now - 1000 * 30 ) {
                        delete waitingResponseHandlers[key];
                    }
                }
            } else {
                sendCommandFn( command );
            }
        };
    })();
    
    env.reportRemoteError = function( error ) {
        if( error.message ) {
            env.println( 'Remote error: ' + error.message );
        } else {
            env.println( 'Unsupported remote error: ' + JSON.stringify( error ) );
        }
    };
    
    /*********************/
    /* Debugger context. */
    /*********************/

    function DebuggerContext() {
        this._location = {
            url: null,
            line: 0
        };
        this._env = {};
        this._env[ CV_SHOW_HIERARCHY ] = true;
        this._env[ CV_EVALUATION_DEPTH ] = 1;
        this._env[ CV_COLORS ] = true;
    }

    DebuggerContext.prototype = {
        setLocation: function(url, line) {
            this._location = {
                url: url,
                line: line
            };
        },
        getLocation: function() {
            return this._location; 
        },
        getEnv: function(key, def) {
            if( this._env[key] !== undefined ) {
                return this._env[key];
            }
            if(def) {
                return def;
            }
        },
        getEnvInt: function( key ) {
            var contextId = this.getEnv( key );
            if( contextId !== undefined && typeof(contextId) === 'string' ) {
                contextId = parseInt( contextId );
                if( isNaN( contextId ) ) {
                    contextId = undefined;
                }
            }
            return contextId;
        },
        setEnv: function( key, value ) {
            this._env[key] = value;
        },
        isEnv: function( key ) {
            let value = this.getEnv(key);
            if( value !== undefined ) {
                if( value instanceof String ) {
                    switch(value.trim().toLowerCase()) {
		                case "true": case "yes": case "1": 
		                    return true;
		                case "false": case "no": case "0": case null: 
		                    return false;
		                default: 
		                    return Boolean(value);
	                }
                } else {
                    return Boolean( value );
                }
            }
            return false;
        },
        forEach: function( fn ) {
            for( let p in this._env ) {
                fn( p, this._env[p] );
            }
        },
        getVariables: function() {
            var variables = {};
            this.forEach( function( property, value ) {
                variables[property] = value;
            } );
            return variables;
        }
    };

    var context = new DebuggerContext();

    /*********************/
    /* Debugger Utils. */
    /*********************/

    var Utils = {};
    
    Utils.printLocation = function() {
        var location = context.getLocation();
        env.println('Script: ' + location.url);
        env.println('Line: ' + location.line);
        if( location.source ) {
            env.println('Source: ' + location.source);
        }
    };

    Utils.printBreakpoint = function( breakpoint ) {
        env.println( 'Breakpoint ' + breakpoint.bid + ': script ' + breakpoint.url + ', ' + breakpoint.line + '.' );
    };
    
    Utils.printBacktraceElement = function( i, st ) {
        env.println( '#' + i + ' ' + st.url + ':' + st.line + ( st.depth ? (' (' + st.depth + ')') : '' ) );
    };

    Utils.printVariable = function( variable ) {
        env.println( JSON.stringify( variable, null, 2 ) );
    };
    
    /***************************/
    /* Source code repository. */
    /***************************/

    function SourceRepository() {
        this._scripts = {};
    }

    SourceRepository.prototype = {
        getSourceCode: function( url, fn ) {
            if( this._scripts[url] ) {
                let src = this._scripts[url];
                fn( src.source, src.displacement );
            } else {
                env.sendCommand( protocolStrategy.getSourceScript(url), ( function(packet) {
                    if( packet.source ) {
                        this._scripts[url] = { 
                            source: packet.source,
                            displacement: packet.displacement
                        };
                        fn( packet.source, packet.displacement );
                    } else {
                        env.println('There is no script source in the response.');
                    }
                } ).bind(this));
            }
        },
        printSource: function( location, lines, marker, ctx ) {
        
            this.getSourceCode( location.url, function( script, displacement ) {
                
                /* Show file from the beginning. */
                let line = 0;
                
                if( location.line !== undefined ) {
                    // Convert to native line number.
                    line = location.line + displacement;
                }
                
                if( !lines ) {
                    // Show the whole source script.
                    lines = script.length;
                } else if( ctx ) {
                    // Add space for the context.
                    lines += ctx;
                } 
                
                let start = line;
                
                // Make space for the code context.
                if( ctx ) {
                    if( lines > script.length ) {
                        lines = script.length;
                    }
                    start = line - ctx;
                    if( start < 0 ) {
                        start = 0;
                    }
                }

                let len = ( ( location.line + lines ).toString() ).length;
                
                for( let i = start; i < start + lines && i < script.length; i++ ) {
                    let nr = '';
                    let lineNrStr = ( i - displacement ).toString();
                    let nrLen = lineNrStr.length;
                    for( let j = 0; j < len - nrLen; j++ ) {
                        nr += ' ';
                    }
                    if( context.isEnv( CV_COLORS ) ) {
                        nr += ( '\033[1;31m' + lineNrStr + '\033[0;37m' );
                    } else {
                        nr += lineNrStr;
                    }
                    let def = true;
                    if( marker ) {
                        if( i == line ) {
                            if( context.isEnv( CV_COLORS ) ) {
                                nr += '\033[1;33m> \033[0;37m';
                            } else {
                                nr += '* ';
                            }
                            if( context.isEnv( CV_COLORS ) ) {
                                env.println( nr + '\033[1;40m' + script[i] + '\033[0m' );
                                def = false;
                            }
                        } else {
                            nr += '  ';
                        }
                    } else {
                        nr += ' ';
                    }
                    if( def ) {
                        env.println( nr + script[i] );
                    }
                }
                
            });
        }
    };

    var sourceRepository = new SourceRepository();

    /*******************/
    /* Commands parser */
    /*******************/

    function CommandParser() {
    }

    CommandParser.prototype = {
        parse: function( str ) {
            var command = {
                raw: str,
                options: {},
                args: [],
                /* Checks if there is given option available in the command. */
                isOption: function() {
                    for( let index in arguments ) {
                        if( this.options[arguments[index]] ) {
                            return true;
                        }
                    }
                    return false;
                }
            };
            var cmdArgs = str.split(/\s+/);
            for( let i = 0; i < cmdArgs.length; i++ ) {
                var arg = cmdArgs[i];
                if( arg.indexOf( '--' ) === 0 ) {
                    let key = arg.slice(2);
                    let value = true;
                    let valueIndex = key.indexOf( '=' );
                    if ( valueIndex > 0 ) {
                        value = key.slice(valueIndex + 1);
                        key = key.slice(0, valueIndex);
                    }
                    command.options[key] = value;
                } else if( arg.trim().length > 0 ) {
                    command.args.push( arg.trim() );
                }
            }
            return command;
        }
    };
    
    /******************************************************************/
    /* Default handlers for debugger asynchronous responses/messages. */
    /******************************************************************/

    function DebuggerEventHandlerFactory() {
    }
    
    DebuggerEventHandlerFactory.prototype = {
    
        create: function( type, subtype ) {
            if( subtype && this[type][subtype] ) {
                return this[type][subtype];
            } else if( this[type] ) {
                return this[type];
            }
            return null;
        },
        
        info: {
        
            /* Prints list of available contexts. */
            contexts_list: function( packet ) {
                var contexts = "Available JSContext instances: \n";
                if( packet.contexts ) {
                    var contextId = context.getEnvInt( CV_CURRENT_CONTEXT_ID );
                    var foundId;
                    packet.contexts.forEach( function( ctx ) {
                        contexts += ( ctx.contextId + ') ' + ctx.contextName + ( ctx.paused ? ' (PAUSED)' : ' (RUNNING)' ) );
                        if( contextId === ctx.contextId ) {
                            foundId = contextId;
                        }
                    } );
                    if( foundId === undefined ) {
                        switch( packet.contexts.length ) {
                        case 0:
                            contexts += '\nThere are no JSContexts registered inside the connected '+
                                        'remote debugger, so there is nothing to debug.';
                            break;
                        case 1:
                            contexts += '\nThere is only one JSContext managed by the debugger, so '+
                                        'it has been chosen as the current one.\n'+
                                        'You can change it using the "context" command.';
                            context.setEnv( CV_CURRENT_CONTEXT_ID, packet.contexts[0].contextId );
                            break;
                        default:
                            contexts += '\nThere is more than one JSContext managed by the remote debugger '+
                                        'the first one has been chosen as the current one.\nYou can change it '+
                                        'using the "context" command.';
                            context.setEnv( CV_CURRENT_CONTEXT_ID, packet.contexts[0].contextId );
                            break;
                        }
                    }
                }
                env.println( contexts );
            },
            
            /* Update PC. */
            pc: function( packet ) {
                // Update current location.
                context.setLocation(packet.script, packet.line);
            },
            
            /* Debuggee has been paused. */
            paused: function( packet ) {

                let location = context.getLocation();

                if( packet.url !== location.url ) {
                    if( packet.url ) {
                        env.println(packet.url);
                    } else {
                        env.println('No source script.');
                    }
                }
                
                // Update current location.
                context.setLocation(packet.url, packet.line);
                
                if( context.isEnv( CV_ANIMATION ) ) {
                    
                    let ctxLen = context.getEnvInt( CV_SOURCE_CONTEXT );
                    if( !ctxLen ) {
                        ctxLen = 10;
                    }
                    
                    sourceRepository.printSource( context.getLocation(), ctxLen, true, ctxLen );
                    
                } else { 
                    env.println( packet.line + ' ' + ( packet.source ? packet.source : '' ) );
                }
                
            }
            
        },
        
        error: function( packet ) {
            if( packet.message ) {
                env.println( 'Remote error: ' + packet.message );
            } else {
                env.println( 'Unsupported remote error: ' + JSON.stringify( packet ) );
            }
        }
    };

    /**********************************/
    /* Control commands from the GUI. */
    /**********************************/
    
    function CtrlCommandFactory() {
        this._commandHandler = this._handleSubCommand(this._commandsDef);
    }
    
    CtrlCommandFactory.prototype = {
        create: function( command ) {
            return this._commandHandler(command);
        },
        // Gets help description for a command.
        help: function( command ) {
            var args = command.args.slice();
            var current = this._commandsDef;
            while( current && args.length > 0 ) {
                let arg = args.splice(0,1)[0];
                for( let i in current ) {
                    let def = current[i];
                    if( def.alias ) {
                        if( def.alias.indexOf( arg ) !== -1 ) {
                            if( args.length === 0 ) {
                                return def.help;
                            } else {
                                current = def.sub;
                                break;
                            }
                        }
                    }
                }
            }
        },
        // Prepares call tree for multilevel commands.
        _handleSubCommand: function( defs ) {
            var commands = {};
            for( let index in defs ) {
                let def = defs[index];
                if( def ) {
                    var commandHandler = {};
                    if( def.fn ) {
                        commandHandler.command = def.fn;
                    }
                    if( def.sub ) {
                        commandHandler.next = this._handleSubCommand(def.sub);
                    }
                    if( def.default_sub ) {
                        commandHandler.default_sub = def.default_sub;
                    }
                    for( let aliasIndex in def.alias ) {
                        let alias = def.alias[aliasIndex];
                        commands[alias] = commandHandler;
                    }
                }
            }
            return function(command) {
                if( command.args.length > 0 ) {
                    var commandName = command.args[0];
                    command.args = command.args.slice(1);
                    if( commands[commandName] ) {
                        let result = null;
                        var cmdDef = commands[commandName];
                        if( cmdDef.next ) {
                            if( cmdDef.default_sub ) {
                                let orgArgs = command.args.slice(0);
                                result = cmdDef.next(command);
                                if( result === null ) {
                                    orgArgs.splice(0, 0, cmdDef.default_sub);
                                    command.args = orgArgs;
                                    result = cmdDef.next(command);
                                }
                            } else {
                                result = cmdDef.next(command);
                            }
                        }
                        if( result === null && cmdDef.command ) {
                            return cmdDef.command;
                        }
                        return result;
                    }
                }
                return null;
            };
        },
        _commandsDef: [
            {
                alias: ['version'],
                help: HELP_VERSION,
                fn: function( command ) {
                    env.println('Client version: ' + env.packageVersion  + " Built for SpiderMonkey: " + env.engineMajorVersion + "." + env.engineMinorVersion);
                }
            },
            { 
                alias: ['set'],
                help: HELP_SET,
                fn: function( command ) {
                    // Treat options as variables.
                    for( let key in command.options ) {
                        let option = command.options[key];
                        context.setEnv( key, option );
                    }
                    // Parse arguments.
                    for( let index in command.args ) {
                        var arg = command.args[index];
                        let ai = arg.indexOf('=');
                        if( ai > 0 ) {
                            let vn = arg.slice(0, ai);
                            let vv = arg.slice(ai + 1);
                            context.setEnv(vn, vv);
                        }
                    } 
                }
            },
            { 
                alias: ['env'],
                help: HELP_ENV,
                fn: function( command ) {
                    context.forEach( function( name, value ) {
                        env.println(name + '=' + value);
                    });
                }
            },
            { 
                alias: ['pc'],
                help: HELP_PC,
                fn: function( command ) {
                    env.sendCommand( protocolStrategy.getPC( command.isOption( 'source', 's' ) ), function( packet ) {
                        context.setLocation(packet.script, packet.line);
                        Utils.printLocation();
                    } );
                }
            },
            {
                alias: ['context'],
                help: HELP_CONTEXT,
                fn: function( command ) {
                    var ctxID;
                    if( command.args.length > 0 ) {
                        ctxID = parseInt(command.args[0]);
                        if( isNaN( ctxID ) ) {
                            env.println('Error: Context ID is not an integer value.');
                            return;
                        }
                        context.setEnv(CV_CURRENT_CONTEXT_ID,ctxID);
                    } else {
                        env.sendCommand( protocolStrategy.getContexts() );
                    }
                }
            },
            {
                alias: ['step','s'],
                help: HELP_STEP,
                fn: function( command ) {
                    env.sendCommand( protocolStrategy.getStep() );
                }
            },
            {
                alias: ['next','n'],
                help: HELP_NEXT,
                fn: function( command ) {
                    env.sendCommand( protocolStrategy.getNext() );
                }
            },
            {
                alias: ['continue','c'],
                help: HELP_CONTINUE,
                fn: function( command ) {
                    env.sendCommand( protocolStrategy.getContinue() );
                }
            },
            {
                alias: ['list','l'],
                help: HELP_LIST,
                fn: function( command ) {
                    var lines = 10;
                    if( command.args.length > 0 ) {
                        lines = parseInt(command.args[0]);
                        if( isNaN( lines ) ) {
                            env.println('Error: Number of lines is not an integer value.');
                            return;
                        }
                    }
                    let location = context.getLocation();
                    if( !location.url ) {
                        env.sendCommand( protocolStrategy.getPC(false), function(packet) {
                            if( packet.script && typeof( packet.line ) === 'number' ) {
                                context.setLocation(packet.script, packet.line);
                                sourceRepository.printSource( context.getLocation(), lines, true );
                            } else {
                                env.println('Bad response.');
                            }
                        } );
                    } else {
                        sourceRepository.printSource( location, lines, true );
                    }
                }
            },
            {
                alias: ['delete','d'],
                default_sub: 'breakpoints',
                help: HELP_DELETE,
                sub: [
                    {
                        alias: ['breakpoints','b'],
                        help: HELP_DELETE_BREAKPOINTS,
                        fn: function( command ) {
                            if( command.args.length === 0 ) {
                                env.sendCommand( protocolStrategy.getDeleteAllBreakpoints(), function() {
                                    env.println('All breakpoints have been deleted.');
                                } );
                            } else {
                                var ids = [];
                                for( let index in command.args ) {
                                    let id = parseInt(command.args[index]);
                                    if( isNaN( id ) ) {
                                        env.println( 'Breakpoint identificator has to be an integer value: ' + command.args[index] );
                                        return;
                                    }
                                    ids.push(id);
                                }
                                env.sendCommand( protocolStrategy.getDeleteBreakpoints( ids ), function( packet ) {
                                    env.println('The following breakpoints have been deleted: ' + JSON.stringify( packet.ids ) );
                                } );
                            }
                        }
                    }
                ]
            },
            {
                alias: ['help','h'],
                help: HELP_HELP,
                fn: function( command ) {
                    var help = ctrlCommandFactory.help( command );
                    if( help ) {
                        env.println( help );
                    } else {
                        env.println( HELP );
                    }
                }
            },
            {
                alias: ['break','pause','b'],
                help: HELP_BREAK,
                fn: function( command ) {
                    if( command.args.length === 0 ) {
                        env.sendCommand( protocolStrategy.getPause(), function( packet ) {
                            if( packet.script && typeof( packet.line ) === 'number' ) {
                                context.setLocation(packet.script, packet.line);
                                Utils.printLocation();
                            }
                        } );
                    } else if ( command.args.length === 2 ) {
                        let src = command.args[0];
                        let line = parseInt(command.args[1]);
                        if( isNaN(line) ) {
                            env.println('Error: Number of line has to be an integer.');
                            return;
                        }
                        env.sendCommand( protocolStrategy.getSetBreakpoint( src, line ), function( packet ) {
                            env.println( 'Breakpoint ' + packet.bid + ': script ' + packet.url + ', ' + packet.line + '.' );
                        } );
                    }
                }
            },
            {
                alias: ['info','i'],
                help: HELP_INFO,
                sub: [
                    {
                        alias: ['breakpoints','b'],
                        help: HELP_INFO_BREAKPOINTS,
                        fn: function( command ) {
                            env.sendCommand( protocolStrategy.getGetBreakpoints(), function(packet) {
                                if( packet.breakpoints && packet.breakpoints.length > 0 ) {
                                    for( let index in packet.breakpoints ) {
                                        Utils.printBreakpoint( packet.breakpoints[index] );
                                    }
                                } else {
                                    env.println('No breakpoints found.');
                                }
                            } );
                        }
                    },
                    {
                        alias: ['locals','l'],
                        help: HELP_INFO_LOCALS,
                        fn: function( command ) {
                            let depth;
                            if( !command.isOption('all', 'a') ) {
                                if( command.args.length > 0 ) {
                                    let depth = parseInt(command.args[0]);
                                    if( isNaN( depth ) ) {
                                        env.println('Depths has to be an integer value.');
                                        return;
                                    }
                                } else {
                                    depth = 0;
                                }
                            }
                            let options = {
                                'show-hierarchy':   context.isEnv( CV_SHOW_HIERARCHY ),
                                'evaluation-depth': context.getEnv( CV_EVALUATION_DEPTH )
                            };
                            env.sendCommand( protocolStrategy.getGetVariables( depth, options ), function(packet) {
                                if( packet.variables && packet.variables.length > 0 ) {
                                    let frames = packet.variables;
                                    for( let frameIndex in frames ) {
                                        let frame = frames[frameIndex];
                                        if( frame.stackElement ) {
                                            Utils.printBacktraceElement( frame.stackElement.rDepth, frame.stackElement );
                                        }
                                        if( frame.variables ) {
                                            env.println( 'Variables:' );
                                            for( let i in frame.variables ) {
                                                var variable = frame.variables[i];
                                                env.print( variable.name + ' = ' );
                                                Utils.printVariable( variable.value );
                                            }
                                        }
                                    }
                                } else {
                                    env.println('No variables found.');
                                }
                            } );
                        }
                    }
                ]
            },
            {
                alias: ['backtrace','bt'],
                help: HELP_BACKTRACE,
                fn: function( command ) {
                    env.sendCommand( protocolStrategy.getStacktrace(), function( packet ) {
                        if( packet.stacktrace && packet.stacktrace.length > 0 ) {
                            for( let i in packet.stacktrace ) {
                                let st = packet.stacktrace[i];
                                Utils.printBacktraceElement( i, st );
                            }
                        } else{
                            env.println('No stack found.');
                        }
                    } );
                }
            },
            {
                alias: ['print','p'],
                help: HELP_PRINT,
                fn: function( command ) {
                    // I known, I known...I'll rewrite it when I've a time.
                    var commandLine = command.raw;
                    if( commandLine.indexOf('print') === 0 ) {
                        commandLine = commandLine.substring(6);
                    } else if( commandLine.indexOf('p') === 0 ) {
                        commandLine = commandLine.substring(2);
                    }
                    let options = {
                        'show-hierarchy':   context.isEnv( CV_SHOW_HIERARCHY ),
                        'evaluation-depth': context.getEnv( CV_EVALUATION_DEPTH )
                    };
                    env.sendCommand( protocolStrategy.getEvaluate( commandLine, options ), function( packet ) {
                        if( packet.result !== undefined) {
                            Utils.printVariable( packet.result );
                        } else {
                            env.println('undefined');
                        }
                    } );
                }
            },
            {
                alias: ['source'],
                help: HELP_SOURCE,
                fn: function( command ) {
                    if( command.args.length === 0 ) {
                        env.sendCommand( protocolStrategy.getAllSourceUrls(), function( packet ) {
                            if( packet.urls ) {
                                packet.urls.forEach( function( url ) {
                                    env.println( url );
                                } );
                            }
                        } );
                    } else {
                        sourceRepository.printSource( { url: command.args[0] } );
                    }
                }
            },
            {
                alias: ['animation', 'anim'],
                help: HELP_ANIMATION,
                fn: function( command ) {
                    context.setEnv( CV_ANIMATION, !context.isEnv( CV_ANIMATION ) );
                    env.println( 'Animation: ' + ( context.isEnv( CV_ANIMATION ) ? 'ON' : 'OFF' ) );
                }
            }
        ]
    };

    /********************************/
    /* Commands send to the server. */
    /********************************/

    function ProtocolStrategy() {
    }
    
    ProtocolStrategy.prototype = {
        // Gets current PC from the server.
        getPC: function( source ) {
            return {
                type: 'command',
                name: 'pc',
                source: source
            };
        },
        // Steps to the next instruction or frame.
        getStep: function() {
            return {
                type: 'command',
                name: 'step'
            };
        },
        // Gets all contexts managed by the remote debuggger.
        getContexts: function() {
            return 'get_available_contexts';
        },
        // Gets to the next instruction, but doesn't enter the new frames.
        getNext: function() {
            return {
                type: 'command',
                name: 'next'
            };
        },
        // Steps to the next instruction or frame.
        getSourceScript: function( url ) {
            return {
                type: 'command',
                name: 'get_source',
                url: url
            };
        },
        // Continues execution to the next breakpoint od 'debugger;' statement.
        getContinue: function() {
            return {
                type: 'command',
                name: 'continue'
            };
        },
        // Deletes all breakpoints.
        getDeleteAllBreakpoints: function() {
            return {
                type: 'command',
                name: 'delete_all_breakpoints'
            };
        },
        // Breaks application at the next instruction.
        getPause: function() {
            return {
                type: 'command',
                name: 'pause'
            };
        },
        // Delete breakpoints.
        getDeleteBreakpoints: function(ids) {
            return {
                type: 'command',
                name: 'delete_breakpoint',
                ids: ids
            };
        },
        // Sets a new breakpoint.
        getSetBreakpoint: function( src, line ) {
            return {
                type: 'command',
                name: 'set_breakpoint',
                breakpoint: {
                    url: src,
                    line: line,
                    pending: true
                }
            };
        },
        // Gets all breakpoints.
        getGetBreakpoints: function() {
            return {
                type: 'command',
                name: 'get_breakpoints'
            };
        },
        // Gets stacktrace.
        getStacktrace: function() {
            return {
                type: 'command',
                name: 'get_stacktrace'
            };
        },
        // Gets variables for given stack level.
        getGetVariables: function(depth, options) {
            return {
                type: 'command',
                name: 'get_variables',
                query: {
                    depth: depth,
                    options: options
                }
            };
        },
        // Evaluates variable.
        getEvaluate: function( path, options ) {
            return {
                type: 'command',
                name: 'evaluate',
                path: path,
                options: options
            };
        },
        // Evaluates variable.
        getAllSourceUrls: function() {
            return {
                type: 'command',
                name: 'get_all_source_urls'
            };
        }
    };

    var debuggerEventHandlerFactory = new DebuggerEventHandlerFactory();
    var protocolStrategy = new ProtocolStrategy();
    var ctrlCommandFactory = new CtrlCommandFactory();
    var parser = new CommandParser();

    return {
	    handleDbgCommand: function( contextId, command ) {
	        
	        try {
	        
	            command.contextId = contextId;
	            
	            if( context.isEnv( CV_DEBUG ) ) {
                    env.println('Received:' + JSON.stringify( command ) + ' from context: ' + contextId );
                }
                
                // Check if there is a method waiting for this response.
                if( command.id ) {
                    let responseHandler = waitingResponseHandlers[command.id];
                    if( responseHandler ) {
                        let error = command.type === 'error';
                        let fn = responseHandler.fn;
                        let handled = false;
                        // If there is only one function defined it's destined to handle succesful requests only.
                        if( !error && fn instanceof Function ) {
                            fn( command );
                            handled = true;
                        } else {
                            if( error && fn.error ) {
                                fn.error(command);
                                handled = true;
                            } else if( !error && fn.ok ) {
                                fn.ok(command);
                                handled = true;
                            }
                        }
                        delete waitingResponseHandlers[command.id];
                        if(handled) {
                            return;
                        }
                    } else if( command.type !== 'error' ) {
                        env.println( 'Received a late response from the remote debugger, just ignoring: ' 
                            + ( command.subtype ? command.subtype : 'unknown' ) );
                    }
                }
                
                if( command.type ) {
	                var subtype = null;
	                if( command.subtype ) {
	                    subtype = command.subtype;
	                }
                    var handler = debuggerEventHandlerFactory.create(command.type, subtype);
                    if( typeof( handler ) === 'function' ) {
                        handler( command );
                    } else {
                        env.println( "Unknown command: " + JSON.stringify( command ) );
                    }
                }
                
            
            } catch( exc ) {
                env.println( 'Unhandled exception: ' + exc.message + ' Stack: ' + exc.stack );
            }
            
	    },
        handleCtrlCommand: function( command ) {
            try {
                if( command.trim().length === 0 ) {
                    return;
                }
                var cmd = parser.parse(command);
                var ctrlCmd = ctrlCommandFactory.create( cmd );
                if( ctrlCmd ) {
                    ctrlCmd(cmd);
                } else {
                    env.println('Unknown command.');
                }
            } catch( exc ) {
                var msg = exc.message;
                if( !msg ) {
                    msg = "" + exc;
                }
                env.println("Exception: " + msg);
            }
        }
    };
})();

