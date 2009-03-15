#include "server.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include "workq/util.h"
#include "workq/channel.h"

namespace WQ {

Server::Server(const Settings &settings)
  : m_sock(-1), m_conn_watcher(0), m_loop(0),
    m_timeout(0), m_workers(0), m_term_sig(0),
    m_int_sig(0), m_config(settings), m_pids(0),
    m_last_worker(0), m_db(0),
    m_worker_channels(0)
{
}

bool Server::initialize()
{
  // initialize event loop
  m_loop = ev_default_loop(0);
  if( !m_loop ) { return false; }

  // adjust resolution to debug
  //ev_set_io_collect_interval(m_loop, 0 );
  //ev_set_timeout_collect_interval(m_loop, 0 );

  m_conn_watcher = (struct ev_io*)g_malloc(sizeof(struct ev_io));

  memset(m_conn_watcher, 0, sizeof(struct ev_io) );
  m_conn_watcher->data = this;

  ev_init(m_conn_watcher, on_connect);

  // initialize the timeout, but don't start it
  m_timeout = (struct ev_timer*)g_malloc(sizeof(struct ev_timer));
  memset(m_timeout, 0, sizeof(struct ev_timer) );
  m_timeout->data = this;
  // initialize the timeout duration to min_idle_timeout
  m_idle_timeout_duration = m_config.get_double("min_idle_timeout", 5.0);
  ev_timer_init( m_timeout, on_timeout, 0.0, m_idle_timeout_duration );

  // watch for fatal signals term and int, TODO: add more?
  m_term_sig = (struct ev_signal*)g_malloc(sizeof(struct ev_signal));
  memset(m_term_sig, 0, sizeof(struct ev_signal));
  m_term_sig->data = this;
  ev_signal_init( m_term_sig, on_shutdown_signal, SIGTERM);
  m_int_sig = (struct ev_signal*)g_malloc(sizeof(struct ev_signal));
  memset(m_int_sig, 0, sizeof(struct ev_signal));
  m_int_sig->data = this;
  ev_signal_init( m_int_sig, on_shutdown_signal, SIGINT);

  // open db connection
  const char *adaptor_type = m_config.get_str("adapter","mysql");
  m_db = DB::Adaptor::create(adaptor_type);
  if( m_db ) {
    const char *db_username = m_config.get_str("username");
    const char *db_password = m_config.get_str("password");
    const char *db_database = m_config.get_str("database");
    const char *db_host     = m_config.get_str("host");
    gint        db_port     = m_config.get_int("port", 3306);
    return m_db->connect(db_username,db_password,db_host,db_database,db_port);
  }
  return false;
}

bool Server::start_worker(int i)
{
  pid_t pid;

  WorkerChannel *channel = new WorkerChannel();

  // register the channel with the server
  m_worker_channels[i] = channel;

  // open up the channel
  if( !channel->open() ) {
    perror("pipe");
    return false;
  }

  pid = fork();
  if (pid < 0) {
    perror("Failed to daemonize while forking from parent");
    exit(EXIT_FAILURE);
  }
  if( !pid ) {
    // we're the child process
    channel->child_init();
    Worker::execute(channel, m_config);
  }
  channel->parent_init();
  if( !channel->waitForChildInit() ) {
    g_critical("Worker[%ld] failed to start", (glong)pid);
    return false;
  }
  g_message("Worker[%ld] started", (glong)pid);
  // parent process
  m_pids[i] = pid; // store the pid for communication later

  // setup the child watcher
  m_workers[i] = (struct ev_child*)g_malloc0(sizeof(struct ev_child));
  ev_child_init(m_workers[i], on_worker_exit, pid, 0);

  return true;
}

bool Server::start_workers()
{
  this->m_num_pids = m_config.get_int("workers",1); // spawn 1 worker by default 

  m_pids = (pid_t*)g_malloc0(sizeof(pid_t)*m_num_pids);
  m_workers = (struct ev_child**)g_malloc0(sizeof(struct ev_child*)*m_num_pids);

  m_worker_channels = (WorkerChannel**)g_malloc0(sizeof(WorkerChannel*)*m_num_pids);

  for( int i = 0; i < m_num_pids; ++i ) {
    start_worker(i);
  }

  return true;
}

void Server::on_connect(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
  Server *server = static_cast<Server*>(watcher->data);

  // update our activity timestamp
  server->m_idle_timeout_duration = server->m_config.get_double("min_idle_timeout", 5.0); // reset to min idle timeout
  ev_timer_stop( loop, server->m_timeout );
  ev_timer_set( server->m_timeout, 0.0, server->m_idle_timeout_duration );
  ev_timer_again( loop, server->m_timeout ); // start the timeout again

  // read message byte
  if( ::recv( server->m_sock, server->m_rbuffer, sizeof(server->m_rbuffer), 0 ) != -1 ) {
    server->schedule();
  }
}
void Server::on_timeout(struct ev_loop *loop, struct ev_timer *timer, int revents)
{
  Server *server = static_cast<Server*>(timer->data);
  server->idle_jobcheck();
}
void Server::idle_jobcheck()
{
  DB::Adaptor::Iterator *it = m_db->query_with_result(Worker::count_select_sql());
  if( it ) { 
    DB::Adaptor::Row *row = it->begin();
    if( row ) {
      int pending = row->int_field("pending");
      int max_pending = m_config.get_int("max_scheduled_per_idle", 64);
      //for( int i = 0; i < pending && i < max_pending; ++i ) {
      //  g_usleep(2000000); // give the process signal sometime
     // }
     // TODO: use a pipe between workers intead of signals
      if( pending > 0 ) { // we got activity, reset the timer
        g_message("Server timeout:: pending jobs: %d, after: %f\n", pending, m_idle_timeout_duration );
        this->schedule();
        m_idle_timeout_duration = m_config.get_double("min_idle_timeout", 5.0); // reset to min idle timeout
        ev_timer_stop( m_loop, m_timeout );
        ev_timer_set( m_timeout, 0.0, m_idle_timeout_duration );
        ev_timer_again( m_loop, m_timeout ); // start the timeout again
      }
      else {
        this->increase_timeout(); // no activity timeout longer
      }
      delete row;
    }
    delete it;
  }
  else {
    g_critical("Error selecting job count from master server process");
    this->increase_timeout(); // no activity timeout longer
  }
}
void Server::increase_timeout()
{
  gdouble max_idle_timeout = m_config.get_double("max_idle_timeout", 64);
  if( m_idle_timeout_duration < max_idle_timeout )  {
    // x = (x^2) / max
    gdouble max2 = max_idle_timeout * max_idle_timeout;
    gdouble x2 = m_idle_timeout_duration * m_idle_timeout_duration;
    m_idle_timeout_duration += fabs( log10( max2 - x2 ) );
    g_message("set idle_timeout_duration: %f from %f", m_idle_timeout_duration, max_idle_timeout );
    if( m_idle_timeout_duration > max_idle_timeout || (m_idle_timeout_duration+1.0) >= max_idle_timeout ) {
      m_idle_timeout_duration = max_idle_timeout;
    }
    ev_timer_stop( m_loop, m_timeout );
    ev_timer_set( m_timeout, 0.0, m_idle_timeout_duration );
    ev_timer_again( m_loop, m_timeout ); // start the timeout again
  }
}
void Server::on_worker_exit(struct ev_loop *loop, struct ev_child *child, int revents)
{
  //Server *server = static_cast<Server*>(child->data);
  ev_child_stop(loop, child);
  g_critical("Worker process %d exited with %d", child->rpid, child->rstatus );
  // TODO: restart dead worker
  // if( server->alive() ) {
  //   server->recover_worker(child->rpid);
  // }
}
void Server::on_shutdown_signal(struct ev_loop *loop, struct ev_signal *signal, int revents)
{
  Server *server = static_cast<Server*>(signal->data);
  g_message("term signal received, starting shutdown sequence");
  server->shutdown();
}
void Server::schedule()
{
  pid_t pid = m_pids[m_last_worker];
  g_message("signal worker %d child of %d\n", pid, getpid() );

  // send signal USER1 to worker
  //if( kill(pid,SIGUSR1) ) {
//    perror("kill");
//    g_critical("Error sending work signal USR1 to %d", pid);
 // }
  WorkerChannel *channel = m_worker_channels[m_last_worker];

  if( channel ) {
    
    g_message("channel: %ld : %d", (glong)channel, channel->server_send_pipe[1]);
    char msg[2] = "w";
    int r = write(channel->server_send_pipe[1], msg, sizeof(msg));

    if( r == -1 ) {
      g_critical("Error sending worker signals");
    }

    g_message("channel message sent: %d\n", r);

  }

  //g_print("sent worker signal\n");

  ++m_last_worker;
  // round robin to worker processes
  if( m_last_worker >= m_num_pids ) { m_last_worker = 0; }
}

void Server::shutdown()
{
  gint status;
  g_message("Server:: shutting down...\n");
  ::close(m_sock);

  ev_unloop(m_loop, EVUNLOOP_ALL);

  for( int i = 0; i < m_num_pids; ++i ) {
    g_message("signal worker: %ld", m_pids[i]);
    kill(m_pids[i],SIGUSR2);
    waitpid(m_pids[i],&status,0);
    if( status ) {
      g_critical("child worker did not exit %d cleanly...", status);
    }
    //g_usleep(G_USEC_PER_SEC/100);
  }

  // sleep for half a second and send the signals again
  //g_usleep(G_USEC_PER_SEC/2);

}
Server::~Server()
{
  ev_default_destroy();

  if( m_conn_watcher ) { g_free(m_conn_watcher); }
  if( m_timeout ) { g_free(m_timeout); }
  if( m_term_sig ) { g_free(m_term_sig); }
  if( m_int_sig ) { g_free(m_int_sig); }
  if( m_db ) { delete m_db; }

  if( m_pids  ) { g_free(m_pids); }
  if( m_workers ) {
    for( int i = 0; i < m_num_pids; ++i ) {
      if( m_workers[i] ) { g_free(m_workers[i]); }
    }
  }
  // free all allocated channels
  if( m_worker_channels ) {
    for( int i = 0; i < m_num_pids; ++i ) {
      WorkerChannel *channel = m_worker_channels[i];
      delete channel;
    }
  }
}

int Server::run()
{
  int r = this->bindsock();

  if( r ) return r;

  r = fcntl(m_sock, F_SETFL, fcntl(m_sock, F_GETFL, 0) | O_NONBLOCK);
  assert(0 <= r && "socket non-blocking failed!");

  r = 0;

  ev_io_set(m_conn_watcher,m_sock, EV_READ); // | EV_ERROR);
  ev_io_start(m_loop, m_conn_watcher);
  for( int i = 0; i < m_num_pids; ++i ) {
    ev_child_start(m_loop, m_workers[i]);
  }
  ev_timer_again(m_loop, m_timeout);
  ev_signal_start(m_loop, m_term_sig);
  ev_signal_start(m_loop, m_int_sig);

  ev_loop(m_loop, 0);

  return r;
}

int Server::bindsock()
{
  struct sockaddr_in addr;
  int port = m_config.get_int("listen",4488);

  m_sock = -1;

  if( (m_sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    fprintf(stderr,"socket(): %s", strerror(errno) );
    goto error;
  }

  memset(&addr, 0, sizeof(addr));

  addr.sin_family      = AF_INET;
  addr.sin_port        = htons( port );
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(m_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr,"bind: %s", strerror(errno) );
    goto error;
  }

  g_message("Binding to %d\n", port ); 
  return 0;

error:
  if(m_sock > 0) ::close(m_sock);
  return -1;
}

}
