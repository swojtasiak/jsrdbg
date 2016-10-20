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

#include "js_remote_dbg.hpp"

#include <iostream>

#include <jsdbg_common.h>
#include <jsapi.h>
#include <jsdbgapi.h>
#include <js_utils.hpp>
#include <encoding.hpp>

#include "message_builder.hpp"

using namespace JSR;
using namespace JS;
using namespace std;
using namespace Utils;

#define ENGINE_DATA(x) static_cast<DbgContextData*>( x->getTag() )

namespace MozJS {

    /**
     * Private data for JSContext.
     */
    typedef struct DbgContextData {
        // Used to enable asynchronous callback temporarily.
        bool callbackDisabled;
        // The next callback in the chain if there was any when debugger was registered.
        JSOperationCallback callbackChain;
        // Debugger instance.
        SpiderMonkeyDebugger *debugger;
        // Action queue dedicated for the engine instance.
        action_queue actionQueue;
        // JS context id.
        int contextId;
        // True if engine is paused.
        bool paused;
    } DbgContextData;

    /**
     * Automatically disables the debugger callback for given scope.
     */
    class AutoCallbackDisable {
    public:
        AutoCallbackDisable( DbgContextData &data )
            : _data( data ) {
            data.callbackDisabled = true;
        }
        ~AutoCallbackDisable() {
            _data.callbackDisabled = false;
        }
    private:
        DbgContextData &_data;
    };

    /**
     * Main loop responsible for commands handling.JSUtils
     */
    bool JSR_CommandLoop( JSContext *cx, bool block, bool suspended ) {

        bool result = true;

        Logger &log = LoggerFactory::getLogger();

        // Notice that we do not lock engine here, it's because this code always
        // runs on JS engine's thread so it's fairly unlikely to access an engine
        // instance which has been just deleted, because it's not possible to
        // uninstall and delete the engine from a different thread.

        JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext( cx );
        if( !engine ) {
            // Engine not installed for given context.
            log.error( "JSR_CommandLoop: Engine not installed." );
            return false;
        }

        DbgContextData *ctxData = ENGINE_DATA( engine );

        ClientManager &clientManager = ctxData->debugger->getClientManager();

        // Check if debugger is still installed.
        if( ctxData ) {

            bool doNotPause = clientManager.getClientsCount() == 0 && engine->getEngineOptions().isContinueWhenNoConnections();

            if( suspended || !doNotPause ) {

                AutoCallbackDisable disableCallback( *ctxData );

                // We are operating inside debugger's compartment.
                JSAutoCompartment compartment( cx, engine->getDebuggerGlobal() );

                SpiderMonkeyDebugger *debugger = ctxData->debugger;

                try {

                    action_queue &queue = ctxData->actionQueue;

                    // Wait for incoming actions, but only if function was run in non blocking mode.
                    DebuggerAction *action;

                    debugger->setContextPaused( cx, block );

                    while ( block ? ( ( action = queue.pop() ) != NULL ) : queue.get( action ) ) {

                        ActionResult result = action->execute(cx, *debugger);

                        delete action;

                        switch ( result.result ) {
                            case ActionResult::DA_OK:
                                switch(result.hint) {
                                case ES_CONTINUE:
                                    block = false;
                                    break;
                                case ES_INTERRUPTED:
                                    throw new InterruptionException();
                                    break;
                                default:
                                    // Just ignore the block status.
                                    break;
                                }
                                break;
                            case ActionResult::DA_FAILED: {

                                // Inform all clients about the problem.
                                Command command( Command::BROADCAST, ctxData->contextId,
                                        MessageFactory::getInstance()->prepareErrorMessage(
                                                MessageFactory::CE_COMMAND_FAILED, "Cannot execute debugger command." ) );

                                clientManager.broadcast( command );

                                log.error("Debugger action failed, deleting the command.");

                            }
                            break;
                        }
                    }


                } catch( InterruptionException & ) {
                    // Loop has been interrupted.
                    result = false;
                }

                debugger->setContextPaused( cx, false );

            }

        } else {
            // Debugger has been uninstalled, so leave it as soon as possible.
            result = false;
        }

        return result;
    }

    /**
     * Responsible for sending pending asynchronous commands into the engine.
     */
    JSBool JSOperationCallback_AsyncCommand( JSContext *cx ) {

        Logger &log = LoggerFactory::getLogger();

        JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext( cx );
        if( !engine ) {
            // Engine not installed for given context.
            log.error( "JSOperationCallback_AsyncCommand: Engine not installed." );
            return false;
        }

        DbgContextData *ctxData = ENGINE_DATA( engine );
        if( ctxData && !ctxData->callbackDisabled ) {

            // Debugger compartment scope.
            JSR_CommandLoop( cx, false, false );

            // Chain.
            if( ctxData->callbackChain ) {
                ctxData->callbackChain( cx );
            }

        }

        return JS_TRUE;
    }

}

using namespace MozJS;

/******************/
/* DebuggerAction */
/******************/

DebuggerAction::DebuggerAction()
    : _log( LoggerFactory::getLogger() ) {
}

DebuggerAction::~DebuggerAction() {
}

/*****************/
/* CommandAction */
/*****************/

CommandAction::CommandAction( ClientManager &clientManager, Command &command )
    : _clientManager(clientManager),
      _command( command ) {
}

CommandAction::~CommandAction() {
}

ActionResult CommandAction::execute( JSContext *ctx, Debugger &dbg ) {

    // Just in case, but it do not have to be rooted.
    ActionResult actionResult = { ActionResult::DA_OK, ES_IGNORE };

    // Handle command.
    DebuggerStateHint state;

    // it's always called on the JS engine thread, so there is no
    // need to synchronize it.
    JSDebuggerEngine *engine = dbg.getEngine( ctx );
    if( engine ) {
        if( engine->sendCommand( _command.getClientId(), _command.getValue(), state ) ) {
            actionResult.hint = state;
        } else {
            actionResult.result = ActionResult::DA_FAILED;
        }
    } else {
        _log.error("CommandAction::execute:: Engine not found.");
        actionResult.result = ActionResult::DA_FAILED;
    }

    return actionResult;

}

/*****************/
/* ContinueAction */
/*****************/

ContinueAction::ContinueAction() {
}

ContinueAction::~ContinueAction() {
}

ActionResult ContinueAction::execute( JSContext *ctx, Debugger &debugger ) {
    ActionResult result = { ActionResult::DA_OK, ES_CONTINUE };
    return result;
}

/*************************/
/* SpiderMonkeyDebugger. */
/*************************/

SpiderMonkeyDebugger::SpiderMonkeyDebugger( ClientManager &manager, const JSRemoteDebuggerCfg &cfg )
    : _clientManager(manager),
      _log( LoggerFactory::getLogger() ),
      _cfg( cfg ) {
    // Register debugger as manager's event handler to get information about
    // every client's life cycle.
    manager.addEventHandler(this);
}

SpiderMonkeyDebugger::~SpiderMonkeyDebugger() {
}

/**
 * These signals are emitted by all input queues of managed
 * clients. So every new command received by any of the clients
 * have to be signaled inside this method. Having in regard for
 * all this, we can handle every incoming command just in here
 * in a nonblocking way.
 */
void SpiderMonkeyDebugger::handle( command_queue &queue, int signal ) {

    Command command;

    while( queue.get( command ) ) {

        int clientId = command.getClientId();

        // Sends command to a debugger.
        const string &value = command.getValue();

        if( value == "exit" ) {

            // Just disconnect the client it's the protocol that is
            // responsible for invoking the disposing procedure. Remember
            // exit command cannot be broadcasted.
            if( clientId != Command::BROADCAST ) {
                ClientPtrHolder<Client> client( _clientManager, clientId );
                if( client ) {
                    client->disconnect();
                }
            }

        } else if( value == "get_available_contexts" ) {

            // Gets list of all available contexts.
            sendContextsList( clientId );

        } else {

            // It sends the command to the debugger through the
            // dedicated blocking queue. Notice that it's not a
            // blocking queue.
            int contextId = command.getContextId();

            map_context_iterator it = _contextMap.end();

            if( contextId != -1 ) {

                it = _contextMap.find( contextId );

                if( it != _contextMap.end() ) {

                    // Context ID found, so send command to specific JSContext.
                    // Protect the engine from being deleted.
                    MutexLock locker(_lock);

                    // Direct the command to appropriate engine.
                    JSContext *ctx = it->second.context;
                    JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext( ctx );

                    if( engine ) {
                        // Sends command directly to the queue dedicated for given engine.
                        sendCommandToQueue( ctx, ENGINE_DATA( engine )->actionQueue, command );
                    } else {
                        _log.error( "Engine not found for context: %d", it->first );
                    }

                } else {

                    // No JSContext found for given ID.
                    sendErrorMessage( clientId, MessageFactory::CE_UNKNOWN_CONTEXT_ID, "Unknown JS Context." );
                    sendContextsList( clientId );

                }

            } else {

                // Broadcast the command by sending it to all the registered queues.
                // This operation is non blocking one, so it's not a problem that it's
                // covered by the lock.
                MutexLock locker(_lock);

                for( map_context_iterator it = _contextMap.begin(); it != _contextMap.end(); it++ ) {
                    JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext(it->second.context);
                    if( engine ) {
                        sendCommandToQueue( engine->getJSContext(), ENGINE_DATA( engine )->actionQueue, command );
                    }
                }

            }
        }
    }
}

/**
 * Sends error message to a client.
 */
void SpiderMonkeyDebugger::sendErrorMessage( int clientId, JSR::MessageFactory::ErrorCode errorCode, const std::string &msg ) {

    // No context found, so inform the client.
    Command command( clientId, -1, MessageFactory::getInstance()->prepareErrorMessage( errorCode, msg ) );

    if( !_clientManager.sendCommand( command ) ) {
        _log.error( "SpiderMonkeyDebugger::sendErrorMessage: Cannot send command to client: %d", command.getClientId() );
    }

}

/**
 * Sends contexts list to the client.
 */
void SpiderMonkeyDebugger::sendContextsList( int clientId ) {

    MutexLock locker(_lock);

    std::vector<JSContextState> contexts;

    for( map_context_iterator it = _contextMap.begin(); it != _contextMap.end(); it++ ) {
        JSContextDescriptor &desc = it->second;
        JSContextState state;
        state.contextId = desc.contextId;
        state.contextName = desc.contextName;
        state.paused = isContextPaused( desc.context );
        contexts.push_back( state );
    }

    Command command( clientId, -1, MessageFactory::getInstance()->prepareContextList( contexts ) );

    _clientManager.sendCommand( command );
}

/**
 * Sends a command to a queue. The command is send in a form of a debugger action.
 */
void SpiderMonkeyDebugger::sendCommandToQueue( JSContext *ctx, action_queue &queue, Command &command ) {

    DebuggerAction *commandAction = new CommandAction( _clientManager, command );

    // Warn the client.
    if( queue.getCount() >= 2 ) {

        // Inform client about the problem.
        Command warning( command.getClientId(), command.getContextId(),
                MessageFactory::getInstance()->prepareWarningMessage( MessageFactory::CW_ENGINE_PAUSED,
                        "There are pending commands in the internal debugger's queue.\\n"
                        "It seems that JavaScript engine is blocked and cannot handle commands on the fly.\\n"
                        "If the application being debugged is blocked on a system call or something,\\n"
                        "try to resume it for a while in order to execute a piece of JavaScript code." ) );

        _clientManager.sendCommand( warning );

    }

    if( queue.add( commandAction ) ) {
        // Inform runtime about new debugger commands. The runtime
        // will handle it as soon as possible, but we cannot
        // guarantee when. It's a bit similar to JS setTimeout(fn,0).
        JS_TriggerOperationCallback( JS_GetRuntime( ctx ) );
    } else {
        delete commandAction;
        _log.error( "Queue is full, so the incoming command has been ignored in order not to block the main loop." );
    }

}

/**
 * Handles events from client's manager and registers new clients
 * inside a debugger instance. This method is called only be the protocol
 * instance and by design is prepared to be called by one thread at a time, but
 * remember that it's not the JS engine thread.
 */
void SpiderMonkeyDebugger::handle( Event &event ) {

    ClientEvent &clientEvent = static_cast<ClientEvent&>( event );

    int clientId = clientEvent.getID();

    switch( event.getCode() ) {
    case ClientManager::EVENT_CODE_CLIENT_ADDED:

        _log.debug( "New client connected: %d.", clientId );

        // Send a list of available JS contexts to the client.
        sendContextsList( clientId );

        break;

    case ClientManager::EVENT_CODE_CLIENT_REMOVED:

        _log.debug( "Client disconnected: %d.", clientId );

        if( _clientManager.getClientsCount() == 0 ) {

            _log.debug( "All clients disconnected." );

            // It's enough to lock the engine using this internal lock, because
            // it's the remote debugger which is responsible for installing and
            // deleting the engine instances. So there is no risk that we access
            // free'd engine instance, because we have to go through the same lock
            // in order to delete it.
            MutexLock locker(_lock);

            for( map_context_iterator it = _contextMap.begin(); it != _contextMap.end(); it++ ) {

                JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext(it->second.context);

                if( engine ) {

                    const JSDbgEngineOptions &options = engine->getEngineOptions();

                    if( options.isContinueWhenNoConnections() ) {

                        DebuggerAction *commandAction = new ContinueAction();

                        // Send action to dedicated engine's queue.
                        if( !ENGINE_DATA( engine )->actionQueue.add( commandAction ) ) {
                            delete commandAction;
                            _log.error( "Queue is full, so the incoming command has been ignored in order not to block the main loop." );
                        }

                    }

                } else {
                    _log.error( "There is no engine registered for known JSContext." );
                }
            }

        }

        break;
    }

}

/**
 * Installs debugger inside the SpiderMonkey engine. Debugger is installed as
 * different compartment with its own global object prepared to be used by the
 * debugger instance which is written entirely in Java Script.
 */
int SpiderMonkeyDebugger::install( JSContext *cx, const string &contextName, const JSDbgEngineOptions &options ) {

    int error = JSR_ERROR_NO_ERROR;

    MutexLock locker(_lock);

    if( JSDebuggerEngine::getEngineForContext( cx ) ) {
        return JSR_ERROR_SM_DEBUGGER_ALREADY_INSTALLED;
    }

    JSAutoRequest req(cx);

    int contextId = _contextCounter++;

    AutoPtr<JSDebuggerEngine> engine = new JSDebuggerEngine( *this, cx, contextId, options );

    // Install base engine and decorate it a bit.
    error = engine->install();
    if( !error ) {

        // Enter into the new compartment.
        JSAutoCompartment cr( cx, engine->getDebuggerGlobal() );

        // Install asynchronous callback.
        DbgContextData *ctxData = new DbgContextData();
        ctxData->callbackChain = JS_SetOperationCallback( cx, &JSOperationCallback_AsyncCommand );
        ctxData->callbackDisabled = false;
        ctxData->debugger = this;
        ctxData->contextId = contextId;
        ctxData->paused = false;

        JSContextDescriptor desc;
        desc.context = cx;
        desc.contextId = contextId;
        desc.contextName = contextName;

        _contextMap.insert( map_context_pair( ctxData->contextId, desc ) );

        engine->setTag( ctxData );

        // Prevents engine deletion.
        engine.reset();

    }

    return error;
}

/**
 * Registers new debuggee inside the debugger engine.
 */
int SpiderMonkeyDebugger::registerDebuggee( JSContext *cx, const JS::HandleObject debuggee ) {

    MutexLock locker(_lock);

    JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext( cx );
    if( engine ) {
        return engine->registerDebuggee( debuggee );
    } else {
        return JSR_ERROR_SM_DEBUGGER_IS_NOT_INSTALLED;
    }

}

/**
 * Unregisters debuggee from the debugger instance.
 */
int SpiderMonkeyDebugger::unregisterDebuggee( JSContext *cx, const JS::HandleObject debuggee ) {

    MutexLock locker(_lock);

    JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext( cx );
    if( engine ) {
        return engine->unregisterDebuggee( debuggee );
    } else {
        return JSR_ERROR_SM_DEBUGGER_IS_NOT_INSTALLED;
    }

}

int SpiderMonkeyDebugger::interrupt( JSContext *cx ) {

    MutexLock locker(_lock);

    // Interrupt all debugger action queues.
    for( map_context_iterator it = _contextMap.begin(); it != _contextMap.end(); it++ ) {
        JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext(it->second.context);
        if( engine ) {
            DbgContextData *data = ENGINE_DATA( engine );
            // After this method the queue is inoperable. It'll always
            // throw interrupt exception when it's going to block.
            data->actionQueue.interrupt();
        } else {
            _log.error( "SpiderMonkeyDebugger::interrupt: Engine not registered for ID: %d", it->first );
        }
    }

    return JSR_ERROR_NO_ERROR;
}

int SpiderMonkeyDebugger::uninstall( JSContext *cx ) {

    MutexLock locker(_lock);

    AutoPtr<JSDebuggerEngine> engine = JSDebuggerEngine::getEngineForContext( cx );
    if( !engine ) {
        return JSR_ERROR_SM_DEBUGGER_IS_NOT_INSTALLED;
    }

    AutoPtr<DbgContextData> data = ENGINE_DATA( engine );

    // Remember that there is no need to break the command loop because
    // this method has to be called on the same thread which runs the
    // JS engine, so it's just impossible to call it while the command
    // loop is still operating.

    if( data ) {

        // Restore the callback.
        JS_SetOperationCallback( cx, data->callbackChain );

    }

    engine->uninstall();

    if( data ) {
        _contextMap.erase( data->contextId );
    } else {
        _log.error( "SpiderMonkeyDebugger::uninstall: Engine shouldn't be NULL. It seems to be a bug!" );
    }

    return JSR_ERROR_NO_ERROR;
}

int SpiderMonkeyDebugger::loadScript( JSContext *cx, std::string file, std::string &script ) {

    IJSScriptLoader *loader =  _cfg.getScriptLoader();
    if( loader ) {
        return loader->load( cx, file, script );
    }

    return JSR_ERROR_NO_ERROR;

}

bool SpiderMonkeyDebugger::sendCommand( int clientId, int contextId, std::string &commandRaw ) {

    // Prepare a command and sent it to the client. Notice that we
    // use byte buffer directly here, so user will get utf8 encoded
    // content.
    Command command( clientId, contextId, commandRaw );

    // As long as the command is not a broadcast one,
    // this is a blocking operation, so debuggee/debugger
    // will be blocked as long as the code is sent to the client's
    // output queue.
    return _clientManager.sendCommand( command );
}

bool SpiderMonkeyDebugger::waitForCommand( JSContext *cx, bool suspended ) {
    return JSR_CommandLoop( cx, true, suspended );
}

const JSRemoteDebuggerCfg &SpiderMonkeyDebugger::getDebuggerConf() const {
    return _cfg;
}

ClientManager &SpiderMonkeyDebugger::getClientManager() const {
    return _clientManager;
}

void SpiderMonkeyDebugger::setContextPaused( JSContext *cx, bool paused ) {

    MutexLock locker(_lock);

    JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext( cx );
    if( engine ) {
        ENGINE_DATA( engine )->paused = paused;
    } else {
        _log.error( "SpiderMonkeyDebugger::setContextPaused: Engine not found for JSContext." );
    }

}

bool SpiderMonkeyDebugger::isContextPaused( JSContext *cx ) {

    MutexLock locker(_lock);

    JSDebuggerEngine *engine = JSDebuggerEngine::getEngineForContext( cx );

    if( engine ) {
        return ENGINE_DATA( engine )->paused;
    } else {
        _log.error( "SpiderMonkeyDebugger::isContextPaused: Engine not found for JSContext." );
    }

    return false;
}

int SpiderMonkeyDebugger::_contextCounter = 0;
