#include <cevf_mod.h>
#include <dirent.h>

#include "cvector.h"
#include "cvector_utils.h"
#include "lauxlib.h"
#include "lualib.h"
#include "map.h"

#define lge(...) do {\
  fprintf(stderr, "[debug] %s %d: ", __FUNCTION__, __LINE__);\
  fprintf(stderr, __VA_ARGS__);\
} while (0)

#define CEVF_LUA_MOD_MAIN_FUNCTOIN_NAME "cevf_main"
#define CEVF_LUA_MOD_EVARR_NAME "cevf_events"

static lua_State *L = NULL;

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

static hashmap *m_evtyp_modnamelst = NULL;

static void _delete_m_evtyp_modnamelst_it(void *key, size_t ksize, uintptr_t value, void *usr) {
  cvector_vector_type(char *) modnamelst = (cvector_vector_type(char *))value;
  cvector_for_each(modnamelst, free);
  cvector_free(modnamelst);
}

static inline void delete_m_evtyp_modnamelst(hashmap *m_evtyp_modnamelst) {
  if (m_evtyp_modnamelst == NULL) return;
  hashmap_iterate(m_evtyp_modnamelst, _delete_m_evtyp_modnamelst_it, NULL);
  hashmap_free(m_evtyp_modnamelst);
}

#define _add_evtyp_modname(hmap, evtyp, modnamelst, modname)                \
  do {                                                                      \
    hashmap_get(hmap, &(evtyp), sizeof(evtyp), (uintptr_t *)&(modnamelst)); \
    cvector_push_back(modnamelst, modname);                                 \
    hashmap_set(hmap, &(evtyp), sizeof(evtyp), (uintptr_t)(modnamelst));    \
  } while (0)

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
  lua_getfield(L, -1, CEVF_LUA_MOD_EVARR_NAME);
  cevf_evtyp_t evtyp = CEVF_RESERVED_EV_THEND;
  int arrlen;
  cvector_vector_type(char *) modnamelst = NULL;
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
      _add_evtyp_modname(m_evtyp_modnamelst, evtyp, modnamelst, strdup(modname));
      lua_pop(L, 1);
    }
    if (has_valid_val == 0) {
      _add_evtyp_modname(m_evtyp_modnamelst, evtyp, modnamelst, strdup(modname));
    }
  } else {
    if (lua_isnumber(L, -1)) {
      evtyp = lua_tonumber(L, -1);
    }
    _add_evtyp_modname(m_evtyp_modnamelst, evtyp, modnamelst, strdup(modname));
  }
  lua_pop(L, 1);

  lua_setglobal(L, file2);
  free(file2);
  return 0;
not_a_module:
  lua_pop(L, 1);
  free(file2);
  return 0;
}

static int _enqueue_f(lua_State *L) {
  int res = -1;
  size_t len;
  if (!(lua_isstring(L, -2) && lua_isnumber(L, -1))) {
    lua_pushnumber(L, res);
    return 1;
  }
  const uint8_t *str = lua_tolstring(L, -2, &len);
  cevf_evtyp_t evtyp = lua_tonumber(L, -1);
  res = cevf_copy_enqueue(str, len, evtyp);
  lua_pushnumber(L, res);
  return 1;
}

static inline int _luamac_generic_evhandle(const uint8_t *data, size_t datalen, cevf_evtyp_t evtyp) {
  cvector_vector_type(char *) modnamelst = NULL;
  hashmap_get(m_evtyp_modnamelst, &(evtyp), sizeof(evtyp), (uintptr_t *)&(modnamelst));

  char **modname;
  cvector_for_each_in(modname, modnamelst) {
    lua_getglobal(L, *modname);
    if (!lua_istable(L, -1)) {
      lua_pop(L, 1);
      continue;
    }
    lua_getfield(L, -1, CEVF_LUA_MOD_MAIN_FUNCTOIN_NAME);
    if (!lua_isfunction(L, -1)) {
      lua_pop(L, 1);
      continue;
    }
    lua_pushlstring(L, data, datalen);
    lua_pushcfunction(L, _enqueue_f);
    lua_call(L, 2, 1);
    int res = lua_tonumber(L, 1);
    lua_pop(L, 1);
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
  luaL_openlibs(L);
  m_evtyp_modnamelst = hashmap_create();
  _foreach_filenode(env_str, load_lua_mod, env_str);
  return 0;
}

static void luamac_deinit(void) {
  lua_close(L);
  delete_m_evtyp_modnamelst(m_evtyp_modnamelst);
}

static inline void mod_luamac_init(void) {
  cevf_mod_add_initialiser(0, luamac_init, luamac_deinit);
  cevf_mod_add_consumer_t2_global(luamac_generic_evhandle);
}

cevf_mod_init(mod_luamac_init)
