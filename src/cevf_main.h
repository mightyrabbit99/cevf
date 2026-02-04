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

#ifndef CEVF_CM_THR_CNT
#define CEVF_CM_THR_CNT 2
#endif  // CEVF_CM_THR_CNT
#ifndef CEVF_DATA_MQ_SZ
#define CEVF_DATA_MQ_SZ 10
#endif  // CEVF_DATA_MQ_SZ
#ifndef CEVF_CTRL_MQ_SZ
#define CEVF_CTRL_MQ_SZ 10
#endif  // CEVF_CTRL_MQ_SZ

struct cevf_initialiser_s *_cevf_ini_arr[CEVF_INI_PRIO_MAX];
cevf_asz_t _cevf_ini_arr_sz[CEVF_INI_PRIO_MAX];
struct cevf_producer_s *_cevf_pd_arr = NULL;
cevf_asz_t _cevf_pd_arr_sz = 0;
struct cevf_consumer_t1_s *_cevf_cm_t1_arr;
cevf_asz_t _cevf_cm_t1_arr_sz = 0;
struct cevf_consumer_t2_s *_cevf_cm_t2_arr;
cevf_asz_t _cevf_cm_t2_arr_sz = 0;
struct cevf_procedure_t1_s *_cevf_pcd_t1_arr;
cevf_asz_t _cevf_pcd_t1_arr_sz = 0;
struct cevf_procedure_t2_s *_cevf_pcd_t2_arr;
cevf_asz_t _cevf_pcd_t2_arr_sz = 0;

uint8_t _cevf_cm_thr_cnt = CEVF_CM_THR_CNT;
size_t _cevf_data_mq_sz = CEVF_DATA_MQ_SZ;
size_t _cevf_ctrl_mq_sz = CEVF_CTRL_MQ_SZ;

#ifdef CEVF_STATIC_LIB
uint8_t _cevf_is_static_linked = 1;
#else   // CEVF_STATIC_LIB
uint8_t _cevf_is_static_linked = 0;
#endif  // CEVF_STATIC_LIB

static inline void cevf_add_procedures(void) {
  for (cevf_asz_t i = 0; i < _cevf_pcd_t1_arr_sz; i++)
    cevf_add_procedure_t1(_cevf_pcd_t1_arr[i]);
  for (cevf_asz_t i = 0; i < _cevf_pcd_t2_arr_sz; i++)
    cevf_add_procedure_t2(_cevf_pcd_t2_arr[i]);
}

#define cevf_init() cevf_init(_cevf_data_mq_sz)
#define cevf_start(argc, argv)                                      \
  cevf_run(argc, argv,                                              \
           (struct cevf_run_arg_s){.ini_arr = _cevf_ini_arr,        \
                                   .ini_num = _cevf_ini_arr_sz,     \
                                   .pd_arr = _cevf_pd_arr,          \
                                   .pd_num = _cevf_pd_arr_sz,       \
                                   .cm_t1_arr = _cevf_cm_t1_arr,    \
                                   .cm_t1_num = _cevf_cm_t1_arr_sz, \
                                   .cm_t2_arr = _cevf_cm_t2_arr,    \
                                   .cm_t2_num = _cevf_cm_t2_arr_sz, \
                                   .cm_thr_cnt = _cevf_cm_thr_cnt,  \
                                   .ctrlmqsz = _cevf_ctrl_mq_sz})
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
  free(_cevf_pcd_t1_arr);
  _cevf_pcd_t1_arr = NULL;
  _cevf_pcd_t1_arr_sz = 0;
  free(_cevf_pcd_t2_arr);
  _cevf_pcd_t2_arr = NULL;
  _cevf_pcd_t2_arr_sz = 0;
}

#endif
