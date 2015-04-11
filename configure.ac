#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([jsrdbg], [0.0.2], [slawomir@wojtasiak.com])
m4_include([m4/ax_pthread.m4])
m4_include([m4/ax_lib_readline.m4])
AM_INIT_AUTOMAKE
LT_PREREQ([2.2])
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/jsrdbg.cpp])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# objcopy is used to change the secrion of the JS files 
# linked directly into the library. It is optional tool though.
AC_CHECK_TOOL([OBJCOPY], [objcopy], [])
AM_CONDITIONAL(HAVE_OBJCOPY, [test x"$OBJCOPY" != x])

# Enable JS Engine debugging messages.

AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug],
	[enable debugging messages [EXPERT] @<:@default=no@:>@])],
	[debug="$enableval"], [debug=no])

AS_IF([test "x$debug" = xyes],
	[AC_DEFINE([JSRDBG_DEBUG], [1], [Define to enable debug messages from debugging engine [EXPERT].])])

# Checks for libraries.

# pthreads.
AX_PTHREAD( [
    AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.])
    CLIBS="$PTHREAD_LIBS $LIBS"
    CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
    LDFLAGS="$LDFLAGS $PTHREAD_CFLAGS"
    CC="$PTHREAD_CC"],[])

# realtime clock.
AC_MSG_NOTICE([checking if clock_gettime is available])
AC_SEARCH_LIBS([clock_gettime], [rt posix4])

# SpiderMonkey.
PKG_CHECK_MODULES(MOZJS, [mozjs-25], [mozjs_version=25; break;], 
	[PKG_CHECK_MODULES(MOZJS, [mozjs-24],[mozjs_version=24; break;], [
		])
	])

AC_SUBST(MOZJS_CFLAGS)
AC_SUBST(MOZJS_LIBS)

AS_IF([test "x$mozjs_version" = "x"],[AC_MSG_ERROR([SpiderMonkey not found.])])

AC_MSG_NOTICE([SpiderMonkey version: $mozjs_version])

# Check which version of SpiderMonkey is available.
AM_CONDITIONAL(HAVE_MOZJS_24, [test "x$mozjs_version" = "x24"])
AM_CONDITIONAL(HAVE_MOZJS_25, [test "x$mozjs_version" = "x25"])

AS_IF([test "x$mozjs_version" != "x"],
	[AC_DEFINE_UNQUOTED([MOZJS_VERSION], [$mozjs_version], [SpiderMonkey version.])])

# GNU/reaadline
AX_LIB_READLINE
AS_IF([test "x$ax_cv_lib_readline" = "xno"],[AC_MSG_ERROR([libreadline is mandatory.])])

# GNU/iconv
AM_ICONV
AS_IF([test "x$am_cv_func_iconv" != "xyes"],[AC_MSG_ERROR([libiconv is mandatory.])])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h netdb.h string.h sys/socket.h unistd.h syslog.h],
	[required_headers=yes; break;])
AS_IF([test "x$required_headers" != "xyes"],[AC_MSG_ERROR([Unable to find one of the required headers])])

AC_CHECK_HEADERS([stdint.h inttypes.h sys/types.h],
	[int_headers=yes; break;])
AS_IF([test "x$int_headers" != "xyes"],[AC_MSG_ERROR([Unable to find the standard integers headers])])

# Optional headers.
AC_CHECK_HEADERS([termios.h])

AC_CONFIG_FILES([
fix/Makefile
Makefile
public/Makefile
utils/Makefile
src/js/Makefile
src/Makefile
jrdb/js/Makefile
jrdb/Makefile
check/js/Makefile
check/Makefile
])

AC_OUTPUT
