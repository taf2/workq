#ifndef WQ_MYSQL_ADAPTOR_H
#define WQ_MYSQL_ADAPTOR_H

#include "adaptor.h"
#if defined(HAVE_MYSQL_H)
#include <mysql.h>
#elif defined(HAVE_MYSQL_MYSQL_H)
#include <mysql/mysql.h>
#else
#error "Must have at least one of mysql.h or mysql/mysql.h available!"
#endif

namespace WQ {
  namespace DB {
    struct MysqlAdaptor : public Adaptor {
      MysqlAdaptor();
      virtual ~MysqlAdaptor();

      static Adaptor* create();

      struct MyRow : public Row {
        MyRow(gchar buffer[], gint size, MYSQL *mysql, MYSQL_RES *result, MYSQL_ROW *row, int cols);
        virtual ~MyRow();
 
        virtual int columns()const;
        virtual ColType col_type(const char *name)const;

        // accessors for fields by column/field index
        virtual int int_field(const char *name)const;
        virtual double double_field(const char *name)const;
        virtual const char *str_field(const char *name)const;
      protected:
        struct Col {
          inline Col() : type(UNDEFINED) {
            memset((gpointer)&data,0,sizeof(Col::Data));
          }
          inline ~Col() {
            if( type == STRING ) { g_free(data.string_value); }
          }
          ColType type;
          union Data {
            gchar *string_value;
            glong fixed_value;
            gdouble numeric_value;
          };
          Data data;
          gint len;
          inline static void destroy_column(gpointer data) {  delete (Col*)data; }
        };
        Col* create_column(gchar buffer[], gint size, MYSQL_FIELD *field, const char *row_field);
        GHashTable *m_cols;
      };

      struct MysqlIterator : public Iterator {
        MysqlIterator(MYSQL *conn, gchar *query_buffer, gint size);
        virtual ~MysqlIterator();
        virtual Row *next();
        virtual Row *begin();
      protected:
        MYSQL *m_conn;
        MYSQL_RES *m_result;
        unsigned int m_fields;
        gchar *m_query_buffer;
        gint m_query_buffer_size;
      };

      virtual bool connect(const gchar *username,
                           const gchar *password,
                           const gchar *host,
                           const gchar *database,
                           int port = 3306,
                           const gchar *unix_socket = NULL); // TODO: support unix sockets??
      virtual void disconnect();
      virtual bool alive()const;
      virtual const char *details()const;
      virtual const char *errors()const;

      virtual int rows_affected();

      // execute a sql query
      virtual int query(const gchar *sql, ...);
      // execute a query and retrieve an array of rows
      virtual Iterator *query_with_result(const gchar *sql, ...);
    protected:
      MYSQL *m_mysql;
      gchar m_query_buffer[4098]; // max query length

      gint prepare_query(const gchar *sql, va_list args);
    };
  }
}

#endif
