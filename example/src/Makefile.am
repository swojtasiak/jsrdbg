SUBDIRS = js

noinst_PROGRAMS = jsrdbg_example

jsrdbg_example_SOURCES = main.cpp

jsrdbg_example_CPPFLAGS = -Wall $(MOZJS_CFLAGS) $(JSRDBG_CFLAGS) -Wno-invalid-offsetof
jsrdbg_example_LDADD = $(MOZJS_LIBS) $(JSRDBG_LIBS) js/libdbgexampleres.la
