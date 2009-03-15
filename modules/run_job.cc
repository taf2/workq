#include "run_job.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

// define the run data object
struct RunData {
  GObject parent_instance;
  gchar *command; // path to the binary to run e.g. ruby script.rb
};
struct RunDataClass {
  GObjectClass parent_class;
};
enum RunPropType {
  PROP_0,
  RUN_COMMAND
};
#define RUN_TYPE_DATA (run_data_get_type ())
#define RUN_DATA(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), RUN_TYPE_DATA, RunData))
#define RUN_IS_DATA(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), RUN_TYPE_DATA))
#define RUN_DATA_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), RUN_TYPE_DATA, RunDataClass))
#define RUN_IS_DATA_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), RUN_TYPE_DATA))
#define RUN_DATA_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), RUN_TYPE_DATA, RunDataClass))
G_DEFINE_TYPE (RunData, run_data, G_TYPE_OBJECT);
static void run_data_init(RunData*){}

static void
run_data_set_property(GObject      *object,
                      guint         property_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  RunData *self = RUN_DATA(object);

  switch( property_id ) {
  case RUN_COMMAND:
    g_free(self->command);
    self->command = g_value_dup_string(value);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}
static void
run_data_get_property(GObject      *object,
                      guint         property_id,
                      GValue *value,
                      GParamSpec   *pspec)
{
  RunData *self = RUN_DATA(object);

  switch( property_id ) {
  case RUN_COMMAND:
    g_value_set_string(value, self->command);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void run_data_class_init(RunDataClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  // install property handlers
  gobject_class->set_property = run_data_set_property;
  gobject_class->get_property = run_data_get_property;

  pspec = g_param_spec_string( "command", /* property name */
                               "path to executable command",
                               "Path to executable command",
                               "" /* default value empty string*/,
                               (GParamFlags)G_PARAM_READWRITE );

  g_object_class_install_property( gobject_class, RUN_COMMAND, pspec );
}

G_MODULE_EXPORT struct WQ::Job *creator(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, WQ::DB::Adaptor *dbc)
{
  return new WQ::RunJob(id,name,record, dbc);
}
G_MODULE_EXPORT bool initialize(const gchar *runpath)
{
  return true;
}
G_MODULE_EXPORT void finalize()
{
}
namespace WQ {

RunJob::RunJob(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, WQ::DB::Adaptor *dbc)
  :Job(id,name,record,dbc)
{
}
RunJob::~RunJob()
{
  if( m_data ) {
    g_object_unref(m_data);
  }
}
int RunJob::execute()
{
  gint job_id = this->id();
  gchar *command = NULL;
//  gchar *std_out = NULL;
//  gchar *std_err = NULL;
  gint status = 0;
  GError *error = NULL;

  g_object_get(m_data, "command", &command, NULL);

  if( !command ) {
    g_critical("RunJob::Failed, command string is NULL!");
    return 0;
  }

  gint command_argc = 0;
  gchar **command_argv = NULL;

  if( !g_shell_parse_argv(command, &command_argc, &command_argv, &error) ) {
    g_critical("RunJob::Failed to parse command string: '%s'", error->message);
    g_free(command);
    return 0;
  }

  /*
  // XXX: g_spawn_command_line_sync depends on SIGCHLD to get the exit status, 
  //      we need another way to do this?
  if( !g_spawn_command_line_sync(command, &std_out, &std_err, &status, &error) ) {
    g_critical("RunJob::Failed with %s", error->message);
    g_error_free(error);
    if( std_out ) { g_free(std_out); }
    if( std_err ) { g_free(std_err); }
    g_free(command);
    return 0;
  }
  */

  // capture the output using a pipe
  int ofd[2];
  int efd[2];
  pid_t pid;

  // create pipe for capture stdout
  if( pipe(ofd) == -1 ) {
    g_critical("failed to create pipe for stdout\n"); 
    g_strfreev(command_argv);
    g_free(command);
    return 0;
  }

  // create pipe to capture stderr
  if( pipe(efd) == -1 ) {
    g_critical("failed to create pipe for stderr\n"); 
    close(ofd[0]);
    close(ofd[1]);
    g_strfreev(command_argv);
    g_free(command);
    return 0;
  }

  pid = fork();
  if( pid == -1 ) {
    close(ofd[0]);
    close(ofd[1]);
    close(efd[0]);
    close(efd[1]);
    g_critical("Failed to fork process");
    g_strfreev(command_argv);
    g_free(command);
    return 0;
  }

  if( pid == 0 ) {
    // bind stdout
    close(ofd[0]); // close read end of pipe
    dup2(ofd[1],fileno(stdout));// make 1 same as write-to end of pipe
    close(ofd[1]);

    // bind stderr
    close(efd[0]); // close read end of pipe
    dup2(efd[1],fileno(stderr)); // make 2 same as write-to end of pipe
    close(efd[1]);

    // prepare the new processes environment
    gchar *env[] = { "", "", (char *)0 };
    env[0] = g_strdup_printf("HOME=%s", g_getenv("HOME"));
    env[1] = g_strdup_printf("JOBID=%d", job_id);

    //execlp("cat","cat",__FILE__,NULL);
    execve(command_argv[0], command_argv, env);

    // oops error
    printf("failed to execute: '%s'\n", command_argv[0]);
    perror("execlp");
    exit(EXIT_FAILURE);
  }
  g_strfreev(command_argv); // doesn't need this

  // parent
  close(ofd[1]); // close write end of stdout pipe
  close(efd[1]); // close write end of stderr pipe

  char buffer[1024];
  char *ptr = NULL;
  FILE *ostream = fdopen(ofd[0],"rb");
  FILE *estream = fdopen(efd[0],"rb");
  GString *std_out = g_string_sized_new(1024); // TODO: probably make these a configuration option..
  GString *std_err = g_string_sized_new(512);
  //g_message("reading from child");

  do {
    if( !ferror(ostream) && !feof(ostream) ) {
      ptr = fgets(buffer,sizeof(buffer),ostream);
      if( ptr ) {
        g_string_append(std_out, ptr);
      }
    }
    if( !ferror(estream) && !feof(estream) ) {
      ptr = fgets(buffer,sizeof(buffer),estream);
      if( ptr ) {
        g_string_append(std_err, ptr);
      }
    }
  } while( (!ferror(ostream) && !feof(ostream)) || (!ferror(estream) && !feof(estream)) ) ;
  //g_message("read from child");

  fclose(ostream);
  fclose(estream);
  close(ofd[0]);
  close(efd[0]);

  g_message("wait for child status");
  waitpid(pid,&status,0);

  if( std_out ) {
    g_message("proc output: %s", std_out->str);
    g_string_free(std_out,true);
  }
  if( std_err ) {
    g_message("proc errors: %s", std_err->str);
    g_string_free(std_err,true);
  }
  g_free(command);

  return (status == 0) ? 1 : 0;
}
bool RunJob::load_data()
{
  GError *error = NULL;
  const gchar *data = m_record->str_field("data");
  if( data && *data != '\0' ) {
    m_data = (RunData*)json_construct_gobject(RUN_TYPE_DATA, data, (gsize)-1, &error);
    if( error ) { g_message("RunJob::Unable to create instance: %s", error->message); return false; }
    return true;
  }
  g_critical("RunJob::Must supply data attributes");
  return false;
}

}
