#include "testlib.h"
#include "mock_sql_adaptor.h"
#include "workq/mysql_adaptor.h"

void test_inserting_fixtures()
{
  WQ::DB::MysqlAdaptor ma;

  check( try_connect(ma) );

  check( create_jobs_table(ma) );

  gchar *path = get_absolute_dir_from_file(__FILE__);
  gchar *fixture_path = g_build_path("/", path, "tests", "fixtures", "jobs.yml", NULL);

  check_equal( 7, WQ::DB::load_fixture(ma, "jobs", fixture_path) );

  g_free(path);
  g_free(fixture_path);

  WQ::DB::Adaptor::Iterator *it = ma.query_with_result("select id, name, status, attempts, locked_queue_id, created_at from jobs");
	check( it != NULL );
  if( it ) {
    int c = 0;
    for( WQ::DB::Adaptor::Row *row = it->begin(); row; row = it->next() ) {
      /*g_print("id: %d, name: %s, status: %s, attempts: %d, locked: %d\n", 
                  row->int_field("id"),
                  row->str_field("name"),
                  row->str_field("status"),
                  row->int_field("attempts"),
                  row->int_field("locked") );*/
      check_equal( (c+1), row->int_field("id") );
      check( !strcmp("test", row->str_field("name")) ||
             !strcmp("magick", row->str_field("name")) ||
             !strcmp("lua", row->str_field("name")) ||
             !strcmp("run", row->str_field("name")) ||
             !strcmp("ffmpeg", row->str_field("name")) );
      check( !strcmp("pending", row->str_field("status")) );
      delete row;
      ++c;
    }
    delete it;

    check_equal(7, c);
  }
}

int main(int argc, char **argv)
{
  WQ_TEST_INIT_SETTINGS();

  run_tests("Fixtures:", test_inserting_fixtures, NULL);
  return 0;
}
