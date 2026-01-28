/*
 * Public API for the LHOS Lua VM bindings.
 */

#ifndef LHOS_LUA_H
#define LHOS_LUA_H

#include <stdbool.h>

#ifndef LHOS_EVENT_QUEUE_LEN
#define LHOS_EVENT_QUEUE_LEN 8
#endif

/* forward declare Lua state to avoid forcing inclusion of lua headers */
typedef struct lua_State lua_State;

/* Initialize the embedded Lua VM and register LHOS modules. Safe to call
        multiple times (idempotent). */
void lhos_lua_init (void);

/* Run a Lua script provided as a NUL-terminated string. Returns Lua error
        code (LUA_OK on success). */
int lhos_lua_run_script (const char *script);

/* Returns true if the Lua VM is initialized and available. */
bool lhos_lua_enabled (void);

/* Pump the Lua scheduler — resume yielded coroutines and process timers.
        Minimal implementations may simply yield to the OS. */
void lhos_lua_scheduler_run (void);

/* Enqueue a notification (called from C BLE layer). */
void lhos_lua_enqueue_notification (const uint8_t *data, size_t len);

/* Register a Lua callback for BLE notifications (callable from Lua).
        Should be used within the Lua VM. Returns 1 on success. */
int lhos_lua_register_ble_callback (lua_State *L);

/* Enqueue a network event (called from C network layer). */
void lhos_lua_enqueue_net_event (int conn_id, const uint8_t *data, size_t len);

/* Enqueue a network event taking ownership of `buf` (caller-allocated).
        The buffer must be allocated with a leading `size_t` containing the
        data length, followed by the data bytes. The dispatcher will free it
        after invoking the Lua callback. */
void lhos_lua_enqueue_net_event_owned (int conn_id, void *buf, size_t len);

/* Register a Lua callback for network receive events. Lua callback
   receives `(conn_id, data_string)` and should be registered from
   within the Lua VM. Returns 1 on success. */
int lhos_lua_register_net_callback (lua_State *L);

/* Return non-zero if a net callback is registered in the Lua VM. */
int lhos_lua_net_callback_registered (void);

#endif /* LHOS_LUA_H */
