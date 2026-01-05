/*
 * Copyright (c) 2025 mightyrabbit99
 *
 * License: The MIT License (MIT)
 */

#ifndef CEVF_MAIN_H
#define CEVF_MAIN_H

#include <cevf.h>
#include <stdint.h>
#include <stdlib.h>

struct cevf_initialiser_s *_cevf_ini_arr[CEVF_INI_PRIO_MAX];
cevf_asz_t _cevf_ini_arr_sz[CEVF_INI_PRIO_MAX];
struct cevf_producer_s *_cevf_pd_arr = NULL;
cevf_asz_t _cevf_pd_arr_sz = 0;
struct cevf_consumer_s *_cevf_cm_arr = NULL;
cevf_asz_t _cevf_cm_arr_sz = 0;

#define cevf_start(cm_thr_cnt) cevf_run(_cevf_ini_arr, _cevf_ini_arr_sz, _cevf_pd_arr, _cevf_pd_arr_sz, _cevf_cm_arr, _cevf_cm_arr_sz, cm_thr_cnt);
static void __attribute__((constructor)) _cevf_main_cons(void) {
  for (size_t i = 0; i < CEVF_INI_PRIO_MAX; i++) {
    _cevf_ini_arr[i] = NULL;
    _cevf_ini_arr_sz[i] = 0;
  }
}
static void __attribute__((destructor)) _cevf_main_des(void) {
  for (size_t i = 0; i < CEVF_INI_PRIO_MAX; i++) {
    free(_cevf_ini_arr[i]);
    _cevf_ini_arr[i] = NULL;
    _cevf_ini_arr_sz[i] = 0;
  }
  free(_cevf_pd_arr);
  _cevf_pd_arr = NULL;
  _cevf_pd_arr_sz = 0;
  free(_cevf_cm_arr);
  _cevf_cm_arr = NULL;
  _cevf_cm_arr_sz = 0;
}

#endif
