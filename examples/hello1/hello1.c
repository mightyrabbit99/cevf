#include "cevf_main.h"

#ifndef CEVF_CM_THR_CNT
#define CEVF_CM_THR_CNT 2
#endif // CEVF_CM_THR_CNT

static void _process_terminate(int sig) {
  cevf_terminate();
}

int main(int argc, char *argv[]) {
  cevf_register_signal_terminate(_process_terminate, NULL);
  if (cevf_init()) return -1;
  int res = cevf_start(argc, argv, CEVF_CM_THR_CNT);
  cevf_deinit();
  return res;
}
