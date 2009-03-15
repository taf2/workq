#ifndef WQ_RUN_JOB_H
#define WQ_RUN_JOB_H

#include "workq/job.h"
#include <gmodule.h>

extern "C" {
  // exported symbols
  G_MODULE_IMPORT struct WQ::Job *creator(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, WQ::DB::Adaptor *dbc);
  G_MODULE_IMPORT bool initialize(const gchar *runpath);
  G_MODULE_IMPORT void finalize();
}

namespace WQ {
  struct RunJob : public Job {
    RunJob(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, WQ::DB::Adaptor *dbc);
    virtual ~RunJob();
    virtual int execute();

  protected:
    virtual bool load_data();
  protected:
    struct RunData *m_data;
  };
}

#endif
