#ifndef WQ_UTIL_H
#define WQ_UTIL_H

namespace WQ {

  void daemonize();
  int set_signal(int sig, const char*name, void(*handler)(int) );

}

#endif
