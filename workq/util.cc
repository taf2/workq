#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <glib.h>
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

namespace WQ {

void daemonize()
{
  // see: http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html
  int fd;
  pid_t pid, sid;

  // Fork off the parent process
  pid = fork();
  if (pid < 0) {
    perror("Failed to daemonize while forking from parent");
    exit(EXIT_FAILURE);
  }

  // If we got a good PID, then
  // we can exit the parent process.
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  // Change the file mode mask
  umask(0);       
 
  // Open any logs here
  
  // Create a new SID for the child process
  sid = setsid();
  if (sid < 0) {
    // Log any failures here
    perror("Failed to daemonize while setting child session");
    exit(EXIT_FAILURE);
  }
  
  // Change the current working directory
  if ((chdir("/")) < 0) {
    // Log any failures here
    perror("Failed to daemonize while changing to /");
    exit(EXIT_FAILURE);
  }

  fflush(stdin);
  fflush(stdout);
  fflush(stderr);

  // see: http://code.sixapart.com/trac/memcached/browser/branches/facebook/daemon.c?rev=288
  fd = open("/dev/null",O_RDWR,0);
  if( fd != -1 ) {
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if( fd > STDERR_FILENO ) {
      close(fd);
    }
  }
  else {
    // Close out the standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO); 
  }
}

int set_signal(int sig, const char*name, void(*handler)(int) )
{
  struct sigaction sa;
  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  if (sigaction(sig, &sa, NULL) < 0) {
    g_print("sigaction(%s): '%s'", name, strerror(errno) );
    return 1;
  }
  return 0;
}

}
