#include "mock_sql_adaptor.h"
#include <yaml.h>

const WQ::Settings *wq_test_settings = NULL;

namespace WQ {
namespace DB {

gint load_fixture(Adaptor &adaptor, const char *table_name, const char *fixture_file, bool verbose)
{
  int rows = 0;
  yaml_event_t event;
  yaml_parser_t parser;
  yaml_parser_initialize(&parser);

  FILE *input = fopen(fixture_file, "rb");
  if( !input ) { fprintf(stderr, "Failed to open %s\n", fixture_file); return rows; }

  yaml_parser_set_input_file(&parser, input);

  int done = 0;
  std::string key, value;
  bool start_mapping = false;
  std::map<std::string,std::string> row;

  while( !done ) {
    if (!yaml_parser_parse(&parser, &event)) {
      fprintf(stderr, "fixtures from %s\n", fixture_file);
      return rows;
    }

    done = (event.type == YAML_STREAM_END_EVENT);

    switch(event.type) {
    case YAML_SEQUENCE_START_EVENT:
      //g_print("sequence start\n");
      break;
    case YAML_SEQUENCE_END_EVENT:
      //g_print("sequence end\n");
      break;
    case YAML_MAPPING_START_EVENT:
      row.clear();
      start_mapping = true;
      //g_print("mapping start\n");
      break;
    case YAML_SCALAR_EVENT:
      if( start_mapping ) {
        value.assign( (char*)event.data.scalar.value, event.data.scalar.length );
        if( event.start_mark.column == 2 ) {
          key = value;
        }
        else if( !key.empty() ) {
          row[key] = value;
          //g_print("key: '%s', value: '%s' %d, %d, %d\n", key.c_str(), value.c_str(), event.start_mark.index, event.start_mark.line, event.start_mark.column );
          key = "";
        }
      }
      break;
    case YAML_MAPPING_END_EVENT:
      start_mapping = false;
      //g_print("mapping end: %ld\n", row.size());
      if( !row.empty() ) { 
        // insert row
        gchar **keys = (gchar**)g_malloc(sizeof(gchar*) * (row.size() + 1) );
        gchar **values = (gchar**)g_malloc(sizeof(gchar*) * (row.size() + 1) );
        int i = 0;
        for( std::map<std::string, std::string>::iterator it = row.begin(); it != row.end(); ++it ) {
          keys[i] = g_strdup( it->first.c_str() );
          values[i] = g_strdup( ("'" + it->second + "'").c_str());
          ++i;
        }
        keys[i] = NULL;
        values[i] = NULL;
        gchar *key_str = g_strjoinv(",", keys);
        gchar *val_str = g_strjoinv(",", values);
        gchar *statement = g_strdup_printf("insert into `%s` (%s) values(%s)", table_name, key_str, val_str );

        if( verbose ) {
          g_print("%s\n", statement);
        }

        gint ret = adaptor.query(statement);
        if( ret != 0 ) {
          g_free(key_str);
          g_free(val_str);
          g_strfreev(keys);
          g_strfreev(values);
          g_free(statement);
          yaml_parser_delete(&parser);
          return rows;
        }

        ++rows;

        g_free(key_str);
        g_free(val_str);
        g_strfreev(keys);
        g_strfreev(values);
        g_free(statement);
      }
      row.clear();
      break;
    default:
      //g_print("event: %d\n", event.type);
      break;
    }

    yaml_event_delete(&event);
  }

  fclose(input);
  yaml_parser_delete(&parser);

  return rows;
}

bool create_jobs_table(Adaptor &ma, bool force)
{
  int status = 0;
  // drop the table
  status = ma.query("drop table if exists jobs ");

  // create the table
  // id, of the record
  // name, job record type
  // status, status of the job e.g. 'pending', 'processing', 'error', 'complete'
  // attempts, how many the job has run
  // locked_queue_id, the id of the worker queue that has locked the job for processing
  //                  process_id:thread_id:seconds:microseconds  -> seconds since Jan. 1, 1970
  // data, a JSON data structure to pass options to the job module referenced by name
  // details, text or log message of the final result
  // created_at, when the job was first created
  // updated_at, when the job was last updated
  status = ma.query("create table jobs ( id int(11) not null auto_increment,\
                                name varchar(255) NOT NULL,\
                                status varchar(255) NOT NULL,\
                                attempts int(11) default 0,\
                                locked_queue_id varchar(40) default '', \
                                data TEXT,\
                                details TEXT,\
                                created_at datetime,\
                                updated_at datetime,\
                              PRIMARY KEY  (id)\
                            ) ENGINE=InnoDB DEFAULT CHARSET=utf8");

  return (status == 0);
}

}
}
