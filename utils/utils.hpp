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

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#ifdef _WIN32
#include <Windows.h>
#endif

#include <stddef.h>
#include <assert.h>

#include <stdexcept>
#include <vector>
#include <functional>
#include <string>

namespace Utils {

/**
 * Helper class for exception-safe code.
 */
class OnScopeExit final
{
public:
    template <typename Function>
    OnScopeExit(Function dtor) try
        : _func(std::move(dtor)) {
    } catch (std::exception&) {
        dtor();
    }

    ~OnScopeExit() {
        if (_func) {
            try {
                _func();
            } catch (std::exception&) {
                assert(false);
            }
        }
    }

    void release() { _func = nullptr; }

private:
    std::function<void(void)> _func;

    OnScopeExit(); // n/a
    OnScopeExit(const OnScopeExit&); // n/a
    OnScopeExit& operator=(const OnScopeExit&); // n/a
};

/**
 * Base class for all events.
 */
class Event {
public:
    Event( int code );
    virtual ~Event();
public:
    virtual void setReturnCode( int code );
    virtual int getReturnCode();
    virtual int getCode();
private:
    int _code;
    int _returnCode;
};

/**
 * Generic event handler which can be used to handle almost
 * every kind of events in the system environment.
 */
class EventHandler {
public:
    EventHandler();
    virtual ~EventHandler();
public:
    virtual void handle( Event &event ) = 0;
};

class Mutex;

/**
 * Inherit from this class in order to add events firing
 * related functionality to your own class.
 * @warning This class is thread safe.
 */
class EventEmitter {
public:
    EventEmitter();
    virtual ~EventEmitter();
public:
    /**
     * Adds new events handler.
     */
    void addEventHandler( EventHandler *handler );
    /**
     * Removed given event handler.
     */
    void removeEventHandler( EventHandler *handler );
protected:
    /**
     * Fires given event to all the registered event handlers.
     * @param event Event to be fired.
     */
    void fire( Event &event );
    /**
     * Fires standard CodeEvent to all registered clients.
     * @param Code to be fired.
     */
    void fire( int code );
private:
    // Registered event handlers.
    Mutex *_mutex;
    std::vector<EventHandler*> _eventHandlers;
};

/**
 * Just inherit from this class to make your class non copyable.
 */
class NonCopyable {
protected:
    NonCopyable() {
    }
private:
    /**
    * @throw OperationNotSupportedException
    */
    NonCopyable( const NonCopyable &cpy ) {
        throw std::runtime_error("Operation not supported.");
    }
    /**
    * @throw OperationNotSupportedException
    */
    NonCopyable& operator=( const NonCopyable &exc ) {
        throw std::runtime_error("Operation not supported.");
    }
};

template<typename T>
class AutoPtr {
public:
    AutoPtr( T* ptr = nullptr ) : _ptr(ptr) {
    }
    ~AutoPtr() {
        if( _ptr ) {
            delete _ptr;
        }
    }
    AutoPtr& operator=(AutoPtr<T> &other) {
        if( &other != this ) {
            _ptr = other._ptr;
        }
        return *this;
    }
    AutoPtr& operator=(T* other) {
        if( other != _ptr ) {
            _ptr = other;
        }
        return *this;
    }
    T& operator*() const {
        return *_ptr;
    }
    T* operator&() const {
        return _ptr;
    }
    T* operator->() const {
        return _ptr;
    }
    operator bool() {
        return _ptr != nullptr;
    }
    operator T*() {
        return _ptr;
    }
    operator T&() {
        return *_ptr;
    }
    void reset() {
        _ptr = nullptr;
    }
    T* get() {
        return _ptr;
    }
    T* release() {
        T *tmp = nullptr;
        if( _ptr ) {
            tmp = _ptr;
            _ptr = nullptr;
        }
        return tmp;
    }
private:
    T *_ptr;
};

#ifdef _WIN32

// Windows conversion functions, see http://utf8everywhere.org/#windows

// Maps a UTF-16 (wide character) string to a new UTF-8 encoded character
// string.
//
// Parameters:
// s - Pointer to a 0-terminated Unicode string to convert
//
// Note: this function throws a runtime exception if an invalid input
// character is encountered. This function never silently drops illegal
// code points.
inline std::string narrow(const wchar_t* s) {
    const int bytesNeeded = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, s, -1, nullptr, 0, nullptr, nullptr);

    if (bytesNeeded == 0) {
        DWORD errcode = GetLastError();
        if (errcode == ERROR_NO_UNICODE_TRANSLATION) {
            throw std::runtime_error(
                "Invalid Unicode was found in input string");
        }

        // Anything else should be considered a programming error

        if (errcode == ERROR_INSUFFICIENT_BUFFER) {
            assert(false && "A supplied buffer size was not large enough, or "
                "it was incorrectly set to 0");
        }

        if (errcode == ERROR_INVALID_FLAGS) {
            assert(false && "The values supplied for flags were not valid");
        }

        if (errcode == ERROR_INVALID_PARAMETER) {
            assert(false && "One of the parameter values was invalid");
        }

        throw std::runtime_error(
            "Unknown error occurred in call to WideCharToMultiByte");
    }

    std::vector<char> byteArray(bytesNeeded, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s, -1, &byteArray[0],
        bytesNeeded, nullptr, nullptr);
    // Note that the resulting character string has a terminating null
    // character, and the length returned by the function includes this
    // character
    return std::string(begin(byteArray), end(byteArray) - 1);
}

inline std::string narrow(const std::wstring& s) { return narrow(s.c_str()); }

// Converts a UTF-8 encoded character string to a UTF-16 (wide character)
// string.
//
// Parameters:
// s - Pointer to a 0-terminated UTF-8 string
//
// Note: this function throws a runtime exception if an invalid input
// character is encountered. This function never silently drops illegal
// code points.
inline std::wstring widen(const char* s)
{
    const int bytesNeeded =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, nullptr, 0);

    if (bytesNeeded == 0) {
        DWORD errcode = GetLastError();
        if (errcode == ERROR_NO_UNICODE_TRANSLATION) {
            throw std::runtime_error(
                "Invalid Unicode was found in input string");
        }

        // Anything else should be considered a programming error

        if (errcode == ERROR_INSUFFICIENT_BUFFER) {
            assert(false && "A supplied buffer size was not large enough, or "
                "it was incorrectly set to 0");
        }

        if (errcode == ERROR_INVALID_FLAGS) {
            assert(false && "The values supplied for flags were not valid");
        }

        if (errcode == ERROR_INVALID_PARAMETER) {
            assert(false && "One of the parameter values was invalid");
        }

        throw std::runtime_error(
            "Unknown error occurred in call to WideCharToMultiByte");
    }

    std::vector<wchar_t> byteArray(bytesNeeded, '\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, &byteArray[0],
        bytesNeeded);
    // Again, resulting character string has a terminating null character,
    // and the length returned by the function includes this character
    return std::wstring(begin(byteArray), end(byteArray) - 1);
}

inline std::wstring widen(const std::string& s) { return widen(s.c_str()); }

// Returns a UTF-8 encoded string containing a system error message
// corresponding to given error code.
//
// Note that you always can try this:
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms681381(v=vs.85).aspx
inline std::string systemErrorString(int errcode) {
    // FORMAT_MESSAGE_FROM_SYSTEM: use system message tables to retrieve error
    // text
    // FORMAT_MESSAGE_ALLOCATE_BUFFER: let the function allocate a buffer on
    // local heap for us
    // FORMAT_MESSAGE_IGNORE_INSERTS: we don't pass insertion parameters, see
    // https://blogs.msdn.microsoft.com/oldnewthing/20071128-00/?p=24353

    wchar_t* buf = nullptr;
    const auto len = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buf), 0, nullptr);

    if (buf == nullptr) {
        int newError = GetLastError();
        throw std::runtime_error(
            "Could not get system error string, new error: " +
            std::to_string(newError));
    }

    OnScopeExit freeBuffer([&] { LocalFree(buf); });

    // Note: FormatMessage adds a '\r\n' newline to the message
    std::wstring msg(buf, len - 2);
    return narrow(msg);
}

#endif

}

#endif /* SRC_UTILS_H_ */
