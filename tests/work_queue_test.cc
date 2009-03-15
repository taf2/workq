#include "testlib.h"
#include <glib-object.h>
#include "workq/worker.h"
#include "mock_sql_adaptor.h"
#include "workq/mysql_adaptor.h"

static volatile gint job_counter = 0;
GStaticMutex job_mutex = G_STATIC_MUTEX_INIT;

struct JobTest : public WQ::Job {
  JobTest(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, WQ::DB::Adaptor *dbc) : WQ::Job(id,name, record,dbc) { }
  virtual ~JobTest() { }
  virtual int execute() {
    g_static_mutex_lock(&job_mutex);
    //g_print("executing job: %d\n", m_id);
    ++job_counter;
    g_static_mutex_unlock(&job_mutex);
    return 1;
  }
  static Job *create(gint id, const gchar *name, WQ::DB::Adaptor::Row *record, WQ::DB::Adaptor *dbc) {
    return new JobTest(id,name, record, dbc);
  }
protected:
  virtual bool load_data(){ return true; }

};

static gpointer work_thread(gpointer _td)
{
  static_cast<WQ::Worker::WorkQueue*>(_td)->run();
  return _td;
}

WQ::Worker::WorkQueue *gwq = NULL;

void test_work_queue_receives_work()
{
  int send = 3; // send 3 messages, the first is ignored and set to error because we never registered the magick type in tests/fixtures/jobs.yml
  g_print("sending %d work messages\n", send);
  for( int i = 0; i < send; ++i ) {
    gwq->send_work();
    g_usleep(10000);
  }

  bool complete = false;
  gdouble elapsed = 0.0;
  GTimer *timer =  g_timer_new();

  // give the queue 1 second to complete
  g_timer_start(timer);

  do {
    g_static_mutex_lock( &job_mutex );
    complete = job_counter == (send-1);
    g_static_mutex_unlock( &job_mutex );
    elapsed = g_timer_elapsed(timer, NULL);
    g_usleep(100000); // don't poll too fast
    g_print("completed: %d after %.2f\n", job_counter, elapsed);
  } while( !complete && elapsed < 1.0 ); // don't wait too long

  check( elapsed < 1.5 ); // make sure we're done within our threshold of time
  g_static_mutex_lock( &job_mutex );
  g_print("send: %d, job_counter: %d\n", send, job_counter);
  check_equal( (send-1), job_counter );
  g_static_mutex_unlock( &job_mutex );
  g_timer_stop(timer);
  g_timer_destroy(timer);

}

static void load_test_data()
{
  WQ::DB::MysqlAdaptor ma;
  initialize_adaptor_with_fixtures(ma);
}

// testing the WorkQueue class
int main(int argc, char **argv)
{
  g_type_init();
  g_thread_init(NULL);

  // initialize adaptor table
  WQ::DB::Adaptor::initialize();
  // register mysql adaptor
  WQ::DB::Adaptor::register_adaptor("mysql", WQ::DB::MysqlAdaptor::create );

  volatile bool active = true;
  GData *job_types = NULL;

  // load settings
  WQ_TEST_INIT_SETTINGS();

  // initialize test job
  g_datalist_init(&job_types);
  g_datalist_id_set_data(&job_types, g_quark_from_string("test"), (gpointer)JobTest::create);
  g_datalist_id_set_data(&job_types, g_quark_from_string("thumbnail"), (gpointer)JobTest::create);

  // load fixture data
  load_test_data();

  // create the work queue instance
  gwq = new WQ::Worker::WorkQueue(active, *wq_test_settings, job_types);

  bool bstatus = gwq->initialize();
  check(bstatus);
  if( !bstatus ) { g_critical("Failed to initialize work queue"); return 1; }

  // start up the test thread
  GError *status = NULL;
  gwq->thread = g_thread_create(work_thread, gwq, true, &status);

  if( status ) {
    g_critical("error: %s", status->message);
    g_error_free( status );
    return 1;
  }

  // send the work queue some test jobs
  run_tests("WorkQueue:", test_work_queue_receives_work,  NULL);

  // stop the queue and shutdown the thread
  active = false;
  //g_print("sending stop\n");
  gwq->send_stop();
  //g_thread_join(gwq->thread);
  //g_print("thread joined\n");
  g_datalist_clear(&job_types);

  delete gwq;

  WQ::DB::Adaptor::finalize();

  return 0;
}
