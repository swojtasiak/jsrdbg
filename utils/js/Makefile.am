@SET_MAKE@

include $(top_srcdir)/Commons.mk

noinst_LTLIBRARIES = libresutils.la

libresutils_la_SOURCES = utils_resources.cpp \
	utils_resources.hpp

libresutils_la_CPPFLAGS = -z noexecstack

libresutils_la_LIBADD = moduleArgsParser.js.lo

libresutils_la_LDFLAGS = -no-undefined 
