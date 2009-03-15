/*
 *
 * The server will listen for incoming events
 *
 * When work is either signalled or polled a worker process will wake up and check for 
 * new work by selecting jobs from a database.
 *
 * The server runs as a daemon, forking X number of workers.
 *
 * Workers poll a database for new work as well as respond to a signal from the master process to check
 * for new jobs.
 *
 * Jobs are defined as a record in a database and have specific file system and network components.
 *
 * We start with the following job types:
 *
 * Video encoder:
 *  Given a video file, use ffmpeg to transcode the video file into flv format for streaming.
 *  Upload the final flv to amazon s3 web service.
 *
 * Thumbnail creater:
 *  Given a image file, use ImageMagick to scale the image down to a specific thumbnail size.
 *  Upload the final thumbnail to amazon s3 web service.
 *
 * Spam checker:
 *  Given a body of content (typically a comment), use akismet web service to check if the content and metadata
 *  is spam or not.  Also, support reporting content as not spam or as spam.
 *
 * Sendmail:
 *  Given an email message, send the email message to an IMAP server.
 *
 * RSS feeds:
 *  Given an RSS feed, download and process the RSS Feed creating a user facing feed record.
 *
 * Logging:
 *  A log file should be created for the server process and for each worker processes.
 *  The logging should support recovery in the event the log file is rotated, it should be reopened/crearted.
 *
 * Concurrency:
 *  Some workers may support higher levels of concurrency since they are highly I/O driven.  For these types of works
 *  an event driven approach using libev is usually best.  For less I/O driven more CPU intensive Jobs like Video encoding
 *  multiple threads or processes will be a better model. Because of this and to keep the model simple, a worker process
 *  is composed on a main event loop, that watches for incoming user signals.
 *
 */

#include "workq/config.h"
#include <ev.h>
#include <glib.h>
#include "workq/adaptor.h"
#include "workq/worker.h"
#include "workq/settings.h"
#include "workq/server.h"

#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "workq/util.h"
#include "workq/mysql_adaptor.h"

static int cli_setup(WQ::Settings &, int argc, char **argv);
static void workq_log(const gchar *log_domain, GLogLevelFlags log_level,
                      const gchar *message, gpointer data);

int main(int argc, char **argv)
{
  WQ::DB::Adaptor::initialize();
  FILE *logfile = NULL;
  const gchar *env = g_getenv("WORKQ_ENV");
  if( !env ) {
    env = "development";
  }
  WQ::Settings settings(*argv, env);

  // get settings from command line
  if( cli_setup(settings, argc, argv) ) { return 1; }

  // check for to run process in background
  if( settings.get_bool("daemonize",true) ) {
    const gchar *pidfile = settings.get_str("pidfile");
    if( g_file_test(pidfile, G_FILE_TEST_EXISTS) ) {
      g_error("Process appears to already be running, check '%s'", pidfile);
      exit(EXIT_FAILURE);
    }
    WQ::daemonize();
    // write the daemon pid file
    if( pidfile ) {
      FILE *p = fopen(pidfile,"wb");
      g_fprintf(p, "%ld", (glong)getpid());
      fclose(p);
    }
  }

  // check for logfile
  const gchar *logpath = settings.get_str("logfile");
  if( logpath ) {
    logfile = g_fopen(logpath,"a");
    if( logfile ) {
      g_log_set_handler(NULL, (GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), workq_log, logfile);
    }
  }

  // register sql adaptor
  if( !strcmp("mysql", settings.get_str("adapter")) ) {
    // register mysql adaptor
    WQ::DB::Adaptor::register_adaptor("mysql", WQ::DB::MysqlAdaptor::create );
  }
  else {
    g_print("Adaptor %s not supported!\n", settings.get_str("adapter") );
    exit(EXIT_FAILURE);
  }

  // create the server
  WQ::Server server(settings);

  if( !server.initialize() ) {
    return 1;
  }

  // setup the workers
  server.start_workers();

  // start listening for events
  server.run();

  g_message("server stopped\n");
  WQ::DB::Adaptor::finalize();

  const gchar *pidfile = settings.get_str("pidfile");
  if( g_file_test(pidfile, G_FILE_TEST_EXISTS) ) {
    g_unlink(pidfile);
  }
  if( logfile ) { fflush(logfile); g_message("server down\n"); fclose(logfile); }
  else { fflush(stdout); g_print("server down\n"); }
  exit(EXIT_SUCCESS);
  return 0;
}

int cli_setup(WQ::Settings &config, int argc, char **argv)
{
  int ret = 0;
  gchar *config_path = NULL;
  gboolean check_kill = FALSE;
  gboolean install_schema = FALSE;
  gboolean version = FALSE;
  gchar *env = NULL;

  GOptionEntry entries[] =
  {
    { "config", 'c', 0, G_OPTION_ARG_FILENAME, &config_path, "Path to configuration file", "N" },
    { "install-schema", 'i', 0, G_OPTION_ARG_NONE, &install_schema, "Using the configured database, create the jobs table.", NULL },
    { "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Work Queue version", NULL },
    { "environment", 'e', 0, G_OPTION_ARG_STRING, &env, "Work Queue runtime environment.  A top level key in the configuration file designating what key/value pairs to load.", NULL },
    { "kill", 'k', 0, G_OPTION_ARG_NONE, &check_kill, "Kill the daemon if it's pid file is present", NULL },
    { NULL }
  };
  GError *error = NULL;

  GOptionContext *ctx = g_option_context_new("-c");
  g_option_context_add_main_entries(ctx, entries, NULL);

  if( !g_option_context_parse( ctx, &argc, &argv, &error ) ) {
    g_print("option parsing failed: %s\n", error->message);
    ret = 1;
    goto cli_finished;
  }

  if( version ) {
    printf(VERSION"\n");
    exit(0);
  }

  if( env ) {
    config.set_env(env);
  }

  if( !config_path ) {
    gchar *help = g_option_context_get_help(ctx,TRUE,0);
    g_print( "Must supply a configuration file\n%s\n", help );
    g_free(help);
    ret = 1;
    goto cli_finished;
  }

  if( !config.load_from_file(config_path) ) {
    ret = 1;
    goto cli_finished;
  }

  if( !config.verify() ) {
    ret = 1;
    goto cli_finished;
  }

  config.commit();

  // at this point we have parsed the configuration, and we need to check if the user requested
  // to try and kill the daemon instead of starting it
  if( check_kill ) {
    gchar *pidstr = NULL;
    gsize len;
    if( g_file_get_contents(config.get_str("pidfile", "logs/workq.pid"), &pidstr, &len, NULL ) ) {
      gint pid = atoi(pidstr);
      kill(pid,SIGTERM);
      g_free(pidstr);
      exit(EXIT_SUCCESS); // early terminate
    }
  }
  else if( install_schema ) {
    WQ::DB::MysqlAdaptor db;

    if( !db.connect( config.get_str("username"),
                     config.get_str("password"),
                     config.get_str("host"),
                     config.get_str("database"),
                     config.get_int("port", 3306) ) ) {
      g_critical("Failed to connect to database, check configuration and try again");
      exit(EXIT_FAILURE);
    }
    gint status = db.query("create table jobs ( id int(11) not null auto_increment,\
                                  name varchar(255) NOT NULL,\
                                  status varchar(255) NOT NULL,\
                                  attempts int(11) default 0,\
                                  locked_queue_id varchar(40) default '', \
                                  data TEXT,\
                                  details TEXT,\
                                  created_at datetime,\
                                  updated_at datetime,\
                                PRIMARY KEY  (id)\
                              ) ENGINE=InnoDB DEFAULT CHARSET=utf8");
    exit(status); // early terminate
  }

cli_finished:
  if( config_path) g_free(config_path);
  g_option_context_free(ctx);
  return ret;
}

void workq_log(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer data)
{
  FILE *logfile = (FILE*)data;
  switch( log_level ) {
  case G_LOG_FLAG_RECURSION:
  case G_LOG_FLAG_FATAL:
    g_fprintf(logfile, "[FATAL]:");
    break;
  case G_LOG_LEVEL_ERROR:
    g_fprintf(logfile, "[ERROR]:");
    break;
  case G_LOG_LEVEL_CRITICAL:
    g_fprintf(logfile, "[CRITICAL]:");
    break;
  case G_LOG_LEVEL_WARNING:
    g_fprintf(logfile, "[WARNING]:");
    break;
  case G_LOG_LEVEL_MESSAGE:
    g_fprintf(logfile, "[MESSAGE]:");
    break;
  case G_LOG_LEVEL_INFO:
    g_fprintf(logfile, "[INFO]:");
    break;
  case G_LOG_LEVEL_DEBUG:
    g_fprintf(logfile, "[DEBUG]:");
    break;
  default:
    break;
  }
  g_fprintf(logfile, "%s\n", message);
  fflush(logfile);
}
