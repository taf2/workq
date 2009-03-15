#ifndef TEST_WQ_MOCK_SQL_ADAPTOR_H
#define TEST_WQ_MOCK_SQL_ADAPTOR_H

#include "workq/adaptor.h"
#include <string>
#include <vector>
#include <map>
#include "workq/settings.h"


extern const WQ::Settings *wq_test_settings;

#define WQ_TEST_INIT_SETTINGS() \
  WQ::Settings s(*argv); \
  do { \
  gchar *path = get_absolute_dir_from_file(__FILE__); \
  gchar *test_conf = g_build_path("/", path, "tests", "config", "jobs.yml", NULL);\
\
  check( s.load_from_file(test_conf) );\
  check( s.verify() );\
  g_print("loading test config: %s\n", test_conf); \
\
  s.commit();\
\
  g_free(path);\
  g_free(test_conf);\
\
  wq_test_settings = &s; } while(0)

#define initialize_adaptor_with_fixtures(ma) do { \
  check( try_connect(ma) ); \
  check( create_jobs_table(ma) ); \
  gchar *path = get_absolute_dir_from_file(__FILE__); \
  gchar *fixture_path = g_build_path("/", path, "tests", "fixtures", "jobs.yml", NULL);\
  gint rows = WQ::DB::load_fixture(ma, "jobs", fixture_path); \
  g_print("loaded %d rows\n", rows); \
  g_free(path); \
  g_free(fixture_path); } while(0)

inline bool try_connect(WQ::DB::Adaptor &conn)
{
  const char *db_username = wq_test_settings->get_str("username");
  const char *db_password = wq_test_settings->get_str("password");
  const char *db_database = wq_test_settings->get_str("database");
  const char *db_host     = wq_test_settings->get_str("host");

  return conn.connect(db_username,db_password,db_host, db_database);
}

namespace WQ {
  namespace DB {
    typedef std::map<std::string,std::string> MRow;
    typedef std::vector<MRow> MockDB;

    gint load_fixture(Adaptor &adaptor, const char *table_name, const char *fixture_path, bool verbose = false);

    // if force is true the existing table will be dropped
    bool create_jobs_table(Adaptor &adaptor, bool force = true);

  }
}

#endif
