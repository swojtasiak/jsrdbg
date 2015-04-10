SUBDIRS = js

noinst_PROGRAMS = jsrdbg_check

jsrdbg_check_SOURCES = main.cpp \
	resources.hpp \
	resources.cpp

jsrdbg_check_CPPFLAGS = -Wall -I$(top_srcdir)/public -I$(top_srcdir)/utils $(MOZJS_CFLAGS) -Wno-invalid-offsetof
jsrdbg_check_LDADD = $(top_srcdir)/src/libjsrdbg.la $(top_srcdir)/utils/libutils.la $(MOZJS_LIBS) js/libdbgcheckres.la

check_SCRIPTS = jsrdbg_check

TESTS = $(check_SCRIPTS)
