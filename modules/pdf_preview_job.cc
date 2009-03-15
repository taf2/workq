#include "pdf_preview_job.h"
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

// define the pdf preview data object
struct PdfData {
  GObject parent_instance;
  gchar *original_filename;
  gchar *modified_filename;
};
struct PdfDataClass {
  GObjectClass parent_class;
};
enum PdfPropType {
  PROP_0,
  PDF_ORIGINAL_FILENAME,
  PDF_MODIFIED_FILENAME
};
#define PDF_TYPE_DATA (pdf_data_get_type ())
#define PDF_DATA(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PDF_TYPE_DATA, PdfData))
#define PDF_IS_DATA(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PDF_TYPE_DATA))
#define PDF_DATA_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PDF_TYPE_DATA, PdfDataClass))
#define PDF_IS_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), PDF_TYPE_DATA))
#define PDF_DATA_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PDF_TYPE_DATA, PdfDataClass))
G_DEFINE_TYPE (PdfData, pdf_data, G_TYPE_OBJECT);

static void pdf_data_init(PdfData*){}

static void
pdf_data_set_property(GObject      *object,
                      guint         property_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  PdfData *self = PDF_DATA(object);

  switch( property_id ) {
  case PDF_ORIGINAL_FILENAME:
    g_free(self->original_filename);
    self->original_filename = g_value_dup_string(value);
    break;
  case PDF_MODIFIED_FILENAME:
    g_free(self->modified_filename);
    self->modified_filename = g_value_dup_string(value);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}
static void
pdf_data_get_property(GObject      *object,
                      guint         property_id,
                      GValue *value,
                      GParamSpec   *pspec)
{
  PdfData *self = PDF_DATA(object);

  switch( property_id ) {
  case PDF_ORIGINAL_FILENAME:
    g_value_set_string(value, self->original_filename);
    break;
  case PDF_MODIFIED_FILENAME:
    g_value_set_string(value, self->modified_filename);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void pdf_data_class_init(PdfDataClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  // install property handlers
  gobject_class->set_property = pdf_data_set_property;
  gobject_class->get_property = pdf_data_get_property;

  pspec = g_param_spec_string( "original_filename", /* property name */
                               "path to original pdf",
                               "Path to original pdf",
                               "" /* default value empty string*/,
                               (GParamFlags)G_PARAM_READWRITE );

  g_object_class_install_property( gobject_class, PDF_ORIGINAL_FILENAME, pspec );

  pspec = g_param_spec_string( "modified_filename", /* property name */
                               "path to modified thumb",
                               "Path to modified thumb",
                               "" /* default value empty string*/,
                               (GParamFlags)G_PARAM_READWRITE );

  g_object_class_install_property( gobject_class, PDF_MODIFIED_FILENAME, pspec );
}

// exported symbols
G_MODULE_EXPORT 
WQ::Job *creator(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, WQ::DB::Adaptor *dbc)
{
  return new WQ::PdfPreviewJob(id,name,record, dbc);
}
G_MODULE_EXPORT 
bool initialize(const gchar *runpath)
{
  return true;
}
G_MODULE_EXPORT 
void finalize()
{
}

namespace WQ {

PdfPreviewJob::PdfPreviewJob(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, WQ::DB::Adaptor *dbc)
  : Job(id,name, record, dbc), m_data(0)
{
}
PdfPreviewJob::~PdfPreviewJob()
{
  if( m_data ) {
    g_object_unref(m_data);
  }
}

}
