/*
 * Copyright (c) 2025 mightyrabbit99
 *
 * License: The MIT License (MIT)
 */

#ifndef CEVF_MOD_H
#define CEVF_MOD_H

#include <cevf.h>
#include <stdio.h>
#include <stdlib.h>

extern struct cevf_initialiser_s *_cevf_ini_arr[CEVF_INI_PRIO_MAX];
extern cevf_asz_t _cevf_ini_arr_sz[CEVF_INI_PRIO_MAX];
extern struct cevf_producer_s *_cevf_pd_arr;
extern cevf_asz_t _cevf_pd_arr_sz;
extern struct cevf_consumer_s *_cevf_cm_arr;
extern cevf_asz_t _cevf_cm_arr_sz;

static struct cevf_initialiser_s *_cevf_tmp_ini_p;
static struct cevf_producer_s *_cevf_tmp_pd_p;
static struct cevf_consumer_s *_cevf_tmp_cm_p;
static int _cevf_ret = 0;

#define _cevf_mod_add_initialiser(prio, _init_f, _deinit_f)                                                                                        \
  do {                                                                                                                                             \
    _cevf_ret = 0;                                                                                                                                 \
    if (prio >= CEVF_INI_PRIO_MAX) {                                                                                                               \
      _cevf_ret = -1;                                                                                                                              \
      break;                                                                                                                                       \
    }                                                                                                                                              \
    if (_cevf_ini_arr_sz[prio] == (cevf_asz_t) - 1) {                                                                                              \
      _cevf_ret = -1;                                                                                                                              \
      break;                                                                                                                                       \
    }                                                                                                                                              \
    _cevf_tmp_ini_p = (struct cevf_initialiser_s *)realloc(_cevf_ini_arr[prio], (_cevf_ini_arr_sz[prio] + 1) * sizeof(struct cevf_initialiser_s)); \
    if (_cevf_tmp_ini_p == NULL) {                                                                                                                 \
      _cevf_ret = -1;                                                                                                                              \
      break;                                                                                                                                       \
    }                                                                                                                                              \
    _cevf_ini_arr[prio] = _cevf_tmp_ini_p;                                                                                                         \
    _cevf_ini_arr[prio][_cevf_ini_arr_sz[prio]++] = (struct cevf_initialiser_s){                                                                   \
        .init_f = _init_f,                                                                                                                         \
        .deinit_f = _deinit_f,                                                                                                                     \
    };                                                                                                                                             \
  } while (0)
#define cevf_mod_add_initialiser(prio, init_f, deinit_f) \
  do {                                                   \
    _cevf_mod_add_initialiser(prio, init_f, deinit_f);   \
    if (_cevf_ret == -1) {                               \
      fprintf(stderr, "cevf initialisation error\n");    \
      exit(1);                                           \
    }                                                    \
  } while (0)
#define _cevf_mod_add_producer(_thname, _stack_size)                                                                          \
  do {                                                                                                                        \
    _cevf_ret = 0;                                                                                                            \
    if (_cevf_pd_arr_sz == (cevf_asz_t) - 1) {                                                                                \
      _cevf_ret = -1;                                                                                                         \
      break;                                                                                                                  \
    }                                                                                                                         \
    _cevf_tmp_pd_p = (struct cevf_producer_s *)realloc(_cevf_pd_arr, (_cevf_pd_arr_sz + 1) * sizeof(struct cevf_producer_s)); \
    if (_cevf_tmp_pd_p == NULL) {                                                                                             \
      _cevf_ret = -1;                                                                                                         \
      break;                                                                                                                  \
    }                                                                                                                         \
    _cevf_pd_arr = _cevf_tmp_pd_p;                                                                                            \
    _cevf_pd_arr[_cevf_pd_arr_sz++] = (struct cevf_producer_s){                                                               \
        .thstart = CEVF_THFNAME(_thname),                                                                                     \
        .stack_size = _stack_size,                                                                                            \
        .terminator = CEVF_THENDNAME(_thname),                                                                                \
        .context = NULL,                                                                                                      \
    };                                                                                                                        \
  } while (0)
#define cevf_mod_add_producer(_thname, _stack_size) \
  do {                                              \
    _cevf_mod_add_producer(_thname, _stack_size);   \
    if (_cevf_ret == -1) {                          \
      fprintf(stderr, "cevf producer error\n");     \
      exit(1);                                      \
    }                                               \
  } while (0)
#define _cevf_mod_add_consumer(_ev_typ, _handler)                                                                             \
  do {                                                                                                                        \
    _cevf_ret = 0;                                                                                                            \
    if (_cevf_cm_arr_sz == (cevf_asz_t) - 1) {                                                                                \
      _cevf_ret = -1;                                                                                                         \
      break;                                                                                                                  \
    }                                                                                                                         \
    _cevf_tmp_cm_p = (struct cevf_consumer_s *)realloc(_cevf_cm_arr, (_cevf_cm_arr_sz + 1) * sizeof(struct cevf_consumer_s)); \
    if (_cevf_tmp_cm_p == NULL) {                                                                                             \
      _cevf_ret = (cevf_asz_t) - 1;                                                                                           \
      break;                                                                                                                  \
    }                                                                                                                         \
    _cevf_cm_arr = _cevf_tmp_cm_p;                                                                                            \
    _cevf_cm_arr[_cevf_cm_arr_sz++] = (struct cevf_consumer_s){                                                               \
        .evtyp = _ev_typ,                                                                                                     \
        .handler = _handler,                                                                                                  \
    };                                                                                                                        \
  } while (0)
#define cevf_mod_add_consumer(_ev_typ, _handler) \
  do {                                           \
    _cevf_mod_add_consumer(_ev_typ, _handler);   \
    if (_cevf_ret == -1) {                       \
      fprintf(stderr, "cevf consumer error\n");  \
      exit(1);                                   \
    }                                            \
  } while (0)
#define cevf_mod_init(function) \
  static void __attribute__((constructor)) cevf_mod_init_##function(void) { function(); }

#endif  // CEVF_MOD_H
