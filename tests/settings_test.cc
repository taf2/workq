#include "testlib.h"
#include <string.h>
#include "workq/settings.h"

void test_settings_defaults()
{
  WQ::Settings s(__FILE__);

  check( !s.get_str("empty") );
  check( !strcmp( "value", s.get_str("empty", "value") ) );
  check_equal( 0, s.get_int("empty") );
  check_equal( 10, s.get_int("empty", 10) );
  check_equal( 0.0, s.get_double("empty") );
  check_equal( 10.0, s.get_double("empty", 10.0) );
}

void test_settings_load_sample()
{
  WQ::Settings s(__FILE__);
  gchar *path = get_absolute_dir_from_file(__FILE__);
  gchar *test_conf = g_build_path("/", path, "tests", "config", "jobs.yml", NULL);

  check( s.load_from_file(test_conf) );
  check( s.verify() );

  s.commit();

  check( !strcmp("mysql", s.get_str("adapter")) );
  check( !strcmp("jobs_test", s.get_str("database")) );
  //check( !strcmp("root", s.get_str("username")) );
  check( !strcmp("127.0.0.1", s.get_str("host")) );
  check_equal( 3306, s.get_int("port") );
  check_equal( 5, s.get_int("pool") );
  check_equal( 5, s.get_int("workers") );
  check_equal( 4488, s.get_int("listen") );
  check_equal( 10, s.get_int("pollrate") );

  g_free(path);
  g_free(test_conf);
}

int main()
{
  run_tests("Settings:", test_settings_defaults, test_settings_load_sample, NULL);
  return 0;
}
