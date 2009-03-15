#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "workq/channel.h"

namespace WQ {
WorkerChannel::WorkerChannel() : child(false)
{
  memset(server_send_pipe,0, sizeof(server_send_pipe));
  memset(worker_send_pipe,0, sizeof(worker_send_pipe));
}
WorkerChannel::~WorkerChannel()
{
  if( child ) {
    close(server_send_pipe[0]); // done with the parent
  }
  else {
    close(server_send_pipe[1]); // done with the worker
  }
}
bool WorkerChannel::open()
{
  return (pipe(server_send_pipe) == 0) && (pipe(worker_send_pipe) == 0);
}
void WorkerChannel::child_init()
{
  child = true;
  // close the write side, we only take orders from the server
  close(server_send_pipe[1]);
  // close the read side, we can only talk to the server
  close(worker_send_pipe[0]);
}
void WorkerChannel::parent_init()
{
  child = false;
  // close the read side, we don't listen to our workers
  close(server_send_pipe[0]);
  // close the write side, we just listen to our workers
  close(worker_send_pipe[1]);
}
bool WorkerChannel::waitForChildInit()
{
  char readbuffer[80];

  if( read(worker_send_pipe[0], readbuffer, sizeof(readbuffer)) == -1 ) { // hold the process down until it closes
    perror("read");
    return false;
  }
  // TODO: check readbuffer for errors starting the worker
  close(worker_send_pipe[0]); // close it
  return true;
}
}
