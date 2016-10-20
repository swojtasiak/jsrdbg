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

#ifndef SRC_THREADS_H_
#define SRC_THREADS_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include <queue>

#include "utils.hpp"

namespace Utils {

/**
 * Thrown if given blocking method has been interrupted.
 */
class InterruptionException {
public:
    InterruptionException();
    virtual ~InterruptionException();
};

/**
 * Interface for classes which can be run on dedicated threads.
 */
class Runnable {
public:
    Runnable();
    virtual ~Runnable();
    virtual void run() = 0;
    virtual void interrupt();
};

class Condition;

/**
 * Standard mutex implementation.
 */
class Mutex : public NonCopyable {
public:
    Mutex();
    ~Mutex();
    void lock();
    void unlock();
private:
    friend Condition;
#ifdef _WIN32
    CRITICAL_SECTION _criticalsect;
#else
    pthread_mutex_t _mutex;
#endif
};

/**
 * Locks mutex as long as given scope is available.
 */
class MutexLock {
public:
    MutexLock( Mutex &mutex );
    ~MutexLock();
private:
    Mutex &_mutex;
};

/**
 * Conditional variable.
 */
class Condition : public NonCopyable {
public:
    Condition();
    ~Condition();
    void signal();
    void broadcast();
    void wait( Mutex &mutex );
    bool wait( Mutex &mutex, int milis );
private:
#ifdef _WIN32
    CONDITION_VARIABLE _condition;
#else
    pthread_cond_t _condition;
#endif
};

/**
 * Generic interface for the thread implementation.
 */
class Thread : public NonCopyable {
public:
    /**
     * Creates a thread for given runnable. Remember that you are
     * still the owner of the runnable, so thread won't delete it.
     */
    Thread(Runnable &runnable);
    /**
     * Destroys the thread.
     */
    virtual ~Thread();
    /**
     * Starts the new thread. Take into account that this method doesn't return any error
     * code. It's not a critical runtime system, so in case of such exceptions standard
     */
    virtual void start();
    /**
     * Sets thread's interrupted flag. It doesn't terminate the thread.
     * This flag has to be interpreted by the thread itself.
     */
    virtual void interrupt();
    /**
     * Wait's for the thread to terminate.
     */
    virtual void join();
    /**
     * Returns true if thread has been started.
     */
    virtual bool isStarted();
private:
    // Mutex used to synchronize thread starting process.
    Mutex _mutex;
    // True if thread has been already started.
    bool _started;
    // Runnable used by the thread.
    Runnable &_runnable;
    // Native thread.
#ifdef _WIN32
    HANDLE _threadh;
#else
    pthread_t _thread;
#endif
};

template<typename T>
class BlockingQueue;

/**
 * Generic signal handler.
 */
template<typename T>
class QueueSignalHandler {
public:
    QueueSignalHandler() {}
    virtual ~QueueSignalHandler() {}
    virtual void handle( BlockingQueue<T> &queue, int signal ) = 0;
};

/**
 * Simple blocking queue implementation.
 */
template<typename T>
class BlockingQueue {
public:

    static const int SIGNAL_NEW_ELEMENT = 1;

    BlockingQueue( int max = -1 ) :
        _max(max),
        _interrupt(false),
        _signalHandler(NULL) {
    }

    BlockingQueue(const BlockingQueue &queue)
        : _max(queue._max),
          _interrupt(false),
          _queue( queue._queue ),
          _signalHandler( queue._signalHandler ) {
        // Do not copy mutex and condition. Every queue
        // has their own independent instances.
    }

    BlockingQueue& operator=( const BlockingQueue &queue ) {
        if( &queue != this ) {
            _max = queue._max;
            _queue = queue._queue;
            _interrupt = queue._interrupt;
            _signalHandler = queue._signalHandler;
        }
        return *this;
    }

    ~BlockingQueue() {
        // Just in case.
        interrupt();
    }

    /**
     * Peeks a next element from the front of the queue without removing it.
     * This function doesn't block.
     */
    bool peek( T &element ) {
        bool exists = false;
        _mutex.lock();
        if( !_queue.empty() ) {
            element = _queue.front();
            exists = true;
        }
        _mutex.unlock();
        return exists;
    }

    /**
     * Pops the next element from the front of the queue.
     */
    void popOnly() {
        _mutex.lock();
        if( !_queue.empty() ) {
            _queue.pop();
            _conditionFull.signal();
            if( _queue.empty() ) {
                _conditionEmptyWait.broadcast();
            }
        }
        _mutex.unlock();
    }

    /**
     * Gets next element if there is any.
     */
    bool get( T &element ) {
        bool exists = false;
        _mutex.lock();
        if( !_queue.empty() ) {
            element = _queue.front();
            _queue.pop();
            _conditionFull.signal();
            if( _queue.empty() ) {
                _conditionEmptyWait.broadcast();
            }
            exists = true;
        }
        _mutex.unlock();
        return exists;
    }

    /**
     * Adds new element without blocking anything. Returns true if
     * element has been added.
     */
    bool add( T &element ) {
        bool result = true;
        _mutex.lock();
        if( _max != -1 && _queue.size() == _max ) {
            result = false;
        } else {
            _queue.push(element);
            _conditionEmpty.signal();
        }
        _mutex.unlock();
        if( _signalHandler ) {
            _signalHandler->handle( *this, SIGNAL_NEW_ELEMENT );
        }
        return result;
    }

    /**
     * Gets next element from the queue. It's a blocking operation
     * which blocks if there is nothing in the queue.
     * @throws InterruptionException
     */
    T pop() {
        _mutex.lock();
        if( _interrupt ) {
            _mutex.unlock();
            throw InterruptionException();
        }
        while( _queue.empty() ) {
            _conditionEmpty.wait( _mutex );
            if ( _interrupt ) {
                _mutex.unlock();
                throw InterruptionException();
            }
        }
        T element  = _queue.front();
        _queue.pop();
        _conditionFull.signal();
        if( _queue.empty() ) {
            _conditionEmptyWait.broadcast();
        }
        _mutex.unlock();
        return element;
    }

    /**
     * Adds new element into the queue.
     * @param element Element to add to the queue.
     */
    void push(T &element) {
        _mutex.lock();
        if( _interrupt ) {
            _mutex.unlock();
            throw InterruptionException();
        }
        while( _max != -1 && _queue.size() == _max ) {
            _conditionFull.wait( _mutex );
            if ( _interrupt ) {
                _mutex.unlock();
                throw InterruptionException();
            }
        }
        _queue.push(element);
        _conditionEmpty.signal();
        _mutex.unlock();
        if( _signalHandler ) {
            _signalHandler->handle( *this, SIGNAL_NEW_ELEMENT );
        }
    }

    /**
     * Interrupts threads waiting using pop method.
     */
    void interrupt() {
        _mutex.lock();
        _interrupt = true;
        _conditionEmpty.broadcast();
        _conditionFull.broadcast();
        _conditionEmptyWait.broadcast();
        _mutex.unlock();
    }

    /**
     * Returns true if queue is empty.
     */
    bool isEmpty() {
        bool empty;
        _mutex.lock();
        empty = _queue.empty();
        _mutex.unlock();
        return empty;
    }

    /**
     * Returns number of elements in the queue.
     */
    int getCount() {
        int count;
        _mutex.lock();
        count = static_cast<int>(_queue.size());
        _mutex.unlock();
        return count;
    }

    /**
     * Waits as long as the queue is empty.
     */
    void waitForEmpty() {
        _mutex.lock();
        if( _interrupt ) {
            _mutex.unlock();
            throw InterruptionException();
        }
        while( !_queue.empty() ) {
            _conditionEmptyWait.wait( _mutex );
            if ( _interrupt ) {
                _mutex.unlock();
                throw InterruptionException();
            }
        }
        _mutex.unlock();
    }

    /**
     * Sets new signal handler.
     * @param signalHandler Signal handler.
     */
    void setSignalHandler( QueueSignalHandler<T> *signalHandler ) {
        this->_signalHandler = signalHandler;
    }

private:
    int _max;
    bool _interrupt;
    Mutex _mutex;
    Condition _conditionEmpty;
    Condition _conditionFull;
    Condition _conditionEmptyWait;
    std::queue< T > _queue;
    QueueSignalHandler<T> *_signalHandler;
};

}

#endif /* SRC_THREADS_H_ */
