#ifndef WQ_JOB_H
#define WQ_JOB_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <glib.h>
#include "settings.h"
#include "adaptor.h"

namespace WQ {
  // base class for processing jobs
  // There will be different classes of jobs derived from this base class
  struct Job {
    virtual ~Job();

    // execute the job, performs whatever task has been assigned, may trigger other jobs
    virtual int execute() = 0;

    // called by worker process to run the job, essentially a wrapper around execute that
    // first decides whether load_data needs to be called or not
    int real_execute();

    // access the job id
    inline int id()const{ return this->m_id; }
    // access the job record for this record
    inline WQ::DB::Adaptor::Row *record() { return this->m_record; }
    inline const gchar *name()const{ return this->m_name; }
  protected:
    // Job takes ownership of record
    // Job has access to db connection
    // id is the extracted job id
    // name is the extracted job name
    Job(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, DB::Adaptor *dbc);

    virtual bool load_data() = 0;

    gint m_id;
    gchar *m_name;
    WQ::DB::Adaptor::Row *m_record;
    DB::Adaptor          *m_db;
  };

}
// exported symbols from jobs
typedef struct WQ::Job*(*job_creator_t)(gint, const gchar*, WQ::DB::Adaptor::Row *, WQ::DB::Adaptor *);
typedef bool (*job_initialize_t)(const gchar*);
typedef void (*job_finalize_t)();



#endif
