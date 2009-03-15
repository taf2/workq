#ifndef WQ_SETTINGS_H
#define WQ_SETTINGS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <glib.h>

namespace WQ {
  struct Settings {
    Settings(const char *binpath, const gchar *environment = "development");
    ~Settings();

    // load config file at filepath into memory
    bool load_from_file(const gchar *filepath);

    // return true if all configuration keys are valid
    bool verify()const;

    // commit configuration into memory
    void commit();

    const gchar *get_str(GQuark key, const gchar *default_value = NULL)const;
    const gchar *get_str(const gchar *key, const gchar *default_value = NULL)const;

    gint get_int(GQuark key, gint default_value = 0)const;
    gint get_int(const gchar *key, gint default_value = 0)const;
 
    gdouble get_double(GQuark key, gdouble default_value = 0.0)const;
    gdouble get_double(const gchar *key, gdouble default_value = 0.0)const;
    
    bool get_bool(GQuark key, bool default_value = false)const;
    bool get_bool(const gchar *key, bool default_value = false)const;

    const gchar *get_binpath()const{ return m_binpath; }

    void set_env(const gchar *env) { g_free(m_env); m_env = g_strdup(env); }
    const gchar *get_env()const{ return m_env; }
  protected:
    gchar *m_binpath;
    gchar *m_env;
    GData *m_table, *m_loaded;
  };
}

#endif
