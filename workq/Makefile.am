lib_LTLIBRARIES = libworkq.la

libworkq_la_SOURCES = server.cc worker.cc channel.cc adaptor.cc \
										  mysql_adaptor.cc pstring.cc \
       							  settings.cc util.cc job.cc

libworkq_la_CPPFLAGS = -I$(top_srcdir)/ $(MYSQL_CFLAGS) -DWQ_CONFIG_LIB_DIR="\"$(libdir)/workq/\""
libworkq_la_LDFLAGS= -version-info $(WORKQ_LIBRARY_VERSION) -release $(WORKQ_RELEASE)

library_includedir=$(includedir)/workq-1.0/workq

library_include_HEADERS = server.h worker.h channel.h adaptor.h\
												  mysql_adaptor.h pstring.h \
                          settings.h util.h job.h

libworkq_la_configdir=$(libdir)/workq-1.0/include
libworkq_la_config_DATA= config.h
