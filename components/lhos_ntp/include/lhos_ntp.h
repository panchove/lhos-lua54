/* LHOS NTP bindings - C side
 * Exposes `lhos_ntp_sync(lua_State*)` and `lhos_ntp_status(lua_State*)`
 */
#ifndef LHOS_NTP_H
#define LHOS_NTP_H

#ifndef LUA_STATE_DECL
typedef struct lua_State lua_State;
#define LUA_STATE_DECL
#endif

int lhos_ntp_sync (lua_State *L);
int lhos_ntp_status (lua_State *L);
int lhos_ntp_get_time (lua_State *L);
int lhos_ntp_set_server (lua_State *L);
int lhos_ntp_stop (lua_State *L);

#endif /* LHOS_NTP_H */
