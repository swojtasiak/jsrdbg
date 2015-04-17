@SET_MAKE@

include $(top_srcdir)/Commons.mk

noinst_LTLIBRARIES = libdbgres.la

libdbgres_la_SOURCES = js_resources.hpp \
	js_resources.cpp

libdbgres_la_CPPFLAGS = -I$(top_srcdir)/utils -z noexecstack

libdbgres_la_LIBADD = mozjs_dbg.js.lo \
	module.js.lo

libdbgres_la_LDFLAGS = -no-undefined 

