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

#ifndef JAR_SRC_FACADE_H_
#define JAR_SRC_FACADE_H_

#include <string>
#include <map>

#include <utils.hpp>
#include <threads.hpp>
#include <log.hpp>

#define MAX_CLIENT_QUEUE_LENGTH 4096

namespace JSR {

// Holds client related events, which should be
// identified by client's id.
class ClientEvent : public Utils::Event {
public:
    ClientEvent(int code,  int clientId);
    virtual ~ClientEvent();
public:
    int getID() const;
private:
    int _clientId;
};

/**
 * Represents one command.
 */
class Command {
public:
    static const int BROADCAST = -1;
    Command();
    Command( int clientId, int contextId, const std::string &command );
    Command( int clientId, int contextId, char *buffer, int offset, size_t size );
    Command( const Command &command );
    Command& operator =( const Command &command );
    virtual ~Command();
    virtual const std::string& getValue() const;
    virtual int getClientId() const;
    virtual int getContextId() const;
private:
    std::string _command;
    int _clientId;
    int _contextId;
};

typedef Utils::BlockingQueue<Command> command_queue;

/**
 * Default implementation of the client class.
 */
class Client {
public:
    /**
     * Creates client instance for a given ID.
     * @param id Client's ID.
     */
    explicit Client( int id );
    virtual ~Client();
public:
    /**
     * Disconnects the client. This method shouldn't block nor raise
     * any exceptions. If anything failed, just log it inside of this
     * method and return as if nothing happened.
     */
    virtual void disconnect();
    /**
     * Returns true if client is still connected.
     */
    virtual bool isConnected() const;
    /**
     * Gets numeric client ID. No matter what technology is used
     * to handle the client<->server communication we have to provide
     * a numeric client's ID just to identify the client using
     * a primitive value not prone to the memory deallocation.
     * @return Client's ID.
     */
    virtual int getID() const;
    /**
     * Gets input blocking queue.
     */
    virtual Utils::BlockingQueue<Command>& getInQueue();
    /**
     * Gets output blocking queue.
     */
    virtual Utils::BlockingQueue<Command>& getOutQueue();
private:
    // Queue for input commands.
    Utils::BlockingQueue<Command> _inCommands;
    // Queue for output commands.
    Utils::BlockingQueue<Command> _outCommands;
    // Client's ID.
    int _clientID;
};

/**
 * Component responsible for managing all the client
 * used by the server. It's mainly responsible for
 * the disposing procedure.
 */
class ClientManager : public Utils::EventEmitter {
public:
    ClientManager();
    virtual ~ClientManager();
public:
    /**
     * Starts client's manager.
     */
    virtual void start();
    /**
     * Adds client to the manager. It does nothing than
     * just informing the manager that there is a client
     * that should be disposed later, when the manager is shut
     * down.
     * @return Standard JSRDBG error code in case of critical exceptions.
     */
    virtual int addClient( Client *client );
    /**
     * Adds client to the disposing queue. From the time this
     * method has been invoked, client you can forgot about the
     * client. It's manager that is responsible for disposing
     * and freeing everything.
     */
    virtual void removeClient( Client *client );
    /**
     * Sends given command to all registered clients. This
     * is not blocking operation, so if there is no space left
     * in a queue, client is silently ignored when broadcasting.
     * @param command Command to send.
     */
    virtual void broadcast( Command &command );
    /**
     * Sends command to client or client if it's a broadcasting command.
     * If command is destined for concrete client the operation is blocking
     * if client's queue is full.
     * @param command Command to be send.
     * @return True if command has been send successfully, or false otherwise (no client found).
     */
    virtual bool sendCommand( Command &command );
    /**
     * Gets a client for given ID. This method increments
     * internal counter which is then used to prevent the
     * client from being removed, so it's very important to
     * use "returnClient" when client is not needed anymore.
     * @param id Client's ID.
     * @return Client instance or NULL.
     */
    virtual Client* getClient( int id );
    /**
     * Returns the client subtracting internal pointer by one.
     * See 'getClient' above.
     */
    virtual void returnClient( Client *client );
    /**
     * Call periodically in order to execute peding
     * actions if there a re any.
     */
    virtual void periodicCleanup();
    /**
     * Disposes all the clients registered inside the manager.
     */
    virtual int stop();
    /**
     * Gets number of clients managed.
     * @return Number of clients.
     */
    virtual int getClientsCount();
private:
    /**
     * Tries to remove client and marks it as
     * one to be removed if it cannot be removed.
     * @param client Client to be removed.
     * @param eventCode Event's code.
     * @return True if operation succeeded and client has been removed.
     */
    bool tryRemoveClient( Client *client, int &eventCode );
public:
    // Client has been added to the manager.
    static const int EVENT_CODE_CLIENT_ADDED = 1;
    // Client has been removed from the manager.
    static const int EVENT_CODE_CLIENT_REMOVED = 2;
    // Client has been marked to be removed as soon as possible.
    static const int EVENT_CODE_CLIENT_MARKED_TO_REMOVE = 3;
private:
    // Handles the reference counting logic.
    class ClientWrapper {
    public:
        explicit ClientWrapper(Client *client);
        ClientWrapper(const ClientWrapper &cpy);
        Client* getClient();
        void returnClient( Client *client );
        bool isConnected();
        bool isRemovable();
        void markRemove();
        bool isMarkedToRemove();
        void deleteClient();
    private:
        // A pointer to the client itself.
        Client *client;
        // A kind of reference counter.
        int counter;
        // True if client is marked to be removed when possible.
        bool removed;
    };
protected:
    Utils::Logger &_log;
    // Synchronized set of available clients.
    Utils::Mutex _mutex;
    // Map of managed clients.
    std::map<int,ClientWrapper> _clients;
};

/**
 * Holds client's pointer and helps to return it correctly.
 */
template<typename T>
class ClientPtrHolder : public Utils::NonCopyable {
public:
    explicit ClientPtrHolder( ClientManager &manager, int clientId = 0 )
        : _manager(manager) {
        _client = static_cast<T*>(manager.getClient(clientId));
    }
    ~ClientPtrHolder() {
        release();
    }
    ClientPtrHolder& operator=(Client* other) {
        if( other != _client ) {
            _client = static_cast<T*>(other);
        }
        return *this;
    }
    T& operator*() const {
        return *_client;
    }
    T* operator&() const {
        return static_cast<T*>(_client);
    }
    T* operator->() const {
        return static_cast<T*>(_client);
    }
    operator bool() {
        return _client != NULL;
    }
    void release() {
        if( _client ) {
            _manager.returnClient( _client );
            _client = NULL;
        }
    }
private:
    ClientManager &_manager;
    Client *_client;
};


}

#endif /* JAR_SRC_FACADE_H_ */
