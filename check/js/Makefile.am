@SET_MAKE@

include $(top_srcdir)/Commons.mk

noinst_LTLIBRARIES = libdbgcheckres.la

libdbgcheckres_la_SOURCES = dbg_check.js \
	ts_simple_calculations.js \
	ts_simple_calculations_deb.js \
	ts_simple_calculations_functions.js \
	ts_debugger_statements.js \
	ts_deep_stacktrace.js

nodist_libdbgcheckres_la_SOURCES = dbg_check.js.lo \
	ts_simple_calculations.js.lo \
	ts_simple_calculations_deb.js.lo \
	ts_simple_calculations_functions.js.lo \
	ts_debugger_statements.js.lo \
	ts_deep_stacktrace.js.lo

libdbgcheckres_la_CPPFLAGS = -z noexecstack

libdbgcheckres_la_LIBADD = dbg_check.js.lo \
	ts_simple_calculations.js.lo \
	ts_simple_calculations_deb.js.lo \
	ts_simple_calculations_functions.js.lo \
	ts_debugger_statements.js.lo \
	ts_deep_stacktrace.js.lo

libdbgcheckres_la_LDFLAGS = -no-undefined

