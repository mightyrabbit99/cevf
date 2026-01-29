#include "cevf_main.h"
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdint.h>
#include "cvector.h"
#include "cvector_utils.h"

#ifndef CEVF_CM_THR_CNT
#define CEVF_CM_THR_CNT 2
#endif // CEVF_CM_THR_CNT

static void _process_terminate(int sig) {
  cevf_terminate();
}

int main(int argc, char *argv[]) {
  char *env_str;
  env_str = getenv("CEVF_CM_THR_CNT");
  uint8_t cm_thr_cnt = CEVF_CM_THR_CNT;
  if (env_str) {
    cm_thr_cnt = (uint8_t)atoi(env_str);
    if (cm_thr_cnt == 0) {
      fprintf(stderr, "Error parsing cm_thr_cnt\n");
      cm_thr_cnt = CEVF_CM_THR_CNT;
    }
  }
  env_str = getenv("CEVF_MODS");
  char *p = NULL;
  cvector_vector_type(void *) mods = NULL;
  if (env_str) {
    p = strdup(env_str);
    char *p2 = p, *token;
    while ((token = strsep(&p2, ":")) != NULL) {
      void *a = dlopen(token, RTLD_LAZY | RTLD_DEEPBIND);
      if (a == NULL) {
        fprintf(stderr, "Error loading cevf mod: %s\n", token);
        continue;
      }
      cvector_push_back(mods, a);
    }
  }

  cevf_register_signal_terminate(_process_terminate, NULL);
  if (cevf_init()) return -1;
  int res = cevf_start(argc, argv, cm_thr_cnt);
  cevf_deinit();

  cvector_for_each(mods, dlclose);
  cvector_free(mods);
  free(p);
  return res;
}
