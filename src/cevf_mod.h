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

extern struct cevf_initialiser_s *_cevf_ini_arr[CEVF_INI_PRIO_MAX];
extern cevf_asz_t _cevf_ini_arr_sz[CEVF_INI_PRIO_MAX];
extern struct cevf_producer_s *_cevf_pd_arr;
extern cevf_asz_t _cevf_pd_arr_sz;
extern struct cevf_consumer_s *_cevf_cm_arr;
extern cevf_asz_t _cevf_cm_arr_sz;
extern uint8_t _cevf_is_static_linked;

static struct cevf_initialiser_s *_cevf_tmp_ini_p;
static struct cevf_producer_s *_cevf_tmp_pd_p;
static struct cevf_consumer_s *_cevf_tmp_cm_p;
static int _cevf_ret = 0;

#ifdef CEVF_ALLOW_LOAD_SUBMOD
static void **_cevf_submod_arr = NULL;
static cevf_asz_t _cevf_submod_arr_sz = 0;
static void **_cevf_tmp_submod_p;
#endif  // CEVF_ALLOW_LOAD_SUBMOD

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
#ifdef CEVF_ALLOW_LOAD_SUBMOD
#define _cevf_mod_add_submod(modp)                                                                       \
  do {                                                                                                   \
    _cevf_ret = 0;                                                                                       \
    if (_cevf_submod_arr_sz == (cevf_asz_t) - 1) {                                                       \
      _cevf_ret = -1;                                                                                    \
      break;                                                                                             \
    }                                                                                                    \
    _cevf_tmp_submod_p = (void **)realloc(_cevf_submod_arr, (_cevf_submod_arr_sz + 1) * sizeof(void *)); \
    if (_cevf_tmp_submod_p == NULL) {                                                                    \
      _cevf_ret = -1;                                                                                    \
      break;                                                                                             \
    }                                                                                                    \
    _cevf_submod_arr = _cevf_tmp_submod_p;                                                               \
    _cevf_submod_arr[_cevf_submod_arr_sz++] = modp;                                                      \
  } while (0)
#define cevf_mod_add_submod(modfile)                          \
  do {                                                        \
    void *a__ = dlopen(modfile, RTLD_LAZY | RTLD_DEEPBIND);   \
    if (a__ == NULL) {                                        \
      fprintf(stderr, "Error loading cevf mod: %s\n", token); \
      break;                                                  \
    }                                                         \
    _cevf_mod_add_submod(a__);                                \
    if (_cevf_ret == -1) {                                    \
      fprintf(stderr, "cevf submod error\n");                 \
      exit(1);                                                \
    }                                                         \
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
#define cevf_is_static() (_cevf_is_static_linked == 1)

#ifdef CEVF_ALLOW_LOAD_SUBMOD
#define cevf_mod_init(function)                                                                               \
  __attribute__((section(".init_array"))) static void *_cevf_mod_init_##function = &cevf_mod_init_##function; \
  static void cevf_mod_init_##function(int argc, char *argv[]) { function(argc, argv); }
#else  // CEVF_ALLOW_LOAD_SUBMOD
#define cevf_mod_init(function) \
  static void __attribute__((constructor)) cevf_mod_init_##function(void) { function(); }
#endif  // CEVF_ALLOW_LOAD_SUBMOD

#endif  // CEVF_MOD_H
