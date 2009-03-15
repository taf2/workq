#include <glib.h>
#include <stdlib.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

static void print_json(JsonNode *node);

static int print_json_from_file(const gchar *file);

int main(int argc, char **argv)
{
  if( argc < 2 ) {
    g_print ("Usage: test <filename.json>\n");
    return EXIT_FAILURE;
  }

  g_type_init();

  int r = print_json_from_file(argv[1]);
  r += print_json_from_file(argv[1]);
  r += print_json_from_file(argv[1]);
  r += print_json_from_file(argv[1]);
  return r;
}

static int print_json_from_file(const gchar *file)
{
  JsonParser *parser = NULL;
  JsonNode *root = NULL;
  GError *error = NULL;
  parser = json_parser_new ();

  error = NULL;
  json_parser_load_from_file (parser, file, &error);

  if( error ) {
    g_print ("Unable to parse `%s': %s\n", file, error->message);
    g_error_free (error);
    g_object_unref (parser);
    return EXIT_FAILURE;
  }

  root = json_parser_get_root (parser);

  print_json(root);

  /* manipulate the object tree and then exit */
  g_object_unref (parser);

  return EXIT_SUCCESS;
}

static void print_json_object(JsonObject *object)
{
  GList *next = NULL;
  GList *members = NULL;
  next = members = json_object_get_members(object);
  g_print("{");
  while( next ) {
    g_print("%s: ", (const gchar*)next->data );
    JsonNode *node = json_object_get_member(object,(const gchar*)next->data);
    print_json(node);
    next = next->next;
    if( next ) {  g_print(", "); }
  }
  g_print("}");
  g_list_free(members);
}

static void print_json_array(JsonArray *array)
{
  guint size = json_array_get_length(array) - 1;
  g_print("[\n");
  for( guint i = 0; i < size; ++i ) {
    print_json(json_array_get_element(array,i));
    g_print(",\n");
  }
  print_json(json_array_get_element(array,size));
  g_print("\n]");
}

static void print_json(JsonNode *node)
{
  switch( JSON_NODE_TYPE(node) ) {
  case JSON_NODE_OBJECT:
    print_json_object(json_node_get_object(node));
    break;
  case JSON_NODE_ARRAY:
    print_json_array(json_node_get_array(node));
    break;
  case JSON_NODE_VALUE:
    switch(json_node_get_value_type(node)) {
    case G_TYPE_BOOLEAN: {
      gboolean value = json_node_get_boolean(node);
      g_print("%s", value ? "true" : "false" );
      } break;
    case G_TYPE_INT:
    case G_TYPE_LONG:
      g_print("%d", json_node_get_int(node));
      break;
    case G_TYPE_UINT:
    case G_TYPE_ULONG:
      g_print("%d", json_node_get_int(node));
      break;
    case G_TYPE_ENUM:
      break;
    case G_TYPE_STRING:
      g_print("\"%s\"", json_node_get_string(node));
      break;
    case G_TYPE_FLOAT:
    case G_TYPE_DOUBLE:
      g_print("%f", json_node_get_double(node));
      break;
    case G_TYPE_POINTER:
    case G_TYPE_BOXED:
    case G_TYPE_PARAM:
    case G_TYPE_OBJECT:
    default:
      g_print("unsupported type!\n");
      break;
    }
    break;
  case JSON_NODE_NULL:
    g_print("null");
    break;
  default:
    g_print("node: unknown\n");
    break;
  }
}
