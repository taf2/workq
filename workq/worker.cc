#include "worker.h"
#include "util.h"
#include <gmodule.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

/*
 * Worker Process
 *
 * A main thread listens for signals from the master process.
 *
 * When a SIGUSR1 signal is received work is queued in a thread specific queue.
 * 
 * When the thread detects new work has been added to it's queue it wakes up and pops the work from its queue.
 * Next it updates X number of records using a process/thread/timestamp to identify the work. This also servers
 * the purpose of ensuring no other worker picks up the same work.
 *
 * Once the work is marked the worker thread selects all work assigned to it and begins looping through each job.
 *
 * For each job the worker creates a job object passing it the database record/row, providing the data to complete the task.
 * After each job is run the status of the record is updated as either complete or error
 *
 * After the worker thread finishes all it's job's it goes back to sleep waiting on more items to appear in it's queue.
 *
 */

namespace WQ {

// worker process startup
void Worker::execute(WorkerChannel *channel, const Settings &config)
{
  // we're the child process, change execution paths
  const gchar *exe_path = config.get_str("execution_path", config.get_binpath());
  gchar *change_path = NULL;

  if( g_path_is_absolute(exe_path) ) {
    change_path = g_strdup(exe_path);
  }
  else {
    gchar *cur_path = g_get_current_dir();
    change_path = g_build_path("/", cur_path, exe_path, NULL);
    g_free(cur_path);
  }
  //g_message("Worker[%ld] change dir: %s", (glong)getpid(), change_path);
  g_chdir(change_path);

  // initialize the gobject type system
  g_type_init();
  // initialize the gthread environment
  g_thread_init(NULL);

  Worker worker(config);

  worker.initialize(channel);

  // load jobs as dynamic libraries, based on configuration
  gchar **job_types = g_strsplit(config.get_str("jobs"), ",", 0);
  gchar **job_counter = job_types;
  guint job_total = 0;
  GModule **job_modules = NULL;

  // count the number of jobs to load
  while( *job_counter++ ) { ++job_total; }

  // allocate space for each job
  job_modules = (GModule**)g_malloc(sizeof(GModule*)*job_total);

  const gchar *module_prefix = config.get_str("job_prefix", WQ_CONFIG_LIB_DIR);

  for( guint i = 0; i < job_total; ++i ) {
    const gchar *job_module_name = g_strstrip(job_types[i]);

    gchar *module_path = g_strdup_printf("%s/%s", module_prefix, job_module_name);

    // open the module
    //g_message("opening job '%s/%s'", module_prefix, job_module_name);
    job_modules[i] = g_module_open(module_path, G_MODULE_BIND_MASK);
    if( !job_modules[i] ) {
      g_warning("%s: %s", module_path, g_module_error ());
      g_free(module_path);
      continue;
    }
    g_free(module_path);

    job_creator_t creator;
    job_initialize_t initialize;

    if( !g_module_symbol( job_modules[i], "creator", (gpointer*)&creator ) ) {
      g_warning("%s: %s", job_module_name, g_module_error ());
      continue;
    }
    if( g_module_symbol( job_modules[i], "initialize", (gpointer*)&initialize ) ) {
      initialize(change_path);
    }
    worker.register_job_type(job_module_name, creator);
  }
  g_strfreev(job_types);
  g_free(change_path);

  close(channel->worker_send_pipe[1]); // close the pipe signaling to parent we're up and running
  worker.run();

  for( guint i = 0; i < job_total; ++i ) {
    job_finalize_t finalize;
    if( g_module_symbol( job_modules[i], "finalize", (gpointer*)&finalize ) ) {
      finalize();
    }
    g_module_close(job_modules[i]);
  }
  g_free(job_modules);
  exit(EXIT_SUCCESS);
}

Worker::Worker(const Settings &config)
  : m_config(config), m_loop(0),
    m_io_work(0), // m_signal_work(0), 
    m_signal_stop(0), m_signal_block_int(0),
    m_threads(0), m_job_types(0), m_channel(0)
{
  g_datalist_init(&m_job_types);
}

Worker::~Worker()
{
  //if( m_signal_work ) { g_free(m_signal_work); }
  if( m_io_work ) { g_free(m_io_work); }
  if( m_signal_stop ) { g_free(m_signal_stop); }
  if( m_signal_block_int ) { g_free(m_signal_block_int); }
  if( m_threads ) { g_free( m_threads ); }
  //if( m_channel ) { delete m_channel; }
  g_datalist_clear(&m_job_types);
}

bool Worker::initialize(WorkerChannel *channel)
{
  m_loop = ev_default_loop(0);
  if( !m_loop ) { return false; }

  // adjust resolution to debug
  ev_set_io_collect_interval(m_loop, 0 );
  ev_set_timeout_collect_interval(m_loop, 0 );
  
  /*m_signal_work = (struct ev_signal*)g_malloc(sizeof(struct ev_signal));
  memset(m_signal_work, 0, sizeof(struct ev_signal) );
  m_signal_work->data = this;
  ev_signal_init(m_signal_work, on_signal_work, SIGUSR1);
  */
  this->m_channel = channel;

  m_io_work = (struct ev_io*)g_malloc(sizeof(struct ev_io));
  memset(m_io_work, 0, sizeof(struct ev_io));
  m_io_work->data = this;
  ev_io_init(m_io_work, on_io_work, channel->server_send_pipe[0], EV_READ);

  m_signal_stop = (struct ev_signal*)g_malloc(sizeof(struct ev_signal));
  memset(m_signal_stop, 0, sizeof(struct ev_signal) );
  m_signal_stop->data = this;
  ev_signal_init(m_signal_stop, on_signal_stop, SIGUSR2);

  m_signal_block_int = (struct ev_signal*)g_malloc(sizeof(struct ev_signal));
  memset(m_signal_block_int, 0, sizeof(struct ev_signal) );
  m_signal_block_int->data = this;
  ev_signal_init(m_signal_block_int, on_signal_stop, SIGINT); // XXX: might want to block this to ensure deterministic shutdown

  return true;
}

/*
void Worker::on_signal_work(struct ev_loop *loop, struct ev_signal *watcher, int revents)
{
  g_message("received work signal\n");
  Worker *worker = static_cast<Worker*>(watcher->data);
  worker->schedule();
}*/
void Worker::on_io_work(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
  char buffer[81];
  Worker *worker = static_cast<Worker*>(watcher->data);
  worker->schedule();
  // read from the channel
  memset(buffer,0,sizeof(buffer));
  int bytes = read(worker->m_channel->server_send_pipe[0], buffer, (sizeof(buffer)-1));
  if( bytes == -1 || bytes == 0 ) {
    g_critical("Parent has gone down!");
    ev_io_stop(loop, watcher);
    worker->shutdown();
    exit(EXIT_FAILURE); // die, we lost our parent
    return;
  }
  //g_message("received work signal:%d", bytes);
  for( int i = 0; i < (bytes-1); ++i ) {
    worker->schedule();
  }
}

gpointer Worker::work_thread(gpointer _td)
{
  static_cast<Worker::WorkQueue*>(_td)->run();
  return _td;
}
void Worker::schedule()
{
  // determine which thread is least busy and send it the work

  int threads = m_config.get_int("threads", 2);

  //g_message("%ld scheduling to %d threads\n", (glong)getpid(), threads);

  // sort the threads array using the queue length as the condition
  //qsort(m_threads,threads,sizeof(WorkQueue*),Worker::WorkQueue::compare);
  gint smallest = 10;
  gint selected = 0;
  gint value = 0;

  for( gint i = 0; i < threads; ++i ) {
    value = m_threads[i]->pending();
    if( smallest < value ) {
      smallest = value;
      selected = i;
    }
  }

  //g_message("%ld sending work to thread: %d with %d pending\n", (glong)getpid(), selected, value);

  // after sorting the first entry in the array should contain the least busy thread
  m_threads[selected]->send_work();
}

void Worker::on_signal_stop(struct ev_loop *loop, struct ev_signal *watcher, int revents)
{
  g_print("worker received shutdown\n");

  Worker *worker = static_cast<Worker*>(watcher->data);

  // stop receiving events
  //ev_signal_stop(worker->m_loop, worker->m_signal_work);
  ev_io_stop(worker->m_loop, worker->m_io_work);
  ev_signal_stop(worker->m_loop, worker->m_signal_stop);
  ev_signal_stop(worker->m_loop, worker->m_signal_block_int);
  ev_unloop(worker->m_loop,0);

  // tell all threads to stop working
  worker->shutdown();
}

Worker::WorkQueue::WorkQueue(const volatile bool &active, const Settings &config, GData *job_types)
  : thread(NULL), m_active(active), m_config(config),
    m_queue(g_async_queue_new()), m_job_types(job_types),
    m_db(NULL)
{
}
Worker::WorkQueue::~WorkQueue()
{
  //g_message("Worker::WorkQueue::destructor");
  if( thread ) {
    g_thread_join(thread);
  }
  g_async_queue_unref(m_queue);
  if( m_db ) {
    delete m_db;
  }
}
void Worker::WorkQueue::send_work()
{
  g_atomic_int_add(&m_pending,1);
  g_async_queue_push( m_queue, (gpointer)1);
}
void Worker::WorkQueue::send_stop()
{
  g_async_queue_push( m_queue, (gpointer)-1);
}

int Worker::WorkQueue::compare(const void *q1, const void *q2)
{
  const WorkQueue *w1 = *static_cast<const WorkQueue*const*>(q1);
  const WorkQueue *w2 = *static_cast<const WorkQueue*const*>(q2);

  return (w1->pending() - w2->pending());
}

bool Worker::WorkQueue::initialize()
{
  if( m_db ) { return true; } // already initialized
  const char *adaptor_type = m_config.get_str("adapter","mysql");
  m_db = DB::Adaptor::create(adaptor_type);

  if( m_db ) {
    //g_print("got an adaptor\n");
    const char *db_username = m_config.get_str("username");
    const char *db_password = m_config.get_str("password");
    const char *db_database = m_config.get_str("database");
    const char *db_host     = m_config.get_str("host");
    gint        db_port     = m_config.get_int("port", 3306);
    if(  m_db->connect(db_username,db_password,db_host,db_database,db_port) ) {
      m_db->query("SET NAMES UTF8");
      //m_db->query("SET AUTOCOMMIT=1");
      return true;
    }
  }
  return false;
}
GArray *Worker::WorkQueue::find_jobs(gint workload)
{
  glong pid_id = (glong)getpid();
  glong thread_id = (glong)this;
  GTimeVal curtm;
  gchar lockid[40];

  g_get_current_time(&curtm);
  g_snprintf(lockid,sizeof(lockid), "%ld:%ld:%ld:%ld", pid_id, thread_id, curtm.tv_sec, curtm.tv_usec);

  // start the transaction by updating workload count jobs to locked_queue_id=lockid
  //g_message("lock jobs with: %s\n", lockid);

  if( m_db->query("UPDATE jobs SET locked_queue_id='%s', status='loading' "
                  "WHERE status='pending' AND locked_queue_id='' ORDER BY id LIMIT %d", lockid, workload) ) {
    g_critical("%ld, sql error: %s", thread_id, m_db->errors() );
    return NULL;
  }

  //g_message("rows effected: %d", m_db->rows_affected() );

  // query the adaptor
  DB::Adaptor::Iterator *it = m_db->query_with_result("SELECT id, name, data FROM jobs "
                                                      "WHERE status='loading' AND locked_queue_id='%s' ORDER BY id LIMIT %d", lockid, workload );
  if( !it ) {
    g_critical("%ld, sql error: %s", thread_id, m_db->errors() );
    m_db->query("UPDATE jobs SET locked_queue_id='' WHERE status!='pending' AND locked_queue_id='%s' ORDER BY id", lockid );
    return NULL;
  }

  // store each selected job for processing after the sql transaction
  GArray *jobs = g_array_new(false, true, sizeof(Job*));

  for( DB::Adaptor::Row *row = it->begin(); row; row = it->next() ) {
    int job_id                 = row->int_field("id");
    const char *job_name       = row->str_field("name");
    GQuark job_name_id         = g_quark_from_string(job_name);
    job_creator_t type_creator = (job_creator_t)g_datalist_id_get_data(&m_job_types, job_name_id);

    //g_message("%ld, received job: %s:%d", thread_id, job_name, job_id);
    if( type_creator ) {
      // we know how to handle this type
      Job *job = type_creator(job_id, job_name, row, m_db);
      if( job ) {
        g_array_append_val(jobs, job);
      }
      else {
        g_critical("%ld Failed to create job type: %s:%d", thread_id, job_name, job_id);
        if( m_config.get_bool("discard_unknown_jobs",true) ) {
          m_db->query("UPDATE jobs SET status='error', locked_queue_id='' WHERE id=%d ORDER BY id", job_id);
        }
      }
      // lock the job 
      //g_print("new job locked\n");
      m_db->query("UPDATE jobs SET status='processing' WHERE id=%d", job_id);
      // the job takes control of the row and will free it for us
    }
    else {
      // we've never heard of this type log it and move on
      if( m_config.get_bool("discard_unknown_jobs",true) ) {
        m_db->query("UPDATE jobs SET status='error', locked_queue_id='' WHERE id=%d ORDER BY id", job_id);
      }
      g_critical("%ld, unknown job type: %s:%d", thread_id, job_name, job_id);
      // else the job is not removed, this can cause other jobs to never be processed,
      // better to always set to error? or have other workers setup to not discard this job
      // XXX: you really really better have another worker setup somewhere to eat these jobs or
      // you'll spin on these
      delete row;
    }
  }
  //m_db->query("COMMIT");

  delete it;

  return jobs;
}

void Worker::WorkQueue::run()
{
  GTimeVal wait_time;
  int thread_sleep_time = m_config.get_int("thread_sleep_time", 2000000);
  glong pid_id = (glong)getpid();
  glong thread_id = (glong)this;

  while( m_active ) {
    gpointer message = NULL;
    gint workload = 0;

    g_get_current_time(&wait_time);
    g_time_val_add(&wait_time, thread_sleep_time);

    message = g_async_queue_timed_pop(m_queue, &wait_time);
    if( message ) {
      do {
        glong value = (glong)message;
        //g_message("%ld:%ld, work message: %ld\n", pid_id, thread_id, value );
        if( value == -1 ) { workload = 0; break; } // -1 is the stop message
        workload += value;
        g_thread_yield();
      } while( (message=g_async_queue_try_pop( m_queue )) );
    }

    if( workload <= 0 ) { continue; }

    GArray *jobs = this->find_jobs(workload);
    if( !jobs ) { continue; }

    // process the jobs now that they're locked
    for( guint i = 0; i < jobs->len; ++i ) {
      Job *job = g_array_index(jobs,Job*,i);
      //g_message("%ld:%ld, execute job: %s:%d", pid_id, thread_id, job->name(), job->id() );
      if( job->real_execute() ) {
        // log job succeeded
        //g_message("%ld:%ld, job %s:%d succeeded", pid_id, thread_id, job->name(), job->id() );
        m_db->query("UPDATE jobs SET locked_queue_id='', status='complete' WHERE id=%d ORDER BY id", job->id());
      }
      else {
        // log job failed
        //g_message("%ld:%ld, job %s:%d failed", pid_id, thread_id, job->name(), job->id() );
        m_db->query("UPDATE jobs SET locked_queue_id='', status='error' WHERE id=%d ORDER BY id", job->id());
      }
      --workload;
      g_atomic_int_add(&m_pending,-1);
      delete job;
    }
    g_array_free(jobs,true);

  } // end while
  //g_message("WorkQueue::%ld shutting down...", thread_id );
  g_thread_exit(0);
}

bool Worker::register_job_type( const char *job_name, job_creator_t constructor )
{
  GQuark key = g_quark_from_string(job_name);
  g_datalist_id_set_data(&m_job_types, key, (gpointer)constructor);
  return true;
}

int Worker::run()
{
  int threads = m_config.get_int("threads", 2);
  m_active = true;
  m_threads = (WorkQueue**)g_malloc(sizeof(WorkQueue*) * threads);

  // startup worker threads
  for( int i = 0; i < threads; ++i ) {
    GError *status = NULL;

    m_threads[i] = new WorkQueue(m_active, m_config, m_job_types);
    if( !m_threads[i]->initialize() ) {
      g_critical("failed to initialize WorkQueue");
      return 1;
    }
    m_threads[i]->thread = g_thread_create(work_thread, m_threads[i], true, &status);

    if( status ) {
      g_critical("error: %s\n", status->message);
      g_error_free( status );
      return 1;
    }
  }
 
  // enable signal listeners
  //ev_signal_start(m_loop, m_signal_work);
  ev_io_start(m_loop, m_io_work);
  ev_signal_start(m_loop, m_signal_stop);
  ev_signal_start(m_loop, m_signal_block_int);

  // run the loop
  ev_loop(m_loop,0);

  return 0;
}

void Worker::shutdown()
{
  int threads = m_config.get_int("threads", 2);
  //g_message("Worker:: shutting down...");
  m_active = false;
  for( int i = 0; i < threads; ++i ) {
    m_threads[i]->send_stop();
  }

  for( int i = 0; i < threads; ++i ) {
    //g_thread_join(m_threads[i]->thread);
    delete m_threads[i];
    m_threads[i] = NULL;
  }
}

}
