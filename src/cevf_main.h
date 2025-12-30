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

struct cevf_initialiser_s *_cevf_ini_arr = NULL;
cevf_asz_t _cevf_ini_arr_sz = 0;
struct cevf_producer_s *_cevf_pd_arr = NULL;
cevf_asz_t _cevf_pd_arr_sz = 0;
struct cevf_consumer_s *_cevf_cm_arr = NULL;
cevf_asz_t _cevf_cm_arr_sz = 0;

#define cevf_start(cm_thr_cnt) cevf_run(_cevf_ini_arr, _cevf_ini_arr_sz, _cevf_pd_arr, _cevf_pd_arr_sz, _cevf_cm_arr, _cevf_cm_arr_sz, cm_thr_cnt);
static void __attribute__((destructor)) _cevf_main_des(void) {
  free(_cevf_ini_arr);
  _cevf_ini_arr = NULL;
  _cevf_ini_arr_sz = 0;
  free(_cevf_pd_arr);
  _cevf_pd_arr = NULL;
  _cevf_pd_arr_sz = 0;
  free(_cevf_cm_arr);
  _cevf_cm_arr = NULL;
  _cevf_cm_arr_sz = 0;
}

#endif
