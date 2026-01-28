#ifndef LHOS_LUA_POSIX_H
#define LHOS_LUA_POSIX_H

#include "lua.h"

int lhos_lua_posix_open (lua_State *L);
int lhos_lua_posix_read (lua_State *L);
int lhos_lua_posix_write (lua_State *L);
int lhos_lua_posix_close (lua_State *L);
int lhos_lua_posix_stat (lua_State *L);
int lhos_lua_posix_unlink (lua_State *L);
int lhos_lua_posix_rename (lua_State *L);
int lhos_lua_posix_mkdir (lua_State *L);
void lhos_lua_posix_register (lua_State *L);

#endif // LHOS_LUA_POSIX_H
