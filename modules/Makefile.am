EXTRA_DIST = run_job.h
pkglibdir=$(libdir)/workq/modules
pkglib_LTLIBRARIES = run.la

run_la_SOURCES = run_job.cc
run_la_LDFLAGS = -module -avoid-version $(JSON_GLIB_LIBS)
run_la_CPPFLAGS = -I$(top_srcdir) $(JSON_GLIB_CFLAGS)
