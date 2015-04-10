
@SET_MAKE@

SUBDIRS = js

lib_LTLIBRARIES = libjsrdbg.la

libjsrdbg_la_SOURCES = jsrdbg.cpp \
	jsldbg.cpp \
	client.hpp \
	client.cpp \
	protocol.hpp \
	protocol.cpp \
	tcp_protocol.hpp \
	tcp_protocol.cpp \
	resources.hpp \
	resources.cpp \
	debuggers.hpp \
	debuggers.cpp \
	js_remote_dbg.hpp \
	js_remote_dbg.cpp \
	js_dbg_engine.hpp \
	js_dbg_engine.cpp \
	message_builder.hpp \
	message_builder.cpp \
	common.cpp
	
libjsrdbg_la_CPPFLAGS = -I$(top_srcdir)/utils -I$(top_srcdir)/public $(MOZJS_CFLAGS) -Wno-invalid-offsetof
libjsrdbg_la_LDFLAGS = -no-undefined $(MOZJS_LIBS)
libjsrdbg_la_LIBADD = $(top_srcdir)/utils/libutils.la js/libdbgres.la 

pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = libjsrdbg.pc

libjsrdbg.pc: libjsrdbg.pc.in
	@sed -e 's![@]prefix[@]!$(prefix)!g' \
		-e 's![@]exec_prefix[@]!$(exec_prefix)!g' \
		-e 's![@]includedir[@]!$(includedir)!g' \
		-e 's![@]libdir[@]!$(libdir)!g' \
		-e 's![@]PACKAGE_VERSION[@]!$(PACKAGE_VERSION)!g' \
		-e 's![@]MOZJS_LDFLAGS[@]!$(MOZJS_LDFLAGS)!g' \
		-e 's![@]MOZJS_LIBS[@]!$(MOZJS_LIBS)!g' \
		$(srcdir)/libjsrdbg.pc.in > $@

EXTRA_DIST = libjsrdbg.pc.in

CLEANFILES = *.pc
