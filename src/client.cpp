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

#include "client.hpp"

#include <vector>
#include <map>
#include <string>

#include <jsrdbg.h>

using namespace JSR;
using namespace Utils;
using namespace std;

/* Client event */

ClientEvent::ClientEvent(int code, int clientId)
    : Event(code),
      _clientId(clientId) {
}

ClientEvent::~ClientEvent() {
}

int ClientEvent::getID() const {
    return _clientId;
}

/* Command. */

Command::Command()
    : _clientId(0),
      _contextId(0) {
}

Command::Command( int clientId, int engineId, const std::string &command )
    : _command( command ),
      _clientId( clientId ),
      _contextId( engineId ){
}

Command::Command( int clientId, int engineId, char *buffer, int offset, size_t size )
    : _command( buffer, offset, size ),
      _clientId( clientId ),
      _contextId( engineId ) {
}

Command::Command( const Command &command )
    : _command(command._command),
      _clientId(command._clientId),
      _contextId(command._contextId) {
}

Command& Command::operator=( const Command &command ) {
    if (this != &command) {
        _command = command._command;
        _clientId = command._clientId;
        _contextId = command._contextId;
    }
    return *this;
}

Command::~Command() {
}

int Command::getClientId() const {
    return _clientId;
}

int Command::getContextId() const {
    return _contextId;
}

const std::string& Command::getValue() const {
    return _command;
}

/* Client. */

Client::Client( int id ) :
        _inCommands(MAX_CLIENT_QUEUE_LENGTH),
        _outCommands(MAX_CLIENT_QUEUE_LENGTH),
        _clientID(id) {
}

Client::~Client() {
}

void Client::disconnect() {
}

bool Client::isConnected() const {
    return true;
}

int Client::getID() const {
    return _clientID;
}

BlockingQueue<Command>& Client::getInQueue() {
    return _inCommands;
}

BlockingQueue<Command>& Client::getOutQueue() {
    return _outCommands;
}

/* Client's manager */

ClientManager::ClientManager()
    : _log(LoggerFactory::getLogger()) {
}

ClientManager::~ClientManager() {
}

void ClientManager::start() {
}

int ClientManager::addClient( Client *client ) {
    if( !client ) {
        _log.error( "Cannot add NULL client." );
        return JSR_ERROR_ILLEGAL_ARGUMENT;
    }

    {
        MutexLock lock( _mutex );
        _clients.insert( std::pair<int,ClientWrapper>( client->getID(), ClientWrapper( client ) ) );
    }

    ClientEvent event(EVENT_CODE_CLIENT_ADDED, client->getID());
    fire(event);
    return JSR_ERROR_NO_ERROR;
}

void ClientManager::broadcast( Command &command ) {
    vector<int> ids;
    // Collect all clients identificators.

    {
        MutexLock lock( _mutex );

        for( map<int,ClientWrapper>::iterator it = _clients.begin(); it != _clients.end(); it++ ) {
            ids.push_back(it->first);
        }
    }

    // Now having all identificators collected, try to send commands to clients
    // that are in conjunction with these identificators.
    for( vector<int>::iterator it = ids.begin(); it != ids.end(); it++ ) {
        ClientPtrHolder<Client> client( *this, *it );
        // Ignore clients that have been removed in this time window.
        if( client ) {
            client->getOutQueue().add( command );
        }
    }
}

bool ClientManager::sendCommand( Command &command ) {
    bool result = true;
    if( command.getClientId() == Command::BROADCAST ) {
        broadcast( command );
    } else {
        ClientPtrHolder<Client> client( *this, command.getClientId() );
        if( client ) {
            client->getOutQueue().push( command );
        } else {
            result = false;
        }
    }
    return result;
}

void ClientManager::removeClient( Client *client ) {
    if( !client ) {
        _log.error( "Cannot remove NULL client." );
        return;
    }
    int clientId = 0;
    int eventCode = 0;
    tryRemoveClient( client, eventCode );
    // Events cannot be fired inside the critical section
    // just because they might be blocked by actions which use
    // client manager from different threads, so it could
    // cause deadlocks.
    if( eventCode ) {
        ClientEvent event(eventCode, clientId);
        fire(event);
    }
}

bool ClientManager::tryRemoveClient(Client *client, int &eventCode) {
    // Only this block should be synchronized.
    MutexLock lock( _mutex );
    int clientId = client->getID();
    bool remove = false;
    // Remove the client if it exists.
    std::map<int,ClientWrapper>::iterator it = _clients.find( clientId );
    if( it != _clients.end() ) {
        ClientWrapper &wrapper = it->second;
        if( !wrapper.isMarkedToRemove() ) {
            if( wrapper.isRemovable() ) {
                remove = true;
            } else {
                wrapper.markRemove();
                eventCode = EVENT_CODE_CLIENT_MARKED_TO_REMOVE;
            }
        } else if ( wrapper.isRemovable() ) {
            remove = true;
        }
        if( remove ) {
            wrapper.deleteClient();
            _clients.erase( it );
            eventCode = EVENT_CODE_CLIENT_REMOVED;
        }
    }
    return remove;
}

Client* ClientManager::getClient( int id ) {
    MutexLock lock( _mutex );
    std::map<int,ClientWrapper>::iterator it = _clients.find( id );
    if( it != _clients.end() ) {
        return it->second.getClient();
    }
    return NULL;
}

void ClientManager::returnClient( Client *client ) {
    if( !client ) {
        _log.error( "Cannot return NULL client." );
        return;
    }
    MutexLock lock( _mutex );
    std::map<int,ClientWrapper>::iterator it = _clients.find( client->getID() );
    if( it != _clients.end() ) {
        it->second.returnClient( client );
    } else {
        _log.error( "Cannot return client with id: %d, because it doesn't exist in the manager.", client->getID() );
    }
}

void ClientManager::periodicCleanup() {
    // Vector of clients that were removed.
    vector<int> removedClients;

    {
        MutexLock lock( _mutex );

        // Remove client's as soon as they can be removed.
        map<int,ClientWrapper>::iterator it = _clients.begin();
        while (it != _clients.end()) {
            if (it->second.isMarkedToRemove() && it->second.isRemovable()) {
                int clientId = it->second.getClient()->getID();
                it->second.deleteClient();
                map<int,ClientWrapper>::iterator toErase = it;
                ++it;
                _clients.erase(toErase);
                removedClients.push_back(clientId);
            } else {
                ++it;
            }
        }
    }

    // Fire events about removed clients.
    for( vector<int>::iterator it = removedClients.begin(); it != removedClients.end(); it++ ) {
        ClientEvent event(EVENT_CODE_CLIENT_REMOVED, *it);
        fire(event);
    }
}

int ClientManager::stop() {
    // Try to close and delete all existing clients. If any of them are in use yet,
    // they will be marked as 'to be removed' and can be removed further by calls
    // to 'periodicCleanup' its why the error code is returned.

    vector<int> removedClients;
    bool empty = true;
    {
        MutexLock lock( _mutex );

        map<int,ClientWrapper> clients = _clients;
        for( map<int,ClientWrapper>::iterator it = clients.begin(); it != clients.end(); it++ ) {
            int eventCode;
            Client *client = it->second.getClient();
            int clientId = client->getID();
            if( tryRemoveClient( client, eventCode ) ) {
                removedClients.push_back( clientId );
            }
        }
        empty = _clients.empty();
    }

    // Fire events about removed clients.
    for( vector<int>::iterator it = removedClients.begin(); it != removedClients.end(); it++ ) {
        ClientEvent event(EVENT_CODE_CLIENT_REMOVED, *it);
        fire(event);
    }
    if( !empty ) {
        return JSR_ERROR_CANNOT_REMOVE_CONNECTIONS;
    }
    return JSR_ERROR_NO_ERROR;
}

int ClientManager::getClientsCount() {
    MutexLock lock( _mutex );
    int clients = _clients.size();
    return clients;
}

ClientManager::ClientWrapper::ClientWrapper(Client *client) {
   this->client = client;
   this->counter = 0;
   this->removed = false;
}

ClientManager::ClientWrapper::ClientWrapper(const ClientWrapper &cpy) {
   this->client = cpy.client;
   this->counter = cpy.counter;
   this->removed = cpy.removed;
}

Client* ClientManager::ClientWrapper::getClient() {
    counter++;
    return client;
}

void ClientManager::ClientWrapper::returnClient( Client *client ) {
    counter--;
}

bool ClientManager::ClientWrapper::isConnected() {
    return client && client->isConnected();
}

bool ClientManager::ClientWrapper::isRemovable() {
    return counter == 0;
}

void ClientManager::ClientWrapper::markRemove() {
    removed = true;
}

bool ClientManager::ClientWrapper::isMarkedToRemove() {
    return removed;
}

void ClientManager::ClientWrapper::deleteClient() {
    delete client;
    client = NULL;
}
