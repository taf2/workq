#ifndef WQ_CHANNEL_H
#define WQ_CHANNEL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

namespace WQ {
  // maintain a line of communicate with the worker
  struct WorkerChannel {
    WorkerChannel(); // does not open the pipe, call initialize
    ~WorkerChannel(); // close the pipe
    bool open(); // open's the pipe

    void child_init(); // initialize the child
    void parent_init(); // initialize the parent

    void notifyChild(int workload);

    bool waitForChildInit();

    int server_send_pipe[2]; // server sends messages, worker receives messages
    int worker_send_pipe[2]; // worker sends messages, server receives messages
    bool child;
  };
}

#endif
