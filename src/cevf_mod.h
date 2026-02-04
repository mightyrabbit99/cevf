/*
 * Copyright (c) 2025 Tan Wah Ken
 *
 * License: The MIT License (MIT)
 */

#ifndef CEVF_MOD_H
#define CEVF_MOD_H

#include <cevf.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef CEVF_ALLOW_LOAD_SUBMOD
#include <dlfcn.h>
#endif  // CEVF_ALLOW_LOAD_SUBMOD

#define CEVF_PASTER(x, y) x##_##y
#define CEVF_CONCAT(x, y) CEVF_PASTER(x, y)

#ifndef CEVF_THF_ARGNAME
#define CEVF_THF_ARGNAME argname__
#endif  // CEVF_THF_ARGNAME
#define CEVF_THFDECL(fname) void *CEVF_THFNAME(fname)(void *CEVF_THF_ARGNAME)
#define CEVF_THFNAME(fname) CEVF_CONCAT(fname, th)
#define CEVF_THENDDECL(fname) void CEVF_THENDNAME(fname)(void *CEVF_THF_ARGNAME)
#define CEVF_THENDNAME(fname) CEVF_CONCAT(fname, terminate)

extern struct cevf_initialiser_s *_cevf_ini_arr[CEVF_INI_PRIO_MAX];
extern cevf_asz_t _cevf_ini_arr_sz[CEVF_INI_PRIO_MAX];
extern struct cevf_producer_s *_cevf_pd_arr;
extern cevf_asz_t _cevf_pd_arr_sz;
extern struct cevf_consumer_t1_s *_cevf_cm_t1_arr;
extern cevf_asz_t _cevf_cm_t1_arr_sz;
extern struct cevf_consumer_t2_s *_cevf_cm_t2_arr;
extern cevf_asz_t _cevf_cm_t2_arr_sz;
extern struct cevf_procedure_t1_s *_cevf_pcd_t1_arr;
extern cevf_asz_t _cevf_pcd_t1_arr_sz ;
extern struct cevf_procedure_t2_s *_cevf_pcd_t2_arr;
extern cevf_asz_t _cevf_pcd_t2_arr_sz ;

extern uint8_t _cevf_is_static_linked;
extern uint8_t _cevf_cm_thr_cnt;
extern size_t _cevf_data_mq_sz;
extern size_t _cevf_ctrl_mq_sz;

static struct cevf_initialiser_s *_cevf_tmp_ini_p;
static struct cevf_producer_s *_cevf_tmp_pd_p;
static struct cevf_consumer_t1_s *_cevf_tmp_cm_t1_p;
static struct cevf_consumer_t2_s *_cevf_tmp_cm_t2_p;
static struct cevf_procedure_t1_s *_cevf_tmp_pcd_t1_p;
static struct cevf_procedure_t2_s *_cevf_tmp_pcd_t2_p;
static int _cevf_ret = 0;

#ifdef CEVF_ALLOW_LOAD_SUBMOD
static void **_cevf_submod_arr = NULL;
static cevf_asz_t _cevf_submod_arr_sz = 0;
static void **_cevf_tmp_submod_p;
#endif  // CEVF_ALLOW_LOAD_SUBMOD

#define __cevf_add_list_item(arr, arr_sz, p, item_typ, item)             \
  do {                                                                   \
    _cevf_ret = 0;                                                       \
    if ((arr_sz) == (cevf_asz_t) - 1) {                                  \
      _cevf_ret = -1;                                                    \
      break;                                                             \
    }                                                                    \
    (p) = (item_typ *)realloc((arr), ((arr_sz) + 1) * sizeof(item_typ)); \
    if ((p) == NULL) {                                                   \
      _cevf_ret = -1;                                                    \
      break;                                                             \
    }                                                                    \
    (arr) = (p);                                                         \
    (arr)[arr_sz++] = (item_typ)(item);                                  \
  } while (0)
#define cevf_mod_add_initialiser(prio, _init_f, _deinit_f)                                                                 \
  do {                                                                                                                     \
    struct cevf_initialiser_s item__ = (struct cevf_initialiser_s){                                                        \
        .init_f = _init_f,                                                                                                 \
        .deinit_f = _deinit_f,                                                                                             \
    };                                                                                                                     \
    __cevf_add_list_item(_cevf_ini_arr[prio], _cevf_ini_arr_sz[prio], _cevf_tmp_ini_p, struct cevf_initialiser_s, item__); \
    if (_cevf_ret == -1) {                                                                                                 \
      fprintf(stderr, "cevf initialisation error\n");                                                                      \
      exit(1);                                                                                                             \
    }                                                                                                                      \
  } while (0)
#define cevf_mod_add_producer(_thname, _stack_size)                                                      \
  do {                                                                                                   \
    struct cevf_producer_s item__ = (struct cevf_producer_s){                                            \
        .thstart = CEVF_THFNAME(_thname),                                                                \
        .stack_size = _stack_size,                                                                       \
        .terminator = CEVF_THENDNAME(_thname),                                                           \
        .context = NULL,                                                                                 \
    };                                                                                                   \
    __cevf_add_list_item(_cevf_pd_arr, _cevf_pd_arr_sz, _cevf_tmp_pd_p, struct cevf_producer_s, item__); \
    if (_cevf_ret == -1) {                                                                               \
      fprintf(stderr, "cevf producer error\n");                                                          \
      exit(1);                                                                                           \
    }                                                                                                    \
  } while (0)
#define cevf_mod_add_consumer_t1(_ev_typ, _handler)                                                                  \
  do {                                                                                                               \
    struct cevf_consumer_t1_s item__ = (struct cevf_consumer_t1_s){                                                  \
        .evtyp = _ev_typ,                                                                                            \
        .handler = _handler,                                                                                         \
    };                                                                                                               \
    __cevf_add_list_item(_cevf_cm_t1_arr, _cevf_cm_t1_arr_sz, _cevf_tmp_cm_t1_p, struct cevf_consumer_t1_s, item__); \
    if (_cevf_ret == -1) {                                                                                           \
      fprintf(stderr, "cevf consumer error\n");                                                                      \
      exit(1);                                                                                                       \
    }                                                                                                                \
  } while (0)
#define cevf_mod_add_consumer_t1_global(_handler) cevf_mod_add_consumer_t1(CEVF_RESERVED_EV_THEND, _handler)
#define cevf_mod_add_consumer_t2(_ev_typ, _handler)                                                                  \
  do {                                                                                                               \
    struct cevf_consumer_t2_s item__ = (struct cevf_consumer_t2_s){                                                  \
        .evtyp = _ev_typ,                                                                                            \
        .handler = _handler,                                                                                         \
    };                                                                                                               \
    __cevf_add_list_item(_cevf_cm_t2_arr, _cevf_cm_t2_arr_sz, _cevf_tmp_cm_t2_p, struct cevf_consumer_t2_s, item__); \
    if (_cevf_ret == -1) {                                                                                           \
      fprintf(stderr, "cevf consumer error\n");                                                                      \
      exit(1);                                                                                                       \
    }                                                                                                                \
  } while (0)
#define cevf_mod_add_consumer_t2_global(_handler) cevf_mod_add_consumer_t2(CEVF_RESERVED_EV_THEND, _handler)
#define cevf_mod_add_procedure_t1(_pcdno, _f)                                                                            \
  do {                                                                                                                   \
    struct cevf_procedure_t1_s item__ = (struct cevf_procedure_t1_s){                                                    \
        .pcdno = _pcdno,                                                                                                 \
        .vfunc = _f,                                                                                                     \
    };                                                                                                                   \
    __cevf_add_list_item(_cevf_pcd_t1_arr, _cevf_pcd_t1_arr_sz, _cevf_tmp_pcd_t1_p, struct cevf_procedure_t1_s, item__); \
    if (_cevf_ret == -1) {                                                                                               \
      fprintf(stderr, "cevf procedure error\n");                                                                         \
      exit(1);                                                                                                           \
    }                                                                                                                    \
  } while (0)
#define cevf_mod_add_procedure_t2(_pcdno, _f, _ctx)                                                                      \
  do {                                                                                                                   \
    struct cevf_procedure_t2_s item__ = (struct cevf_procedure_t2_s){                                                    \
        .pcdno = _pcdno,                                                                                                 \
        .func = _f,                                                                                                      \
        .ctx = _ctx,                                                                                                     \
    };                                                                                                                   \
    __cevf_add_list_item(_cevf_pcd_t2_arr, _cevf_pcd_t2_arr_sz, _cevf_tmp_pcd_t2_p, struct cevf_procedure_t2_s, item__); \
    if (_cevf_ret == -1) {                                                                                               \
      fprintf(stderr, "cevf procedure error\n");                                                                         \
      exit(1);                                                                                                           \
    }                                                                                                                    \
  } while (0)

#ifdef CEVF_ALLOW_LOAD_SUBMOD
#define cevf_mod_add_submod(modfile)                                                              \
  do {                                                                                            \
    void *a__ = dlopen(modfile, RTLD_LAZY | RTLD_DEEPBIND);                                       \
    if (a__ == NULL) {                                                                            \
      fprintf(stderr, "Error loading cevf mod: %s\n", token);                                     \
      break;                                                                                      \
    }                                                                                             \
    __cevf_add_list_item(_cevf_submod_arr, _cevf_submod_arr_sz, _cevf_tmp_submod_p, void *, a__); \
    if (_cevf_ret == -1) {                                                                        \
      fprintf(stderr, "cevf submod error\n");                                                     \
      exit(1);                                                                                    \
    }                                                                                             \
  } while (0)
static void __attribute__((destructor)) _cevf_mod_deinit(void) {
  for (cevf_asz_t i = 0; i < _cevf_submod_arr_sz; i++) {
    dlclose(_cevf_submod_arr[i]);
    _cevf_submod_arr[i] = NULL;
  }
  free(_cevf_submod_arr);
  _cevf_submod_arr = NULL;
  _cevf_submod_arr_sz = 0;
}
#endif  // CEVF_ALLOW_LOAD_SUBMOD
#define cevf_mod_set_consumer_thread_count(count) (_cevf_cm_thr_cnt = (count))
#define cevf_mod_set_data_mq_size(size) (_cevf_data_mq_sz = (size))
#define cevf_mod_set_ctrl_mq_size(size) (_cevf_ctrl_mq_sz = (size))
#define cevf_is_static() (_cevf_is_static_linked == 1)

#ifdef CEVF_STATIC_INIT
#define cevf_mod_init(function)                                  \
  static void cevf_mod_init_##function(int argc, char *argv[]) { \
    function();                                                  \
  }                                                              \
  __attribute__((section(".init_array"))) static void *_cevf_mod_init_##function = &cevf_mod_init_##function;
#else  // CEVF_STATIC_INIT
#define cevf_mod_init(function)                                             \
  static void __attribute__((constructor)) cevf_mod_init_##function(void) { \
    function();                                                             \
  }
#endif  // CEVF_STATIC_INIT

#endif  // CEVF_MOD_H
