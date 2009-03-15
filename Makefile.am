bin_PROGRAMS = workqd
EXTRA_DIST = README ChangeLog
DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

workqd_SOURCES  = workqd.cc
workqd_LDADD    = workq/libworkq.la
workqd_LDFLAGS  = -static $(MYSQL_LDFLAGS)
workqd_CPPFLAGS = -I$(top_srcdir) $(MYSQL_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = workq-1.0.pc

SUBDIRS = workq modules tests docs


all-test:
	./tests/mysql_test
	./tests/settings_test
	./tests/pstring_test
	./tests/mock_adaptor_test
	./tests/work_queue_test
	./tests/run_job_test

# set the current job to test here
#all-local:
#	./tests/settings_test

dist-hook:
	rm -fr $(distdir)/tests/config/.svn
	rm -fr $(distdir)/tests/fixtures/.svn
	find $(distdir) -name '.svn' | xargs rm -rf
# Count the number of lines in *.h and *.cpp files
linecount:
	@ echo -n 'core: ' ; wc -l `find workq -name '*.cc'` `find workq -name '*.h'` | grep total
	@ echo -n 'modules: ' ; wc -l `find modules -name '*.cc'` `find modules -name '*.h'` | grep total
	@ echo -n 'server: ' ; wc -l workqd.cc | grep total
    
# see: http://ch.tudelft.nl/~arthur/svn2cl/downloads.html
changelog:
	svn log --xml --verbose | xsltproc svn2cl-0.11/svn2cl.xsl - > ChangeLog
