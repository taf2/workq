#ifndef WQ_SERVER_H
#define WQ_SERVER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <ev.h>
#include <glib.h>

#include "workq/adaptor.h"
#include "workq/worker.h"
#include "workq/settings.h"

namespace WQ {

  // listen for incoming job requests
  // when a new job is received send a signal USR1 to the next available worker
  struct Server {
    Server(const Settings &config);
    ~Server();

    bool initialize();

    bool start_workers();

    int run();


  protected:
    static void on_connect(struct ev_loop *loop, struct ev_io *watcher, int revents);
    static void on_timeout(struct ev_loop *loop, struct ev_timer *timer, int revents);
    static void on_worker_exit(struct ev_loop *loop, struct ev_child *child, int revents);
    static void on_shutdown_signal(struct ev_loop *loop, struct ev_signal *signal, int revents);

    // bind to UDP socket on port
    int bindsock();

    // create worker processes
    bool start_worker(int i);

    void schedule();

    void idle_jobcheck();
    void increase_timeout();

    void shutdown();
  private:
    int m_sock;
    struct ev_io *m_conn_watcher;
    struct ev_loop *m_loop;
    struct ev_timer *m_timeout;
    struct ev_child **m_workers; // monitor worker status
    struct ev_signal *m_term_sig;
    struct ev_signal *m_int_sig;
    gdouble m_idle_timeout_duration;

    char m_rbuffer[1]; // read buffer
    const Settings &m_config;
    pid_t *m_pids;
    int m_num_pids;
    int m_last_worker;
    WorkerChannel **m_worker_channels;

    DB::Adaptor *m_db;
  };
}
#endif
