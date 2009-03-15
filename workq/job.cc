#include "job.h"

namespace WQ {

Job::Job(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, DB::Adaptor *dbc)
  : m_id(id), m_name(g_strdup(name)), m_record(record), m_db(dbc)
{
}

Job::~Job()
{
  g_free(m_name);
  if( m_record ) {
    delete m_record;
  }
  m_db = NULL;
}

int Job::real_execute() 
{
  if( this->load_data() ) {
    // can do other job initialization e.g. loading the job record
    return this->execute();
  }
  return 0;
}

}
