@SET_MAKE@

include $(top_srcdir)/Commons.mk

noinst_LTLIBRARIES = libdbgclientres.la

libdbgclientres_la_SOURCES = js_resources.cpp \
	js_resources.hpp

libdbgclientres_la_CPPFLAGS = -I$(top_srcdir)/utils -z noexecstack

libdbgclientres_la_LIBADD = mozjs_dbg_client.js.lo

libdbgclientres_la_LDFLAGS = -no-undefined 
	