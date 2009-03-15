#ifndef WQ_STRING_PLUS_H
#define WQ_STRING_PLUS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <glib.h>
#include <string.h>

// C++ style accessor for GString
namespace WQ {
  struct PString {
    PString();
    PString( const gchar *str );
    PString( const gchar *str, gssize size );
    PString( const PString &str );
    ~PString();

    PString& operator=(const PString &str);
    PString& operator+=(const PString &str);

    inline operator GString*(){return this->m_string;}
    inline operator const GString*()const{return this->m_string;}
 
    inline operator gchar*(){ return this->m_string->str; }
    inline operator const gchar*()const{ return this->m_string->str; }

  protected:
    GString *m_string;
  };

  inline PString operator+(const PString &str1, const PString &str2){ PString s = str1; s += str2; return s; }

  inline bool operator==(const PString &str1, const PString &str2) {
    return g_string_equal(static_cast<const GString*>(str1),static_cast<const GString*>(str2));
  }

  inline bool operator==(const PString &str1, const gchar *str2) {
    return !strcmp(str1,str2);
    //return g_string_equal(static_cast<const GString*>(str1),static_cast<const GString*>(str2));
  }

  inline bool operator!=(const PString &str1, const PString &str2) {
    return !g_string_equal(static_cast<const GString*>(str1),static_cast<const GString*>(str2));
  }
}


#endif
