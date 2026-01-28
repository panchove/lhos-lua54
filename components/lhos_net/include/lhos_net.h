/* Lightweight network binding for LHOS Lua
 * Provides simple TCP connect/send/recv/disconnect APIs exposed to Lua.
 */
#ifndef LHOS_NET_H
#define LHOS_NET_H

#ifndef LUA_STATE_DECL
typedef struct lua_State lua_State;
#define LUA_STATE_DECL
#endif

/* Configuration */
#ifndef LHOS_NET_MAX_CONNECTIONS
#define LHOS_NET_MAX_CONNECTIONS 8
#endif
#ifndef LHOS_NET_RX_QUEUE_LEN
#define LHOS_NET_RX_QUEUE_LEN 8
#endif

/* Public API exposed to Lua. These functions are implemented in a
     non-blocking fashion: `lhos_net_connect` returns a connection id
     (integer) which should be used for subsequent `send`/`recv`/`disconnect`.

     - `connect(host, port, options?) -> conn_id | nil, err`
           options (table, optional):
               - `reconnect` (boolean): enable automatic reconnection (default
   false)
               - `backoff_ms` (number): initial backoff in ms (default 1000)
               - `max_retries` (integer): max retry attempts (0 = unlimited)
     - `disconnect(conn_id) -> true | false, err`
     - `send(conn_id, data) -> bytes_queued | false, err`
     - `recv(conn_id, max_bytes=1024) -> data | nil, err`  (non-blocking poll)

*/
int lhos_net_connect (lua_State *L);
int lhos_net_disconnect (lua_State *L);
int lhos_net_send (lua_State *L);
int lhos_net_recv (lua_State *L);

/* Set per-connection dispatch preference.
     Usage from Lua: `net.set_use_dispatcher(conn_id, true|false)`
     If true, incoming data will be forwarded to the Lua dispatcher when a
     net callback is registered. If false, incoming data always goes to
   `recv`/`rxq`.
*/
int lhos_net_set_use_dispatcher (lua_State *L);

/* Helper: DNS resolve */
int lhos_net_resolve (lua_State *L);

#endif /* LHOS_NET_H */
