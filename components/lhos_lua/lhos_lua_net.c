
#include "lhos_lua_net.h"
#include "lauxlib.h"
#include "lhos_net.h"
#include "lua.h"

int
lhos_lua_net_connect (lua_State *L)
{
  return lhos_net_connect (L);
}

int
lhos_lua_net_disconnect (lua_State *L)
{
  return lhos_net_disconnect (L);
}

int
lhos_lua_net_send (lua_State *L)
{
  return lhos_net_send (L);
}

int
lhos_lua_net_recv (lua_State *L)
{
  return lhos_net_recv (L);
}

int
lhos_lua_net_set_use_dispatcher (lua_State *L)
{
  return lhos_net_set_use_dispatcher (L);
}

int
lhos_lua_net_set_callback (lua_State *L)
{
  return lhos_lua_register_net_callback (L);
}

void
lhos_lua_net_register (lua_State *L)
{
  /* Create `net` table and populate with functions exposed to Lua */
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_net_connect);
  lua_setfield (L, -2, "connect");
  lua_pushcfunction (L, lhos_lua_net_disconnect);
  lua_setfield (L, -2, "disconnect");
  lua_pushcfunction (L, lhos_lua_net_send);
  lua_setfield (L, -2, "send");
  lua_pushcfunction (L, lhos_lua_net_recv);
  lua_setfield (L, -2, "recv");
  lua_pushcfunction (L, lhos_lua_net_set_use_dispatcher);
  lua_setfield (L, -2, "set_use_dispatcher");
  lua_pushcfunction (L, lhos_lua_net_set_callback);
  lua_setfield (L, -2, "set_callback");
  lua_pushcfunction (L, lhos_lua_net_shutdown_all);
  lua_setfield (L, -2, "shutdown_all");
  /* Set as global `net` */
  lua_setglobal (L, "net");
}

int
lhos_lua_net_shutdown_all (lua_State *L)
{
  return lhos_net_shutdown_all (L);
}
