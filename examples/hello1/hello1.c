#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "argum.h"
#include "cevf_main.h"

#define CM_THR_CNT 2

void _kill_handler(int sig) {
  cevf_terminate();
}

int main(int argc, char *argv[]) {
  signal(SIGINT, _kill_handler);
  signal(SIGTERM, _kill_handler);
  if (cevf_init()) return -1;
  int res = cevf_start(CM_THR_CNT);
  cevf_deinit();
  return res;
}
