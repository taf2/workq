#ifndef WQ_ADAPTOR_H
#define WQ_ADAPTOR_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <glib.h>
#include <yaml.h>

namespace WQ {
  namespace DB {
    // db adaptor, provides access to mysqld
    struct Adaptor {
      static GData *wq_adaptor_set;

      typedef Adaptor*(*adaptor_creator_t)();

      static void initialize();
      static void finalize();
      static void register_adaptor(const char *name, adaptor_creator_t creator);
      static Adaptor* create(const char *adapter_type_string); 

      enum ColType {
        FIXED,
        NUMERIC,
        STRING,
        UNDEFINED
      };

      struct Row {
        virtual ~Row();

        virtual int columns()const = 0;

        virtual ColType col_type(const char *name)const = 0;

        // accessors for fields by column/field index
        virtual int int_field(const char *name)const = 0;
        virtual double double_field(const char *name)const = 0;
        virtual const char *str_field(const char *name)const = 0;
      protected:
        Row(int cols);
      };

      // allow iteration over a result set
      //
      // Iterator *it = adaptor->query_with_result("select * from table",it);
      //
      // if( !it ) {
      //   // something went wrong check sql syntax
      // }
      //
      // for( Row *row = it->begin(); row; row = it->next() ) {
      //   // use the row
      //   for( int i = 0; i < row->columns(); ++i ) {
      //     switch(row->col_type(i) ) {
      //     case  FIXED:
      //       printf("%d ", row->int_field(i) );
      //       break;
      //     case  NUMERIC:
      //       printf("%f ", row->double_field(i) );
      //       break;
      //     case  STRING:
      //       printf("%s ", row->str_field(i) );
      //       break;
      //     }
      //   }
      //   delete row;
      //   printf("\n");
      // }
      //
      // delete it;
      //
      struct Iterator {
        virtual ~Iterator();
        virtual Row *next() = 0;
        virtual Row *begin() = 0;
      protected:
        Iterator();
      };


      virtual ~Adaptor();

      virtual bool connect(const gchar *username,
                           const gchar *password,
                           const gchar *host,
                           const gchar *database,
                           int port = 3306,
                           const gchar *unix_socket=NULL)= 0;

      virtual void disconnect() = 0;

      // test whether the connection is still alive
      virtual bool alive()const = 0;
      // details about last query
      virtual const char *details()const = 0;
      virtual const char *errors()const = 0;

      // call after query to determine how many rows changed
      virtual int rows_affected() = 0;

      // execute a sql query
      virtual int query(const gchar *sql, ...) = 0;
      // execute a query and retrieve an array of rows
      virtual Iterator *query_with_result(const gchar *sql, ...) = 0;
    protected:
      Adaptor();
    };
  }
}

#endif
