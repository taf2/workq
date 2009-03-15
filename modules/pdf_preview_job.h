#ifndef WQ_PDF_PREVIEW_JOB_H
#define WQ_PDF_PREVIEW_JOB_H

#include "workq/job.h"
#include <gmodule.h>

extern "C" {
// exported symbols
G_MODULE_IMPORT struct WQ::Job *creator(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, WQ::DB::Adaptor *dbc);
G_MODULE_IMPORT bool initialize(const gchar *runpath);
G_MODULE_IMPORT void finalize();
}

// uses libpoppler to generate a preview of the pdf
namespace WQ {
  struct PdfPreviewJob : public Job {
    PdfPreviewJob(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, WQ::DB::Adaptor *dbc);
    virtual ~PdfPreviewJob();
    virtual int execute();

  protected:
    virtual bool load_data();

    bool generate_preview();
  protected:
    struct PdfData *m_data;
  };
}

#endif
