#include "adaptor.h"

namespace WQ {
namespace DB {

GData *Adaptor::wq_adaptor_set = NULL;

void Adaptor::initialize()
{
  g_datalist_init(&wq_adaptor_set);
}
void Adaptor::finalize()
{
  g_datalist_clear(&wq_adaptor_set);
}
void Adaptor::register_adaptor(const char *name, adaptor_creator_t creator)
{
  GQuark key = g_quark_from_string(name);
  g_datalist_id_set_data(&wq_adaptor_set, key, (gpointer)creator );
}

Adaptor *Adaptor::create(const char *ats)
{
  if( !wq_adaptor_set ) { return NULL; }
  GQuark key = g_quark_from_string(ats);

  adaptor_creator_t creator_func = (adaptor_creator_t)g_datalist_id_get_data(&wq_adaptor_set, key);

  if( creator_func ) { return creator_func(); } else { return NULL; }
}

Adaptor::Iterator::Iterator()
{
}

Adaptor::Iterator::~Iterator()
{
}

Adaptor::Adaptor(){}
Adaptor::~Adaptor(){}
Adaptor::Row::Row(int){}
Adaptor::Row::~Row(){}

}
}
