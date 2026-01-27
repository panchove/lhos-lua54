#ifndef LHOS_LUA_H
#define LHOS_LUA_H

#include <stdbool.h>

void lhos_lua_init (void);
int lhos_lua_run_script (const char *script);
bool lhos_lua_enabled (void);

/*
 * Configuration macros to guide the lhos Lua allocator.
 * If CONFIG_LUA_USE_SPIRAM is set in sdkconfig, lhos should allocate
 * the Lua heap using MALLOC_CAP_SPIRAM via pvPortMallocCaps.
 */
#ifdef CONFIG_LUA_USE_SPIRAM
#define LHOS_LUA_ALLOC_SPIRAM 1
#else
#define LHOS_LUA_ALLOC_SPIRAM 0
#endif

#endif /* LHOS_LUA_H */
