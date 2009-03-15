#include "testlib.h"
#include "workq/adaptor.h"
#include "workq/mysql_adaptor.h"
#include "mock_sql_adaptor.h"

void test_mysql_connection()
{
  { // test connection
    WQ::DB::MysqlAdaptor ma;

    check( try_connect(ma) );
    check( ma.alive() );
  }

  { // test no connection
    WQ::DB::MysqlAdaptor ma;
    check_equal( false, ma.connect("invalid","nogood",wq_test_settings->get_str("host"), wq_test_settings->get_str("database") ) );
  }
}

void test_mysql_query()
{
  WQ::DB::MysqlAdaptor ma;

  check( try_connect(ma) );
  check_equal( 0, ma.query("HELP 'create table'") );
}

void test_mysql_query_with_result()
{
  WQ::DB::MysqlAdaptor ma;

  check( try_connect(ma) );
  check_equal( 0, ma.query("SET NAMES UTF8") );
  check_equal( 0, ma.query("DROP TABLE IF EXISTS `test_table`") );
  check_equal_with_info( 0, ma.query("CREATE TABLE `test_table` (\
                                       `id` int(11) NOT NULL auto_increment,\
                                       `code` varchar(255) NOT NULL,\
                                        PRIMARY KEY  (`id`)\
                                      ) ENGINE=InnoDB DEFAULT CHARSET=utf8"), ma.details() );
  
  const int samples = 100;
  // insert some test data
  for( int i = 0; i < samples; ++i ) {
    check_equal( 0, ma.query("insert into `test_table` (code) values('test') ") );
  }

  WQ::DB::Adaptor::Iterator *it = ma.query_with_result("select id, code from test_table");

	check( it != NULL );

  if( it ) {

    int c = 1;
    for( WQ::DB::Adaptor::Row *row = it->begin(); row; row = it->next() ) {
      //printf("%d, %s\n", row->int_field("id"), row->str_field("code") );
      check_equal(c, row->int_field("id"));
      check_equal(0, strcmp("test", row->str_field("code")) );
      ++c;
      delete row;
    }

    delete it;

  }
}

int main(int argc, char **argv)
{
  WQ_TEST_INIT_SETTINGS();

  run_tests("Mysql:", test_mysql_connection, test_mysql_query, test_mysql_query_with_result, NULL);

  return 0;
}
