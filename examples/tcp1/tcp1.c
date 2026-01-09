#include "cevf_main.h"

#define CM_THR_CNT 2

static void _process_terminate(int sig) {
  cevf_terminate();
}

int main(int argc, char *argv[]) {
  if (cevf_init()) return -1;
  cevf_register_signal_terminate(_process_terminate, NULL);
  int res = cevf_start(CM_THR_CNT);
  cevf_deinit();
  return res;
}
