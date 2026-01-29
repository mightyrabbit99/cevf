#include <cevf.h>

#include "lauxlib.h"
#include "lualib.h"

int cevf_copy_enqueue_lua(lua_State *L) {
  size_t datasz;
  const char *data = lua_tolstring(L, -2, &datasz);
  cevf_evtyp_t evtyp = (cevf_evtyp_t)lua_tonumber(L, -1);
  cevf_qmsg2_res_t res = cevf_copy_enqueue(data, datasz, evtyp);
  lua_pushnumber(L, (int)res);
  return 1;
}

int cevf_copy_enqueue_soft_lua(lua_State *L) {
  size_t datasz;
  const char *data = lua_tolstring(L, -2, &datasz);
  cevf_evtyp_t evtyp = (cevf_evtyp_t)lua_tonumber(L, -1);
  cevf_qmsg2_res_t res = cevf_copy_enqueue_soft(data, datasz, evtyp);
  lua_pushnumber(L, (int)res);
  return 1;
}

// clang-format off
static const struct luaL_Reg mylib[] = {
    {"cevf_enqueue", cevf_copy_enqueue_lua},
    {"cevf_enqueue_soft", cevf_copy_enqueue_soft_lua},
    {NULL, NULL} /* sentinel */
};
// clang-format on

int luaopen_cevflua(lua_State *L) {
#if LUA_VERSION_NUM > 501
  luaL_newlib(L, mylib);
#else
  luaL_register(L, "cevflua", mylib);
#endif
  return 1;
}
