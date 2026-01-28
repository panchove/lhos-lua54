
#include "lhos_lua_ntp.h"
#include "lhos_ntp.h"

int
lhos_lua_ntp_sync (lua_State *L)
{
  return lhos_ntp_sync (L);
}
int
lhos_lua_ntp_status (lua_State *L)
{
  return lhos_ntp_status (L);
}

int
lhos_lua_ntp_get_time (lua_State *L)
{
  return lhos_ntp_get_time (L);
}

int
lhos_lua_ntp_set_server (lua_State *L)
{
  return lhos_ntp_set_server (L);
}

int
lhos_lua_ntp_stop (lua_State *L)
{
  return lhos_ntp_stop (L);
}

void
lhos_lua_ntp_register (lua_State *L)
{
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_ntp_sync);
  lua_setfield (L, -2, "sync");
  lua_pushcfunction (L, lhos_lua_ntp_status);
  lua_setfield (L, -2, "status");
  lua_pushcfunction (L, lhos_lua_ntp_get_time);
  lua_setfield (L, -2, "get_time");
  lua_pushcfunction (L, lhos_lua_ntp_set_server);
  lua_setfield (L, -2, "set_server");
  lua_pushcfunction (L, lhos_lua_ntp_stop);
  lua_setfield (L, -2, "stop");
  /* Expose as global `ntp` */
  lua_setglobal (L, "ntp");
}
