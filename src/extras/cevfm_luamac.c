#include <cevf_mod.h>
#include <dirent.h>
#include <pthread.h>

#include "cvector.h"
#include "cvector_utils.h"
#include "lauxlib.h"
#include "lualib.h"
#include "map.h"

#define lge(...)                                                \
  do {                                                          \
    fprintf(stderr, "[debug] %s %d: ", __FUNCTION__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                               \
  } while (0)

#define CEVF_LUA_MOD_MAIN_FUNCTOIN_NAME "cevf_main"
#define CEVF_LUA_MOD_EVARR_NAME "cevf_events"
#define CEVF_LUA_MOD_PCDNO_NAME "cevf_procedure_no"
#define CEVF_LUA_MOD_ENQF_NAME "cevf_enqueue"
#define CEVF_LUA_MOD_EXECPCD_NAME "cevf_exec_procedure"

static lua_State *L = NULL;
static pthread_mutex_t L_mutex;

static inline int _foreach_filenode(const char *path, int (*f)(const char *, void *), void *ctx) {
  int ret = 0;
  DIR *dp;
  struct dirent *ep;
  dp = opendir(path);
  if (dp == NULL) {
    lge("Couldn't open the directory %s", path);
    return -1;
  }
  while ((ep = readdir(dp)) != NULL) {
    if (ep->d_name[0] == '.') continue;
    ret = f(ep->d_name, ctx);
    if (ret < 0) break;
  }

  closedir(dp);
  return ret;
}

struct _fnode_arr_s {
  char **arr;
  size_t arrlen;
};

static int _add_fnode_to_array(const char *name, void *ctx) {
  struct _fnode_arr_s *arr = (struct _fnode_arr_s *)ctx;
  char **arr2 = (char **)realloc(arr->arr, arr->arrlen + 1);
  if (arr2 == NULL) return -1;
  arr->arr = arr2;
  arr->arr[arr->arrlen++] = strdup(name);
}

static int pstrcmp(const void* a, const void* b) {
  return strcmp(*(const char**)a, *(const char**)b);
}

static inline int _foreach_filenode_sorted(const char *path, int (*f)(const char *, void *), void *ctx) {
  struct _fnode_arr_s arr1 = (struct _fnode_arr_s){0};
  _foreach_filenode(path, _add_fnode_to_array, (void *)&arr1);
  qsort(arr1.arr, arr1.arrlen, sizeof(arr1.arr[0]), pstrcmp);
  for (size_t i = 0; i < arr1.arrlen; i++) {
    f(arr1.arr[i], ctx);
    free(arr1.arr[i]);
  }
  free(arr1.arr);
}

static int _enqueue_f(lua_State *L) {
  int res = -1;
  size_t len;
  if (!(lua_isstring(L, -2) && lua_isnumber(L, -1))) {
    return 0;
  }
  const uint8_t *str = lua_tolstring(L, -2, &len);
  cevf_evtyp_t evtyp = lua_tonumber(L, -1);
  res = cevf_copy_enqueue(str, len, evtyp);
  lua_pushnumber(L, res);
  return 1;
}

static int _execpcd_f(lua_State *L) {
  size_t len, reslen;
  if (!(lua_isstring(L, -1) && lua_isnumber(L, -2))) {
    return 0;
  }
  cevf_pcdno_t pcdno = lua_tonumber(L, -2);
  const uint8_t *str = lua_tolstring(L, -1, &len);
  uint8_t *res = cevf_exec_procedure_t2(pcdno, str, len, &reslen);
  lua_pushlstring(L, res, reslen);
  free(res);
  return 1;
}

static inline int _exec_mainf_t1(lua_State *L, const char *modname, const uint8_t *data, size_t datalen) {
  pthread_mutex_lock(&L_mutex);
  lua_getglobal(L, modname);
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 0;
  }
  lua_getfield(L, -1, CEVF_LUA_MOD_MAIN_FUNCTOIN_NAME);
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 1);
    return 0;
  }
  lua_pushlstring(L, data, datalen);
  lua_call(L, 1, 1);
  if (!lua_isnumber(L, -1)) {
    lua_pop(L, 1);
    return 0;
  }
  int res = lua_tonumber(L, -1);
  lua_pop(L, 1);
  pthread_mutex_unlock(&L_mutex);
  return res;
}

static inline char *_exec_mainf_t2(lua_State *L, const char *modname, const uint8_t *data, size_t datalen, size_t *outlen) {
  pthread_mutex_lock(&L_mutex);
  lua_getglobal(L, modname);
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 0;
  }
  lua_getfield(L, -1, CEVF_LUA_MOD_MAIN_FUNCTOIN_NAME);
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 1);
    return 0;
  }
  lua_pushlstring(L, data, datalen);
  lua_call(L, 1, 1);
  if (!lua_isstring(L, -1)) {
    lua_pop(L, 1);
    return NULL;
  }
  const char *res2 = lua_tolstring(L, -1, outlen);
  if (res2 == NULL) {
    lua_pop(L, 1);
    return NULL;
  }
  char *res = (char *)malloc(*outlen);
  if (res == NULL) {
    lua_pop(L, 1);
    return NULL;
  }
  memcpy(res, res2, *outlen);
  lua_pop(L, 1);
  pthread_mutex_unlock(&L_mutex);
  return res;
}


static hashmap *m_evtyp_modnamelst = NULL;
static pthread_mutex_t m_evtyp_modnamelst_mutex;
static hashmap *m_pcdno_modname = NULL;
static pthread_mutex_t m_pcdno_modname_mutex;

static char *luamac_generic_ctx = "luamac_generic_ctx";
static uint8_t *luamac_generic_procedure_t2(const uint8_t *input, size_t input_len, size_t *output_len, void *ctx) {
  char *ctx2 = (char *)ctx;
  cevf_pcdno_t pcdno = (cevf_pcdno_t)(ctx2 - luamac_generic_ctx);
  char *modname;
  pthread_mutex_lock(&m_pcdno_modname_mutex);
  if (!hashmap_get(m_pcdno_modname, &pcdno, sizeof(pcdno), (uintptr_t *)&(modname))) {
    pthread_mutex_unlock(&m_pcdno_modname_mutex);
    return NULL;
  }
  pthread_mutex_unlock(&m_pcdno_modname_mutex);

  return (uint8_t *)_exec_mainf_t2(L, modname, input, input_len, output_len);
}

static void _delete_m_typ_modnamelst_it(void *key, size_t ksize, uintptr_t value, void *usr) {
  cvector_vector_type(char *) modnamelst = (cvector_vector_type(char *))value;
  cvector_for_each(modnamelst, free);
  cvector_free(modnamelst);
}

static inline void delete_m_typ_modnamelst(hashmap *m_typ_modnamelst) {
  if (m_typ_modnamelst == NULL) return;
  hashmap_iterate(m_typ_modnamelst, _delete_m_typ_modnamelst_it, NULL);
  hashmap_free(m_typ_modnamelst);
}

static void _delete_m_typ_modname_it(void *key, size_t ksize, uintptr_t value, void *usr) {
  char *modname = (char *)value;
  free(modname);
}

static inline void delete_m_typ_modname(hashmap *m_typ_modname) {
  if (m_typ_modname == NULL) return;
  hashmap_iterate(m_typ_modname, _delete_m_typ_modname_it, NULL);
  hashmap_free(m_typ_modname);
}

#define _add_typ_modname(hmap, typ, modname)                       \
  do {                                                             \
    cvector_vector_type(char *) lst__ = NULL;                      \
    hashmap_get(hmap, &(typ), sizeof(typ), (uintptr_t *)&(lst__)); \
    cvector_push_back(lst__, modname);                             \
    hashmap_set(hmap, &(typ), sizeof(typ), (uintptr_t)(lst__));    \
  } while (0)

static inline int _load_lua_mod_register_ev(lua_State *L, const char *_modname) {
  lua_getfield(L, -1, CEVF_LUA_MOD_EVARR_NAME);
  cevf_evtyp_t evtyp = CEVF_RESERVED_EV_THEND;
  int arrlen;
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);
    return 0;
  }
  char *modname = strdup(_modname);
  if (modname == NULL) return -1;
  if (lua_istable(L, -1) && (arrlen = lua_objlen(L, -1)) > 0) {
    uint8_t has_valid_val = 0;
    for (int i = 0; i < arrlen; i++) {
      lua_rawgeti(L, -1, i);
      if (!lua_isnumber(L, -1)) {
        lua_pop(L, 1);
        continue;
      }
      has_valid_val = 1;
      evtyp = lua_tonumber(L, -1);
      _add_typ_modname(m_evtyp_modnamelst, evtyp, modname);
      lua_pop(L, 1);
    }
    if (has_valid_val == 0) {
      _add_typ_modname(m_evtyp_modnamelst, evtyp, modname);
    }
  } else {
    if (lua_isnumber(L, -1)) {
      evtyp = lua_tonumber(L, -1);
    }
    _add_typ_modname(m_evtyp_modnamelst, evtyp, modname);
  }
  lua_pop(L, 1);
  return 0;
}

static inline int _load_lua_mod_register_pcd(lua_State *L, const char *_modname) {
  lua_getfield(L, -1, CEVF_LUA_MOD_PCDNO_NAME);
  if (!lua_isnumber(L, -1)) {
    lua_pop(L, 1);
    return 0;
  }
  char *modname = strdup(_modname);
  char *modname2 = NULL;
  if (modname == NULL) return -1;
  cevf_pcdno_t pcdno = lua_tonumber(L, -1);
  if (hashmap_get(m_pcdno_modname, &pcdno, sizeof(pcdno), (uintptr_t *)&(modname2))) {
    free(modname2);
  }
  hashmap_set(m_pcdno_modname, &pcdno, sizeof(pcdno), (uintptr_t)modname);
  cevf_add_procedure_t2((struct cevf_procedure_t2_s){
    .pcdno = pcdno,
    .func = luamac_generic_procedure_t2,
    .ctx = luamac_generic_ctx + pcdno,
  });
  return 0;
}

static int load_lua_mod(const char *file, void *ctx) {
  char *luamod_path = (char *)ctx;
  char *file2 = strdup(file);
  char *p = strrchr(file2, '.');
  if (p == NULL || strcmp(p, ".lua") != 0) {
    free(file2);
    return 0;
  }
  char tmp[strlen(luamod_path) + strlen(file2) + 1];
  snprintf(tmp, sizeof(tmp), "%s/%s", luamod_path, file2);
  luaL_loadfile(L, tmp);
  lua_pcall(L, 0, LUA_MULTRET, 0);
  lua_pushvalue(L, -1);
  if (!lua_istable(L, -1)) return 0;

  *p = '\0';
  char *modname = file2;

  lua_getfield(L, -1, CEVF_LUA_MOD_MAIN_FUNCTOIN_NAME);
  if (!lua_isfunction(L, -1)) goto not_a_module;
  lua_pop(L, 1);

  _load_lua_mod_register_ev(L, modname);
  _load_lua_mod_register_pcd(L, modname);

  lua_setglobal(L, file2);
  free(file2);
  return 0;
not_a_module:
  lua_pop(L, 1);
  free(file2);
  return 0;
}

static inline int _luamac_generic_evhandle(const uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  cvector_vector_type(char *) modnamelst = NULL;
  pthread_mutex_lock(&m_evtyp_modnamelst_mutex);
  hashmap_get(m_evtyp_modnamelst, &(evtyp), sizeof(evtyp), (uintptr_t *)&(modnamelst));
  pthread_mutex_unlock(&m_evtyp_modnamelst_mutex);

  char **modname;
  cvector_for_each_in(modname, modnamelst) {
    int res = _exec_mainf_t1(L, *modname, data, datalen);
    if (res) return res;
  }
  return 0;
}

static int luamac_generic_evhandle(const uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  int res;
  res = _luamac_generic_evhandle(data, datalen, evtyp);
  if (res) return res;
  res = _luamac_generic_evhandle(data, datalen, CEVF_RESERVED_EV_THEND);
  return res;
}

static int luamac_init(int argc, char *argv[]) {
  char *env_str;
  env_str = getenv("CEVF_LUAMOD_PATH");
  if (env_str == NULL) return 0;
  L = luaL_newstate();
  pthread_mutex_init(&L_mutex, NULL);
  luaL_openlibs(L);
  lua_pushcfunction(L, _enqueue_f);
  lua_setglobal(L, CEVF_LUA_MOD_ENQF_NAME);
  lua_pushcfunction(L, _execpcd_f);
  lua_setglobal(L, CEVF_LUA_MOD_EXECPCD_NAME);
  m_evtyp_modnamelst = hashmap_create();
  pthread_mutex_init(&m_evtyp_modnamelst_mutex, NULL);
  m_pcdno_modname = hashmap_create();
  pthread_mutex_init(&m_pcdno_modname_mutex, NULL);
  _foreach_filenode_sorted(env_str, load_lua_mod, env_str);
  return 0;
}

static void luamac_deinit(void) {
  lua_close(L);
  pthread_mutex_destroy(&L_mutex);
  delete_m_typ_modnamelst(m_evtyp_modnamelst);
  pthread_mutex_destroy(&m_evtyp_modnamelst_mutex);
  delete_m_typ_modname(m_pcdno_modname);
  pthread_mutex_destroy(&m_pcdno_modname_mutex);
}

static inline void mod_luamac_init(void) {
  cevf_mod_add_initialiser(0, luamac_init, luamac_deinit);
  cevf_mod_add_consumer_t2_global(luamac_generic_evhandle);
}

cevf_mod_init(mod_luamac_init)
