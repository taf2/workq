#include <stdlib.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>


/* usual GObject boilerplate */
struct _FooObject {
  GObject parent_instance;
  /* instance members */
  int bar;
  gboolean baz;
  gchar *blah;
};

typedef struct _FooObject        FooObject;

struct _FooObjectClass {
  GObjectClass parent_class;
  /* class members */
};

typedef struct _FooObjectClass   FooObjectClass;

typedef enum {
  PROP_0,
  FOO_OBJECT_BAR,
  FOO_OBJECT_BAZ,
  FOO_OBJECT_BLAH,
} FooObjectPropType;

#define FOO_TYPE_OBJECT (foo_object_get_type ())
#define FOO_OBJECT(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), FOO_TYPE_OBJECT, FooObject))
#define FOO_IS_OBJECT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FOO_TYPE_OBJECT))
#define FOO_OBJECT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), FOO_TYPE_OBJECT, FooObjectClass))
#define FOO_IS_OBJECT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), FOO_TYPE_OBJECT))
#define FOO_OBJECT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), FOO_TYPE_OBJECT, FooObjectClass))


G_DEFINE_TYPE (FooObject, foo_object, G_TYPE_OBJECT);

static GObject *
foo_object_constructor( GType                  gtype,
                        guint                  n_properties,
                        GObjectConstructParam *properties)
{
  GObject *obj;
  GObjectClass *parent_class = G_OBJECT_CLASS(foo_object_parent_class);
  obj = parent_class->constructor (gtype, n_properties, properties);
  /* update the object state depending on constructor properties */
  return obj;
}

static void
foo_object_set_property(GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  FooObject *self = FOO_OBJECT(object);

  switch( property_id ) {
  case FOO_OBJECT_BAR:
    self->bar = g_value_get_int(value);
    break;
  case FOO_OBJECT_BAZ:
    self->baz = g_value_get_boolean(value);
    break;
  case FOO_OBJECT_BLAH:
    g_free(self->blah);
    self->blah = g_value_dup_string (value);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}
static void
foo_object_get_property(GObject      *object,
                        guint         property_id,
                        GValue *value,
                        GParamSpec   *pspec)
{
  FooObject *self = FOO_OBJECT(object);

  switch( property_id ) {
  case FOO_OBJECT_BAR:
    g_value_set_int(value,self->bar);
    break;
  case FOO_OBJECT_BAZ:
    g_value_set_boolean(value,self->baz);
    break;
  case FOO_OBJECT_BLAH:
    g_value_set_string (value, self->blah);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void foo_object_class_init(FooObjectClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor  = foo_object_constructor;

  // install property handlers
  gobject_class->set_property = foo_object_set_property;
  gobject_class->get_property = foo_object_get_property;

  pspec = g_param_spec_string( "blah", /* property name */
                               "blah value",
                               "Blah string value property",
                               "" /* default value empty string*/,
                               (GParamFlags)G_PARAM_READWRITE );

  g_object_class_install_property( gobject_class, FOO_OBJECT_BLAH, pspec );

  pspec = g_param_spec_int( "bar",
                            "bar number value",
                            "Set/Get bar's number",
                            0  /* minimum value */,
                            80 /* maximum value */,
                            2  /* default value */,
                            (GParamFlags)G_PARAM_READWRITE );
  g_object_class_install_property( gobject_class, FOO_OBJECT_BAR, pspec);

  pspec = g_param_spec_boolean( "baz",
                                "baz truth value",
                                "Set/Get baz",
                                FALSE,
                                (GParamFlags)G_PARAM_READWRITE );
  g_object_class_install_property( gobject_class, FOO_OBJECT_BAZ, pspec);
}

static void foo_object_init( FooObject *self )
{
}


static const gchar *foo_object_json = " \
{ \
  \"bar\"  : 42, \
  \"baz\"  : true, \
  \"blah\" : \"bravo\" \
} \
";

int main (int argc, char **argv)
{
  g_type_init ();
  for( int j = 0; j < 1; ++j ) {
    FooObject *foo;
    gint bar_int;
    gboolean baz_boolean;
    gchar *blah_str = NULL;
    GError *error;


    error = NULL;
    foo = (FooObject*)json_construct_gobject (FOO_TYPE_OBJECT, foo_object_json, -1, &error);
    if (error)
      g_error ("Unable to create instance: %s", error->message);

    for( int i = 0; i < 1; ++i ) {
      /* FooObject has three properties: bar, blah and baz */
      g_object_get (G_OBJECT (foo),
                    "bar", &bar_int,
                    "baz", &baz_boolean,
                    "blah", &blah_str,
                    NULL);

      g_print("bar: %d, baz: %s, blah: '%s'\n", bar_int, baz_boolean ? "true" : "false", blah_str);
      g_free(blah_str);
    }
    g_object_unref (foo);
  }

  return EXIT_SUCCESS;
}
