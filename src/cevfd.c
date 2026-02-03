#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cevf_main.h"

#ifndef CEVF_STATIC_LIB
#include <dlfcn.h>
#endif  // CEVF_STATIC_LIB

#include "cvector.h"
#include "cvector_utils.h"

#ifndef CEVF_CM_THR_CNT
#define CEVF_CM_THR_CNT 2
#endif  // CEVF_CM_THR_CNT

#ifndef CEVF_DEFAULT_MOD_PATH
#define CEVF_DEFAULT_MOD_PATH "/usr/lib/cevfms"
#endif  // CEVF_DEFAULT_MOD_PATH
#ifndef CEVFM_PREFIX
#define CEVFM_PREFIX "cevfm"
#endif  // CEVFM_PREFIX

#define lge(...) fprintf(stderr, __VA_ARGS__)
#define lg(...)                                                 \
  do {                                                          \
    fprintf(stderr, "[debug] %s %d: ", __FUNCTION__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                               \
  } while (0)

#ifndef CEVF_STATIC_LIB
struct _mod_arr_s {
  cvector_vector_type(char *) modnames;
  cvector_vector_type(void *) mods;
};

static int _add_mod_to_modarr(const char *modname, void *ctx) {
  struct _mod_arr_s *modarr = (struct _mod_arr_s *)ctx;
  cvector_push_back(modarr->modnames, strdup(modname));
  return 0;
}

static int _load_mod_modarr(const char *mpath, void *ctx) {
  struct _mod_arr_s *modarr = (struct _mod_arr_s *)ctx;
  char **it = NULL;
  cvector_for_each_in(it, modarr->modnames) {
    if (*it == NULL) continue;
    char tmp[strlen(mpath) + strlen(CEVFM_PREFIX) + strlen(*it) + 10];
    snprintf(tmp, sizeof(tmp), "%s/%s-%s.so", mpath, CEVFM_PREFIX, *it);
    void *a = dlopen(tmp, RTLD_LAZY | RTLD_DEEPBIND);
    if (a == NULL) continue;
    free(*it);
    *it = NULL;
    cvector_push_back(modarr->mods, a);
  }
}
#endif  // CEVF_STATIC_LIB

static inline void _for_each_item_in_strarr(const char *strarr, const char *sep, int (*f)(const char *, void *), void *ctx) {
  char *p = strdup(strarr);
  char *p2 = p, *token;
  while ((token = strsep(&p2, sep)) != NULL) {
    f(token, ctx);
  }
  free(p);
}

static void _process_terminate(int sig) { cevf_terminate(); }

int main(int argc, char *argv[]) {
  char *env_str;
  env_str = getenv("CEVF_CM_THR_CNT");
  uint8_t cm_thr_cnt = CEVF_CM_THR_CNT;
  if (env_str) {
    cm_thr_cnt = (uint8_t)atoi(env_str);
    if (cm_thr_cnt == 0) {
      lge("Error parsing cm_thr_cnt\n");
      cm_thr_cnt = CEVF_CM_THR_CNT;
    }
  }
#ifndef CEVF_STATIC_LIB
  struct _mod_arr_s modarr = (struct _mod_arr_s){0};
  env_str = getenv("CEVF_MODS");
  if (env_str) {
    _for_each_item_in_strarr(env_str, ":", _add_mod_to_modarr, &modarr);
  }
  env_str = getenv("CEVF_MOD_PATH");
  if (env_str == NULL) env_str = CEVF_DEFAULT_MOD_PATH;
  _for_each_item_in_strarr(env_str, ":", _load_mod_modarr, &modarr);
  cvector_for_each(modarr.modnames, free);
#endif  // CEVF_STATIC_LIB

  cevf_register_signal_terminate(_process_terminate, NULL);
  if (cevf_init()) return -1;
  cevf_add_procedures();
  int res = cevf_start(argc, argv, cm_thr_cnt);
  cevf_deinit();

#ifndef CEVF_STATIC_LIB
  cvector_for_each(modarr.mods, dlclose);
  cvector_free(modarr.modnames);
  cvector_free(modarr.mods);
#endif  // CEVF_STATIC_LIB
  return res;
}
