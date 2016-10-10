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

#ifndef SRC_ENCODING_HPP_
#define SRC_ENCODING_HPP_

#include "jsapi.h"
#include "jsdbgapi.h"

#include <string>
#include <sstream>
#include <errno.h>
#include <iconv.h>
#include <langinfo.h>

namespace Utils {

#define WCE_LOCAL_BUFF_LEN          1024

class EncodingFailedException {
public:
    EncodingFailedException(const std::string& msg);
    ~EncodingFailedException();
    const std::string& getMsg() const;
private:
    std::string _msg;
};

typedef std::basic_string<jschar> jstring;
typedef std::basic_stringstream<jschar> jstringstream;

/**
 * Converts MBS characters into the wide characters. MBS characters are
 * always treated as encoded using default environment encoding.
 */
template<typename T>
class WideCharEncoder {
public:
    /**
     * Creates new encoder for given wide character encoding.
     * UTF-16 encoding is used by default.
     */
    WideCharEncoder( const char *encoding = "UTF-16LE" ) {
        _envCharSet = ::nl_langinfo(CODESET);
        _wideCharSet = encoding;
    }
    virtual ~WideCharEncoder() {
    }
public:

    typedef std::basic_string<T> tstring;

    /**
     * Converts string encoded in UTF-8 into wide character string.
     * @param str String encoded in UTF-8.
     * @return String represented using wide characters.
     */
    virtual tstring utf8ToWide( const std::string str ) {
        return encode<char,T>("UTF-8", _wideCharSet, str);
    }
    /**
     * Converts string encoded using environment default encoding
     * into wide character string.
     * @param str String encoded using environment encoding.
     * @return String represented using wide characters.
     */
    virtual std::basic_string<T> envToWide( const std::string str ) {
        return encode<char,T>(_envCharSet, _wideCharSet, str);
    }
    /**
     * Converts wide character string into UTF-8 one.
     * @param str String represented using wide characters.
     * @return String encoded in UTF-8.
     */
    virtual std::string wideToUtf8( const std::basic_string<T> str ) {
        return encode<T,char>(_wideCharSet, "UTF-8", str);
    }
    /**
     * Converts wide character string into MBCS one encoded using
     * default environment character set.
     * @param str String represented using wide characters.
     * @return String encoded using default character set.
     */
    virtual std::string wideToEnv( const std::basic_string<T> str ) {
        return encode<T,char>(_wideCharSet, _envCharSet, str);
    }

protected:

    template<typename S, typename D>
    std::basic_string<D> encode( const std::string &srcEncoding, const std::string &destEncoding, const std::basic_string<S> str ) {

        std::basic_stringstream<D> outStream;

        bool ignoreChars = false;

        D unknownChar = 0;

        if( !str.empty() ) {

            D buffer[WCE_LOCAL_BUFF_LEN];

            Iconv<S,D> iconv(srcEncoding, destEncoding);

            S *inPtr = const_cast<S*>(str.c_str());
            size_t inLen = str.length();

            while( inLen > 0 ) {
                D *outPtr = buffer;
                size_t outLeft = WCE_LOCAL_BUFF_LEN;
                D suffix = 0;
                // Convert characters.
                if( !iconv.iconv( inPtr, inLen, outPtr, outLeft ) ) {
                    if( iconv.error() == Iconv<S,D>::IE_INVALID_BYTE_SEQ ) {
                        // Illegal multibyte sequence, skip it.
                        if( !ignoreChars ) {
                            if( unknownChar == 0 ) {
                                unknownChar = prepareUnknownChar<D>(destEncoding, '?');
                                if( !unknownChar ) {
                                    ignoreChars = true;
                                }
                            }
                            if( unknownChar ) {
                                suffix = unknownChar;
                            }
                        }
                        if( inLen > 0 ) {
                            inPtr++;
                            inLen--;
                        } else {
                            throw EncodingFailedException("iconv: Character conversion failed with " + errno);
                        }
                    } else if( iconv.error() != Iconv<S,D>::IE_BUFFER_FULL ) {
                        throw EncodingFailedException("iconv: Character conversion failed with " + errno);
                    }
                }
                outStream.write( buffer, WCE_LOCAL_BUFF_LEN - outLeft );
                if( suffix ) {
                    outStream << suffix;
                }
            }

        }

        return outStream.str();
    }

    template<typename D>
    D prepareUnknownChar(const std::string &destEncoding, char replacerChar ) {
        try {
            std::string replacer;
            replacer.append(&replacerChar, 1);
            std::basic_string<D> encodedReplacer = encode<char,D>( _envCharSet, destEncoding, replacer );
            if( encodedReplacer.size() > 0 ) {
                return encodedReplacer.c_str()[0];
            }
        } catch( EncodingFailedException &exc ) {
        }
        return 0;
    }

    template<typename S, typename D>
    class Iconv {
    public:
        // Generic iconv errors.
        enum IconvError {
            IE_OK,
            IE_BUFFER_FULL,
            IE_INVALID_BYTE_SEQ,
            IE_FAILED
        };
        Iconv( std::string source, std::string destination ) {
            // Allocate ICONV converter.
            _cd = ::iconv_open(destination.c_str(), source.c_str());
            if( _cd == reinterpret_cast<iconv_t>(-1) ) {
                if( errno == EINVAL ) {
                    throw EncodingFailedException("Conversion from: " + source + " to: " + destination + " not available.");
                }
                throw EncodingFailedException("open_iconv failed.");
            }
            _error = IE_OK;
        }
        ~Iconv() {
            ::iconv_close(_cd);
        }
    public:
        // Lengths are represented in number or characters not bytes.
        size_t iconv( S *&inPtr, size_t &strLen, D *&outPtr, size_t &outLeft ) {
            bool result = true;
            // Iconv generates one wide char at a time, so it should be
            // perfectly allowed to assume that wide buffers length is
            // always multiplication of wide character size.
            size_t strLenBytes = strLen * sizeof(S);
            size_t outLeftBytes = outLeft * sizeof(D);
            size_t n = ::iconv( _cd, reinterpret_cast<char**>(&inPtr), &strLenBytes, reinterpret_cast<char**>(&outPtr), &outLeftBytes );
            strLen = strLenBytes / sizeof(S);
            outLeft = outLeftBytes / sizeof(D);
            if( n == static_cast<size_t>(-1) ) {
                switch( errno ) {
                case E2BIG:
                    _error = IE_BUFFER_FULL;
                    break;
                case EILSEQ:
                    _error = IE_INVALID_BYTE_SEQ;
                    break;
                default:
                    _error = IE_FAILED;
                }
                result = false;
            } else {
                _error = IE_OK;
            }
            return result;
        }
        IconvError error() {
            return _error;
        }
    private:
        iconv_t _cd;
        IconvError _error;
    };

private:
    // Character set used by environment.
    std::string _envCharSet;
    // Character set for wide strings.
    std::string _wideCharSet;
};

typedef WideCharEncoder<jschar> JCharEncoder;

}

#endif /* SRC_ENCODING_HPP_ */
