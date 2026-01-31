/*
 * Copyright (c) 2025 Tan Wah Ken
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
struct cevf_consumer_t1_s *_cevf_cm_t1_arr;
cevf_asz_t _cevf_cm_t1_arr_sz = 0;
struct cevf_consumer_t2_s *_cevf_cm_t2_arr;
cevf_asz_t _cevf_cm_t2_arr_sz = 0;
struct cevf_procedure_s *_cevf_pcd_arr;
cevf_asz_t _cevf_pcd_arr_sz = 0;

#ifdef CEVF_STATIC_LIB
uint8_t _cevf_is_static_linked = 1;
#else   // CEVF_STATIC_LIB
uint8_t _cevf_is_static_linked = 0;
#endif  // CEVF_STATIC_LIB

#define cevf_start(argc, argv, cm_thr_cnt)                                                                                                                                                  \
  cevf_run(argc, argv, _cevf_ini_arr, _cevf_ini_arr_sz, _cevf_pd_arr, _cevf_pd_arr_sz, _cevf_cm_t1_arr, _cevf_cm_t1_arr_sz, _cevf_cm_t2_arr, _cevf_cm_t2_arr_sz, cm_thr_cnt, _cevf_pcd_arr, \
           _cevf_pcd_arr_sz);
static void __attribute__((destructor)) _cevf_main_des(void) {
  for (size_t i = 0; i < CEVF_INI_PRIO_MAX; i++) {
    free(_cevf_ini_arr[i]);
    _cevf_ini_arr[i] = NULL;
    _cevf_ini_arr_sz[i] = 0;
  }
  free(_cevf_pd_arr);
  _cevf_pd_arr = NULL;
  _cevf_pd_arr_sz = 0;
  free(_cevf_cm_t1_arr);
  _cevf_cm_t1_arr = NULL;
  _cevf_cm_t1_arr_sz = 0;
  free(_cevf_cm_t2_arr);
  _cevf_cm_t2_arr = NULL;
  _cevf_cm_t2_arr_sz = 0;
  free(_cevf_pcd_arr);
  _cevf_pcd_arr = NULL;
  _cevf_pcd_arr_sz = 0;
}

#endif
