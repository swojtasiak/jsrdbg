@SET_MAKE@

SUBDIRS = js

bin_PROGRAMS=jrdb
jrdb_SOURCES=jrdb.cpp \
	events.hpp \
	events.cpp \
	fsevents.hpp \
	fsevents.cpp \
	readline.hpp \
	readline.cpp \
	config.hpp \
	getopt_config.hpp \
	getopt_config.cpp \
	tcp_client.hpp \
	tcp_client.cpp \
	dbg_client.hpp \
	dbg_client.cpp \
	debugger.hpp \
	debugger.cpp \
	js_debugger.hpp \
	js_debugger.cpp \
	resources.hpp \
	resources.cpp \
	errors.hpp
	
jrdb_CPPFLAGS = $(MOZJS_CFLAGS) -I$(top_srcdir)/utils -Wno-invalid-offsetof
jrdb_LDFLAGS = $(MOZJS_LIBS)
jrdb_LDADD = $(top_srcdir)/utils/libutils.la js/libdbgclientres.la

EXTRA_DIST = COPYING