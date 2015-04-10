#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([jsrdbg-example], [0.0.1], [slawomir@wojtasiak.com])
AM_INIT_AUTOMAKE
LT_PREREQ([2.2])
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/main.cpp])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_CHECK_TOOL([OBJCOPY], [objcopy], [])
AM_CONDITIONAL(HAVE_OBJCOPY, [test x"$OBJCOPY" != x])

# SpiderMonkey.
PKG_CHECK_MODULES(MOZJS, [mozjs-25], [mozjs_version=25; break;],
	[PKG_CHECK_MODULES(MOZJS, [mozjs-24],[mozjs_version=24; break;], [
		])
	])

AC_SUBST(MOZJS_CFLAGS)
AC_SUBST(MOZJS_LIBS)

AS_IF([test "x$mozjs_version" = "x"],[AC_MSG_ERROR([SpiderMonkey not found.])])

# JSRDBG.
PKG_CHECK_MODULES([JSRDBG], [libjsrdbg >= 0.0.1])

AC_SUBST(JSRDBG_CFLAGS)
AC_SUBST(JSRDBG_LIBS)

AC_CONFIG_FILES([
Makefile
src/Makefile
src/js/Makefile
])

AC_OUTPUT

