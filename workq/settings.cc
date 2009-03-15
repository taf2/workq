#include "settings.h"
#include <yaml.h>

namespace WQ {
  // 2 config tables
  // 1 that is in active use
  // 1 that is is being loaded e.g. in the event of a SIGHUP
  // only if the keys are valid should it be committed

Settings::Settings(const gchar *binpath, const gchar *environment)
  // hash table of GQuark Keys, 
  : m_binpath(0), m_env(0), m_table(0), m_loaded(0)
{
  g_datalist_init( &m_table );
  if( g_path_is_absolute(binpath) ) {
    m_binpath = g_path_get_dirname(binpath);
  }
  else {
    gchar *cpath = g_get_current_dir();
    gchar *dir = g_path_get_dirname(binpath);
    m_binpath = g_build_path("/", cpath, dir, NULL);
    g_free(cpath);
    g_free(dir);
  }
  m_env = g_strdup(environment);
}
Settings::~Settings()
{
  g_free(m_binpath);
  g_free(m_env);
  g_datalist_clear( &m_table );
  if( m_loaded ) {
    g_datalist_clear( &m_loaded );
  }
}

// load config file at filepath into memory
bool Settings::load_from_file(const gchar *config_path)
{
  // create a temporary table
  bool ret = true;
  FILE *input = fopen(config_path, "rb");
  if( !input ) { g_message("Failed to open %s\n", config_path); return false; }

  g_datalist_init( &m_loaded );

  yaml_event_t event;
  yaml_parser_t parser;
  yaml_parser_initialize(&parser);

  yaml_parser_set_input_file(&parser, input);

  int done = 0;
  int count = 0;
  bool use_keys = false;

  GQuark env_key = g_quark_from_string(m_env);
  GQuark key = 0;

  while( !done ) {
    if (!yaml_parser_parse(&parser, &event)) {
      fprintf(stderr, "Failed to load configuration file\n");
      break;
    }

    done = (event.type == YAML_STREAM_END_EVENT);

    //printf("event type:%d\n", (int)event.type);

    switch(event.type) {
    case YAML_SCALAR_EVENT:
      /*printf("scalar event: {anchor: %c tag: %c value: %s, length: %d, plain_implicit: %d, quoted_implicit: %d, type: %d}\n",
             event.data.scalar.anchor ? (char)*event.data.scalar.anchor : 'n',
             event.data.scalar.tag ? (char)*event.data.scalar.tag : 'n',
             event.data.scalar.value,
             event.data.scalar.length,
             event.data.scalar.plain_implicit,
             event.data.scalar.quoted_implicit,
             (int)event.data.scalar.style );*/
      ++count;
      if( count % 2 ) { // modulate key to value
        gchar *v = g_strndup((char*)event.data.scalar.value, event.data.scalar.length);
        //printf("%s:", v);
        key = g_quark_from_string(v);
        g_free(v);
      }
      else if( key != 0 ) {
        if( use_keys ) {
          gchar *v = g_strndup((char*)event.data.scalar.value, event.data.scalar.length);
          g_datalist_id_set_data_full( &m_loaded, key, v, g_free );
          key = 0;
          //printf(" %s", v);
        }
        //printf("\n");
      }
      break;
    case YAML_SEQUENCE_START_EVENT:
      break;
    case YAML_SEQUENCE_END_EVENT:
      break;
    case YAML_MAPPING_START_EVENT:
      //printf("\n");
      if( key ) { use_keys = !strcmp(g_quark_to_string(key), m_env); }
      count = 0; // reset
      break;
    case YAML_MAPPING_END_EVENT:
      //printf("\n");
      break;
    default:
      break;
    }

    yaml_event_delete(&event);
  }

  fclose(input);
  yaml_parser_delete(&parser);

  return ret;
}

// structure to make it easier to verify key values correctness/sanity
struct Verify {
  Verify() {
    adapterkey  = g_quark_from_string("adapter");
    dbkey       = g_quark_from_string("database");
    userkey     = g_quark_from_string("username");
    passkey     = g_quark_from_string("password");
    poolkey     = g_quark_from_string("pool");
    hostkey     = g_quark_from_string("host");
    portkey     = g_quark_from_string("port");
    workerskey  = g_quark_from_string("workers");
    listenkey   = g_quark_from_string("listen");
    pollratekey = g_quark_from_string("pollrate");
    logfilekey  = g_quark_from_string("logfile");
    pidfilekey  = g_quark_from_string("pidfile");
    valid       = true;
  }
  void test_key_value(GQuark key, const gchar *value)
  {
    //printf("test %ld, (%s) -> %s\n", (long)key, g_quark_to_string(key), value);
    // to verify db info we need to test connect to the db...
    if( key == adapterkey ) {
      adapter = value;
    }
    else if( key == dbkey ) {
      database = value;
    }
    else if( key == userkey ) {
      username = value;
    }
    else if( key == passkey ) {
      password = value;
    }
    else if( key == poolkey ) { 
      int v = atoi(value);
      valid = (v > 0);
    }
    else if( key == hostkey ) {
      hostname = value;
    }
    else if( key == portkey ) {
      port = atoi(value);
      valid = (port > 0);
    }
    else if( key == workerskey ) {
      int v = atoi(value);
      valid = (v > 0);
    }
    else if( key == listenkey ) {
      int v = atoi(value);
      valid = (v > 0);
    }
    else if( key == pollratekey ) {
      int v = atoi(value);
      valid = (v > 0);
    }
    else if( key == logfilekey ) {
      valid = g_file_test(value,G_FILE_TEST_EXISTS);
    }
    else if( key == pidfilekey ) {
      gchar *dir = g_path_get_dirname(value);
      valid = g_file_test(dir,(GFileTest)(G_FILE_TEST_EXISTS|G_FILE_TEST_IS_DIR));
      g_free(dir);
    }
    else {
      valid = false;
    }
  }
  bool final_confirm()
  {
    if( valid ) {
      // using collected credientials try to connect to database
    }
    return valid;
  }
  GQuark adapterkey;
  GQuark dbkey;
  GQuark userkey;
  GQuark passkey;
  GQuark poolkey;
  GQuark hostkey;
  GQuark portkey;
  GQuark workerskey;
  GQuark listenkey;
  GQuark pollratekey;
  GQuark logfilekey;
  GQuark pidfilekey;
  bool valid;
  const gchar *adapter;
  const gchar *database;
  const gchar *username;
  const gchar *password;
  const gchar *hostname;
  gint port;
};

void test_key_value(GQuark key, gpointer value, gpointer valid)
{
  gchar *svalue = (gchar*)value;
  Verify *check = static_cast<Verify*>(valid);
  check->test_key_value(key, svalue);
}

// return true if all configuration keys are valid
bool Settings::verify()const
{
  // loop over each key and verify it's value
  Verify check;
  g_datalist_foreach( (GData**)&m_loaded, test_key_value, (gpointer)&check);
  return check.final_confirm();
}

// commit configuration into memory
void Settings::commit()
{
  g_datalist_clear( &m_table );
  m_table = m_loaded;
  m_loaded = NULL; // mark it as unused
}

const gchar *Settings::get_str(GQuark key, const gchar *default_value)const
{
  GData *t = const_cast<GData*>(m_table);
  const gchar *v = (const gchar*)g_datalist_id_get_data(&t, key);
  if( v ){ return v; }
  return default_value;
}
const gchar *Settings::get_str(const gchar *key, const gchar *default_value)const
{
  GData *t = const_cast<GData*>(m_table);
  GQuark qkey = g_quark_from_string(key);
  const gchar *v = (const gchar*)g_datalist_id_get_data(&t, qkey);
  if( v ){ return v; }
  return default_value;
}

gint Settings::get_int(GQuark key, gint default_value)const
{
  GData *t = const_cast<GData*>(m_table);
  const gchar *value = (const gchar*)g_datalist_id_get_data(&t, key);
  if( value ){ return atoi(value); }
  return default_value;
}
gint Settings::get_int(const gchar *key, gint default_value)const
{
  GData *t = const_cast<GData*>(m_table);
  GQuark qkey = g_quark_from_string(key);
  const gchar *value = (const gchar*)g_datalist_id_get_data(&t, qkey);
  if( value ){ return atoi( value ); }
  return default_value;
}

gdouble Settings::get_double(GQuark key, gdouble default_value)const
{
  GData *t = const_cast<GData*>(m_table);
  const gchar *value = (const gchar*)g_datalist_id_get_data(&t, key);
  if( value ) { return atof(value); }
  return default_value;
}
gdouble Settings::get_double(const gchar *key, gdouble default_value)const
{
  GData *t = const_cast<GData*>(m_table);
  GQuark qkey = g_quark_from_string(key);
  const gchar *value = (const gchar*)g_datalist_id_get_data(&t, qkey);
  if( value ) { return atof( value ); }
  return default_value;
}
bool Settings::get_bool(GQuark key, bool default_value)const
{
  GData *t = const_cast<GData*>(m_table);
  const gchar *value = (const gchar*)g_datalist_id_get_data(&t, key);
  if( value ) {
    return (!strcmp("yes", value) || !strcmp("true", value));
  }
  return default_value;
}
bool Settings::get_bool(const gchar *key, bool default_value)const
{
  GData *t = const_cast<GData*>(m_table);
  GQuark qkey = g_quark_from_string(key);
  const gchar *value = (const gchar*)g_datalist_id_get_data(&t, qkey);
  if( value ) {
    return (!strcmp("yes", value) || !strcmp("true", value));
  }
  return default_value;
}

}
