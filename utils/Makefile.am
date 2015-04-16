noinst_LTLIBRARIES = libutils.la
libutils_la_SOURCES = threads.cpp \
	threads.hpp \
	timestamp.cpp \
	timestamp.hpp \
	utils.cpp \
	encoding.hpp \
	encoding.cpp \
	log.hpp \
	log.cpp \
	js_utils.hpp \
	js_utils.cpp \
	utils.hpp \
	res_manager.cpp \
	res_manager.hpp

libutils_la_CPPFLAGS = $(MOZJS_CFLAGS) -Wno-invalid-offsetof -z noexecstack
libutils_la_LDFLAGS = -no-undefined $(MOZJS_LIBS)
