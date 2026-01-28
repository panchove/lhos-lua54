#include "lhos_lua_posix.h"
#include "lauxlib.h"
#include "lua.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int
push_error (lua_State *L, const char *msg)
{
  lua_pushnil (L);
  lua_pushstring (L, msg ? msg : strerror (errno));
  lua_pushinteger (L, errno);
  return 3;
}

int
lhos_lua_posix_open (lua_State *L)
{
  const char *path = luaL_checkstring (L, 1);
  int flags = (int)luaL_optinteger (L, 2, O_RDONLY);
  mode_t mode = (mode_t)luaL_optinteger (L, 3, 0644);
  int fd = open (path, flags, mode);
  if (fd < 0)
    return push_error (L, NULL);
  lua_pushinteger (L, fd);
  return 1;
}

int
lhos_lua_posix_close (lua_State *L)
{
  int fd = (int)luaL_checkinteger (L, 1);
  if (close (fd) != 0)
    return push_error (L, NULL);
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_posix_read (lua_State *L)
{
  int fd = (int)luaL_checkinteger (L, 1);
  size_t len = (size_t)luaL_checkinteger (L, 2);
  if (len == 0)
    {
      lua_pushliteral (L, "");
      return 1;
    }
  void *buf = malloc (len);
  if (!buf)
    return luaL_error (L, "out of memory");
  ssize_t r = read (fd, buf, len);
  if (r < 0)
    {
      free (buf);
      return push_error (L, NULL);
    }
  lua_pushlstring (L, (const char *)buf, (size_t)r);
  free (buf);
  return 1;
}

int
lhos_lua_posix_write (lua_State *L)
{
  int fd = (int)luaL_checkinteger (L, 1);
  size_t len;
  const char *s = luaL_checklstring (L, 2, &len);
  ssize_t w = write (fd, s, len);
  if (w < 0)
    return push_error (L, NULL);
  lua_pushinteger (L, (lua_Integer)w);
  return 1;
}

int
lhos_lua_posix_stat (lua_State *L)
{
  const char *path = luaL_checkstring (L, 1);
  struct stat st;
  if (stat (path, &st) != 0)
    return push_error (L, NULL);
  lua_newtable (L);
  lua_pushinteger (L, (lua_Integer)st.st_mode);
  lua_setfield (L, -2, "mode");
  lua_pushinteger (L, (lua_Integer)st.st_size);
  lua_setfield (L, -2, "size");
  lua_pushinteger (L, (lua_Integer)st.st_mtime);
  lua_setfield (L, -2, "mtime");
  return 1;
}

int
lhos_lua_posix_unlink (lua_State *L)
{
  const char *path = luaL_checkstring (L, 1);
  if (unlink (path) != 0)
    return push_error (L, NULL);
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_posix_rename (lua_State *L)
{
  const char *oldp = luaL_checkstring (L, 1);
  const char *newp = luaL_checkstring (L, 2);
  if (rename (oldp, newp) != 0)
    return push_error (L, NULL);
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_posix_mkdir (lua_State *L)
{
  const char *path = luaL_checkstring (L, 1);
  mode_t mode = (mode_t)luaL_optinteger (L, 2, 0755);
  if (mkdir (path, mode) != 0)
    return push_error (L, NULL);
  lua_pushboolean (L, 1);
  return 1;
}

void
lhos_lua_posix_register (lua_State *L)
{
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_posix_open);
  lua_setfield (L, -2, "open");
  lua_pushcfunction (L, lhos_lua_posix_read);
  lua_setfield (L, -2, "read");
  lua_pushcfunction (L, lhos_lua_posix_write);
  lua_setfield (L, -2, "write");
  lua_pushcfunction (L, lhos_lua_posix_close);
  lua_setfield (L, -2, "close");
  lua_pushcfunction (L, lhos_lua_posix_stat);
  lua_setfield (L, -2, "stat");
  lua_pushcfunction (L, lhos_lua_posix_unlink);
  lua_setfield (L, -2, "unlink");
  lua_pushcfunction (L, lhos_lua_posix_rename);
  lua_setfield (L, -2, "rename");
  lua_pushcfunction (L, lhos_lua_posix_mkdir);
  lua_setfield (L, -2, "mkdir");
  lua_setglobal (L, "posix");
}
