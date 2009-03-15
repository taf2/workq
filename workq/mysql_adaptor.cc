#include "mysql_adaptor.h"

namespace WQ {
namespace DB {
      
  
Adaptor* MysqlAdaptor::create()
{
  return new MysqlAdaptor();
}

MysqlAdaptor::MysqlAdaptor()
  : Adaptor(), m_mysql(0)
{
}
MysqlAdaptor::~MysqlAdaptor()
{
  this->disconnect();
}
MysqlAdaptor::MyRow::Col* 
MysqlAdaptor::MyRow::create_column(gchar buffer[], gint size, MYSQL_FIELD *field, const char *row_field)
{
  Col *col = new Col;
  g_snprintf(buffer,size,"%.*s", field->length, row_field );
  //g_message("%s(%d) => [%.*s] ", field->name, field->type, field->length, row_field );
  switch(field->type) {
  case MYSQL_TYPE_DECIMAL:
  case MYSQL_TYPE_NEWDECIMAL:
  case MYSQL_TYPE_TINY:
  case MYSQL_TYPE_SHORT:
  case MYSQL_TYPE_LONG:
  case MYSQL_TYPE_LONGLONG:
  case MYSQL_TYPE_INT24:
    col->type = FIXED;
    col->data.fixed_value = atol(buffer);
    //g_message("data fixed: %ld\n", col->data.fixed_value);
    break;
  case MYSQL_TYPE_FLOAT:
  case MYSQL_TYPE_DOUBLE:
    col->type = NUMERIC;
    col->data.numeric_value = atof(buffer);
   // g_message("data floating: %f\n", col->data.numeric_value );
    break;
  case MYSQL_TYPE_NULL:
    break;
  case MYSQL_TYPE_TIMESTAMP:
  case MYSQL_TYPE_DATE:
  case MYSQL_TYPE_TIME:
  case MYSQL_TYPE_DATETIME:
  case MYSQL_TYPE_NEWDATE:
    //g_message("datetime");
    break;
  case MYSQL_TYPE_YEAR:
    //g_message("year");
    break;
  case MYSQL_TYPE_ENUM:
    //g_message("enum");
    break;
  case MYSQL_TYPE_SET:
    //g_message("set");
    break;
  case MYSQL_TYPE_TINY_BLOB:
  case MYSQL_TYPE_MEDIUM_BLOB:
  case MYSQL_TYPE_LONG_BLOB:
  case MYSQL_TYPE_BLOB:
  case MYSQL_TYPE_VAR_STRING:
  case MYSQL_TYPE_STRING:
    //g_message("string blob");
    col->type = STRING;
    col->data.string_value = strdup(buffer);
    break;
  case MYSQL_TYPE_GEOMETRY:
  case MYSQL_TYPE_BIT:
    //g_message("geometry?");
  default:
    break;
  }
  return col;
}

MysqlAdaptor::MyRow::MyRow(gchar buffer[], gint size, MYSQL *mysql, MYSQL_RES *result, MYSQL_ROW *row, int cols)
  : Row(cols)
{
  MYSQL_FIELD *field;
  m_cols = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, MyRow::Col::destroy_column);

  for(int i = 0; i < cols; ++i) {
    field = mysql_fetch_field_direct(result, i);
    Col *col = create_column(buffer,size,field,(*row)[i]);
    //g_message("add col: %s\n", field->name);
    g_hash_table_insert(m_cols, g_strdup(field->name), col);
  }
}

MysqlAdaptor::MyRow::~MyRow()
{
	g_hash_table_destroy(m_cols);
}

int MysqlAdaptor::MyRow::columns()const
{
  return g_hash_table_size(m_cols);
}
Adaptor::ColType MysqlAdaptor::MyRow::col_type(const char *col)const
{
  Col *c = (Col*)g_hash_table_lookup(m_cols, col);
  if( c ) {
    return c->type;
  }
  return UNDEFINED;
}

// accessors for fields by column/field index
int MysqlAdaptor::MyRow::int_field(const char *col)const
{
  Col *c = (Col*)g_hash_table_lookup(m_cols, col);
  if( c ) {
    return c->data.fixed_value;
  }
  return 0;
}
double MysqlAdaptor::MyRow::double_field(const char *col)const
{
  Col *c = (Col*)g_hash_table_lookup(m_cols, col);
  if( c ) {
    return c->data.numeric_value;
  }
  return 0;
}
const char *MysqlAdaptor::MyRow::str_field(const char *col)const
{
  Col *c = (Col*)g_hash_table_lookup(m_cols, col);
  if( c ) {
    if( c->type != STRING ) { return NULL; }
    return c->data.string_value;
  }
  return NULL;
}
bool MysqlAdaptor::connect(const gchar *username,
                           const gchar *password,
                           const gchar *host,
                           const gchar *database,
                           int port,
                           const gchar *unix_socket)
{
  my_bool reconnect = 1;
  m_mysql = (MYSQL*)g_malloc(sizeof(MYSQL));

  if( !m_mysql ) {
    g_critical("failed to allocate memory for mysql object\n");
    return false;
  }

  if( !mysql_init(m_mysql) ) {
    g_critical("failed initialize mysql engine\n");
    goto conn_error;
  }

  if( !mysql_real_connect(m_mysql, host, username, password, database, port, unix_socket,0) ) {
    g_critical("failed to connect to mysql server\n");
    goto conn_error;
  }

  // tell mysql to try and reconnect if the connection is lost
  mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &reconnect);

  return true;
conn_error:
  if( m_mysql ) {
    g_free(m_mysql);
    m_mysql = 0;
  }
  return false;
}
void MysqlAdaptor::disconnect()
{
  if( m_mysql ) {
    mysql_close(m_mysql);
    g_free(m_mysql);
  }
}
bool MysqlAdaptor::alive()const
{
  return m_mysql && !mysql_ping(m_mysql);
}
const char *MysqlAdaptor::details()const
{
  if( m_mysql ) { return mysql_info(m_mysql); } else { return NULL; }
}
const char *MysqlAdaptor::errors()const
{
  if( m_mysql ) { return mysql_error(m_mysql); } else { return NULL; }
}

#define prepare(sql)\
  va_list ap;\
  va_start( ap, sql );\
  gint size = prepare_query(sql,ap);\
  va_end(ap);

// loads formatted query string into query_buffer
gint MysqlAdaptor::prepare_query(const gchar *sql, va_list args)
{
  // XXX: looking into things... it would be very difficult yet ideal if we could 
  // escape strings as they are passed in this method...
  //gchar **g_strsplit(sql,"%s",0);
  //Arguments a;
  //g_printf_fetchargs(args, &a);
  gint size = g_vsnprintf( m_query_buffer, sizeof(m_query_buffer), sql, args );
  //g_print("query: %s\n", m_query_buffer);
  return size;
}

int MysqlAdaptor::rows_affected()
{
  return mysql_affected_rows(m_mysql);
}

// execute a sql query
int MysqlAdaptor::query(const gchar *sql, ...)
{
  if( !m_mysql ) { return -1; }
  prepare(sql);
  //g_message("execute(%ld): %s", (glong)m_mysql, m_query_buffer);
  return mysql_real_query(m_mysql, m_query_buffer, size ); 
}
// execute a query and retrieve an array of rows
MysqlAdaptor::Iterator *MysqlAdaptor::query_with_result(const gchar *sql, ...)
{
  if( !m_mysql ) { g_critical("Database not connected!"); return NULL; }
  prepare(sql);
  //g_message("execute(%ld): %s", (glong)m_mysql, m_query_buffer);
  int r = mysql_real_query(m_mysql, m_query_buffer, size );
  if( r ) {
    g_critical("Failed to execute query" );
    return NULL;
  }

  return new MysqlIterator(m_mysql, m_query_buffer, sizeof(m_query_buffer));
}
MysqlAdaptor::MysqlIterator::MysqlIterator(MYSQL *conn, gchar *query_buffer, gint size)
  : Iterator(), m_conn(conn), m_result(0), m_fields(-1), m_query_buffer(query_buffer), m_query_buffer_size(size)
{
}
MysqlAdaptor::MysqlIterator::~MysqlIterator()
{
  if( m_result ) {
    mysql_free_result(m_result);
  }
}
MysqlAdaptor::Row *MysqlAdaptor::MysqlIterator::next()
{
  MYSQL_ROW row = mysql_fetch_row(m_result);
  if( row ) {
    return new MyRow(m_query_buffer, m_query_buffer_size, m_conn, m_result, &row, m_fields);
  }
  return NULL;
}
MysqlAdaptor::Row *MysqlAdaptor::MysqlIterator::begin()
{
  // the iterator should call this
  m_result = mysql_store_result(m_conn);
  if( !m_result ) { return NULL; }
  m_fields = mysql_field_count(m_conn);
  return this->next();
}

} // end namespace DB
} // end namespace WQ
