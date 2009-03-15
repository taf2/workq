#include "testlib.h"

int g_assertion_count = 0;
int g_failure_count = 0;
int g_test_count = 0;

void record_failure( const char *assertion, const char *file, unsigned int line, const char *function, const char *info)
{
  ++g_failure_count;
  g_print("\n%s:%u: %s: Assertion `%s' failed.\n",file,line,function,assertion);
  if( info ) {
    g_print("details: %s\n", info);
  }
}

void report_finish(GTimer*timer)
{
  g_print("\n\nFinished in \033[1;33m%f\033[0m seconds.\n", g_timer_elapsed(timer,0) );
  if( g_failure_count > 0 ) {
    g_print("\033[31m");
  }
  else {
    g_print("\033[32m");
  }
  g_print("\n%d Tests, %d assertions, %d failures\n", g_test_count, g_assertion_count, g_failure_count);
  g_print("\033[0m");
}

void test_log_handler(const gchar *log_domain,GLogLevelFlags log_level,const gchar *message,gpointer user_data)
{
  fprintf((FILE*)user_data, "%s", message);
  fflush((FILE*)user_data);
}

void run_tests( const char *suite, ... )
{ 
  va_list ap;
  va_start( ap, suite );
  test_func_t test_ptr;
  g_test_count = 0;

  FILE *log = fopen("logs/test.log","wb");
  //g_mem_set_vtable(glib_mem_profiler_table);
  GTimer *timer = g_timer_new();

  //g_log_set_handler(NULL, (GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), test_log_handler, log);

  g_timer_start(timer);

  while( (test_ptr = va_arg(ap,test_func_t)) ) {
    test_ptr();
    g_test_count++;
  }

  va_end(ap);

  g_timer_stop(timer);
  report_finish(timer);
  g_timer_destroy(timer);
  fclose(log);

//  g_mem_profile();
}
gchar *get_absolute_dir_from_file(const char *file)
{
  if( g_path_is_absolute(file) ) {
    return g_path_get_dirname(file);
  }
  gchar *current = g_get_current_dir();
  gchar *base = g_path_get_dirname(file);
  gchar *path = g_build_filename(current,base,NULL);
  g_free(current);
  g_free(base);
  return path;
}
