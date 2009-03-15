#include "pstring.h"
namespace WQ {

PString::PString()
  : m_string(g_string_new(""))
{
}

PString::PString( const gchar *str )
  : m_string(g_string_new(str))
{
}
PString::PString( const gchar *str, gssize size )
  : m_string(g_string_new_len(str,size))
{
}
PString::PString( const PString &str )
  : m_string(g_string_sized_new(static_cast<const GString*>(str)->len))
{
  *this = str;
}
PString::~PString()
{
  g_string_free(m_string,true);
}

PString& PString::operator=(const PString &str)
{
  m_string = g_string_assign(m_string, static_cast<const GString*>(str)->str);
  return *this;
}
PString& PString::operator+=(const PString &str)
{
   m_string = g_string_append_len(m_string, static_cast<const GString*>(str)->str, static_cast<const GString*>(str)->len );
   return *this;
}

}
