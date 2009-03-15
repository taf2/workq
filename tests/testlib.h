#ifndef TEST_LIB_H
#define TEST_LIB_H

#ifdef HAVE_CONFIG_H
#include "workq/config.h"
#endif
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "workq/settings.h"
#include <glib/gstdio.h>
#include <gmodule.h>
#include <glib-object.h>
#include "mock_sql_adaptor.h"
#include "workq/mysql_adaptor.h"
#include "workq/worker.h"

extern int g_assertion_count;
extern int g_failure_count;
extern int g_test_count;

void record_failure( const char *assertion, const char *file, unsigned int line, const char *function, const char *info = NULL);
void report_finish(GTimer*timer);
void test_log_handler(const gchar *log_domain,GLogLevelFlags log_level,const gchar *message,gpointer user_data);
// expect to pass __FILE__, returns string that should be g_free'ed
gchar *get_absolute_dir_from_file(const char *file);

typedef void(*test_func_t)();

void run_tests( const char *suite, ... );

// concept originally seen in GNU C Library
#define WQ_ASSERT_VOID_CAST static_cast<void>

#define check_with_info(expr,info)\
  ++g_assertion_count;\
  ((expr) ? WQ_ASSERT_VOID_CAST(0) : record_failure (__STRING(expr), __FILE__, __LINE__, __FUNCTION__), info)

#define check(expr)\
  check_with_info(expr,NULL)

#define check_equal_with_info(value,expr,info)\
    check_with_info( (value == (expr)), info )

#define check_equal(value,expr)\
    check_with_info( (value == (expr)), NULL )

#endif
