noinst_PROGRAMS = mysql_test \
								  settings_test \
								  work_queue_test \
								  pstring_test \
								  mock_adaptor_test \
								  run_job_test

noinst_LIBRARIES = libtest.a
EXTRA_DIST = ./config/jobs.yml ./fixtures/jobs.yml testlib.h mock_sql_adaptor.h

libtest_a_SOURCES  = testlib.cc mock_sql_adaptor.cc
libtest_a_CPPFLAGS = -I../ -I$(top_srcdir)/ $(MYSQL_CFLAGS)

mysql_test_SOURCES  = mysql_test.cc
mysql_test_LDADD    = libtest.a
mysql_test_LDFLAGS  = -L../workq/ -lworkq $(MYSQL_LDFLAGS)
mysql_test_CPPFLAGS = -I../ -I$(top_srcdir)/ $(MYSQL_CFLAGS)

settings_test_SOURCES  = settings_test.cc
settings_test_LDADD    = libtest.a
settings_test_LDFLAGS  = -L../workq/ -lworkq $(MYSQL_LDFLAGS)
settings_test_CPPFLAGS = -I../ -I$(top_srcdir)/ $(MYSQL_CFLAGS)

work_queue_test_SOURCES  = work_queue_test.cc 
work_queue_test_LDADD    = libtest.a
work_queue_test_LDFLAGS  = -L../workq/ -lworkq $(MYSQL_LDFLAGS)
work_queue_test_CPPFLAGS = -I../ -I$(top_srcdir)/ $(MYSQL_CFLAGS)

pstring_test_SOURCES  = pstring_test.cc 
pstring_test_LDADD 	  = libtest.a
pstring_test_LDFLAGS  = -L../workq/ -lworkq  $(MYSQL_LDFLAGS)
pstring_test_CPPFLAGS = -I../ -I$(top_srcdir)/ $(MYSQL_CFLAGS)

mock_adaptor_test_SOURCES  = mock_adaptor_test.cc
mock_adaptor_test_LDADD    = libtest.a
mock_adaptor_test_LDFLAGS  = -L../workq/ -lworkq $(MYSQL_LDFLAGS)
mock_adaptor_test_CPPFLAGS = -I../ -I$(top_srcdir)/ $(MYSQL_CFLAGS)

run_job_test_SOURCES  = run_job_test.cc
run_job_test_LDADD 	  = libtest.a
run_job_test_LDFLAGS  = -L../workq/ -lworkq $(MYSQL_LDFLAGS)
run_job_test_CPPFLAGS = -I../ -I$(top_srcdir)/ $(MYSQL_CFLAGS)
