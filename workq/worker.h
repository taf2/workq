#ifndef WQ_WORKER_H
#define WQ_WORKER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <ev.h>
#include <glib.h>
#include "workq/settings.h"
#include "workq/job.h"
#include "workq/adaptor.h"
#include "workq/channel.h"

namespace WQ {

  // poll a database for new jobs
  // awake on USR1 signal to check for jobs
  struct Worker {
    Worker(const Settings &config);
    ~Worker();

    static inline const char *select_sql(){ return "select id, name, data from jobs where status='pending' and locked_queue_id=0 limit %d"; }
    static inline const char *count_select_sql(){ return "select count(id) as pending from jobs where status='pending' and locked_queue_id=0"; }

    static void execute(WQ::WorkerChannel *channel, const Settings &config);

    // initialize the worker process
    bool initialize(WQ::WorkerChannel *channel);
 
    int run();

    bool register_job_type( const char *job_name, job_creator_t constructor );

    void shutdown();

    struct WorkQueue {
      WorkQueue(const volatile bool &active, const Settings &config, GData *job_types);
      ~WorkQueue();

      // connect queue to db
      bool initialize();

      void send_work();
      void send_stop();

      void run();

      GArray *find_jobs(gint workload);

      GThread *thread;

      static gint compare(gconstpointer wq1, gconstpointer wq2);

      inline gint pending()const { return g_atomic_int_get(&m_pending); }

    protected:
      const volatile bool &m_active;    // continue polling the queue
      const Settings      &m_config;
      GAsyncQueue         *m_queue;     // queue of incoming requests
      GData               *m_job_types; // supported job requests
      DB::Adaptor         *m_db;        // db connection
      volatile gint       m_pending;    // number of times send_work was called decremented as we loop
    };

  protected:
    //static void on_signal_work(struct ev_loop *loop, struct ev_signal *watcher, int revents);
    static void on_io_work(struct ev_loop *loop, struct ev_io *watcher, int revents);
    static void on_signal_stop(struct ev_loop *loop, struct ev_signal *watcher, int revents);

    static gpointer work_thread(gpointer worker);

    void schedule();

  protected:
    bool m_active;
    const Settings &m_config;
    struct ev_loop *m_loop;
    //struct ev_signal *m_signal_work;
    struct ev_io     *m_io_work;
    struct ev_signal *m_signal_stop;
    struct ev_signal *m_signal_block_int;

    WorkQueue **m_threads;
    GData *m_job_types;
    WQ::WorkerChannel *m_channel;
  };

}

#endif
