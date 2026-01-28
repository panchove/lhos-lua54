/* Non-blocking connection manager for LHOS.
 * - Maintains up to LHOS_NET_MAX_CONNECTIONS concurrent sockets
 * - Each connection has a TX queue and an RX queue to avoid blocking Lua
 * - Lua APIs are non-blocking: `connect` returns conn_id, `send` queues data,
 *   `recv` polls for available data.
 */

#include "lhos_net.h"
#include "esp_log.h"
#include "lauxlib.h"
#include "lua.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static const char *TAG = "lhos_net";

static const char *
gai_errstr (int rc)
{
  switch (rc)
    {
    case EAI_AGAIN:
      return "Temporary failure in name resolution";
    case EAI_BADFLAGS:
      return "Bad flags";
    case EAI_FAIL:
      return "Non-recoverable failure in name resolution";
    case EAI_FAMILY:
      return "Address family not supported";
    case EAI_MEMORY:
      return "Memory allocation failure";
    case EAI_NONAME:
      return "Name or service not known";
    case EAI_SERVICE:
      return "Service not supported for socket type";
    case EAI_SOCKTYPE:
      return "Socket type not supported";
    default:
      return "getaddrinfo error";
    }
}

typedef struct
{
  int sock;
  int id;
  QueueHandle_t txq; /* items: pointer+len pairs */
  QueueHandle_t rxq; /* items: pointer to buffer (malloc'd) */
  /* reconnection policy */
  bool reconnect;
  uint32_t backoff_ms;
  int max_retries; /* 0 = unlimited */
  int retries;
  TickType_t next_retry_tick;
  char host[64];
  int port;
  bool use_dispatcher; /* if true, prefer dispatcher when callback exists */
  bool used;
} lhos_conn_t;

static lhos_conn_t conns[LHOS_NET_MAX_CONNECTIONS];
static TaskHandle_t manager_task = NULL;
static SemaphoreHandle_t conns_lock = NULL;

struct tx_item
{
  void *data;
  size_t len;
};

static void
set_nonblocking (int s)
{
  int flags = fcntl (s, F_GETFL, 0);
  if (flags >= 0)
    fcntl (s, F_SETFL, flags | O_NONBLOCK);
}

static int
alloc_conn (void)
{
  for (int i = 0; i < LHOS_NET_MAX_CONNECTIONS; ++i)
    {
      if (!conns[i].used)
        {
          conns[i].used = true;
          conns[i].id = i + 1; /* return 1-based id */
          conns[i].sock = -1;
          conns[i].txq = xQueueCreate (8, sizeof (struct tx_item));
          conns[i].rxq = xQueueCreate (LHOS_NET_RX_QUEUE_LEN, sizeof (void *));
          conns[i].reconnect = false;
          conns[i].backoff_ms = 1000;
          conns[i].max_retries = 3;
          conns[i].retries = 0;
          conns[i].next_retry_tick = 0;
          conns[i].host[0] = '\0';
          conns[i].port = 0;
          conns[i].use_dispatcher = true;
          return conns[i].id;
        }
    }
  return -1;
}

static lhos_conn_t *
get_conn_by_id (int id)
{
  if (id <= 0 || id > LHOS_NET_MAX_CONNECTIONS)
    return NULL;
  lhos_conn_t *c = &conns[id - 1];
  return c->used ? c : NULL;
}

static void
manager (void *pv)
{
  (void)pv;
  ESP_LOGI (TAG, "lhos_net manager started");
  while (1)
    {
      fd_set readfds, writefds;
      int maxfd = -1;
      FD_ZERO (&readfds);
      FD_ZERO (&writefds);
      xSemaphoreTake (conns_lock, portMAX_DELAY);
      for (int i = 0; i < LHOS_NET_MAX_CONNECTIONS; ++i)
        {
          if (!conns[i].used || conns[i].sock < 0)
            continue;
          int s = conns[i].sock;
          FD_SET (s, &readfds);
          /* always check write as well to complete non-blocking connect/send
           */
          FD_SET (s, &writefds);
          if (s > maxfd)
            maxfd = s;
        }
      xSemaphoreGive (conns_lock);

      struct timeval tv = { 0, 200000 }; /* 200ms */
      int rc = select (maxfd + 1, &readfds, &writefds, NULL, &tv);
      if (rc < 0)
        {
          vTaskDelay (pdMS_TO_TICKS (50));
          continue;
        }

      xSemaphoreTake (conns_lock, portMAX_DELAY);
      for (int i = 0; i < LHOS_NET_MAX_CONNECTIONS; ++i)
        {
          if (!conns[i].used || conns[i].sock < 0)
            continue;
          int s = conns[i].sock;
          /* detect completed non-blocking connect via writefds */
          if (FD_ISSET (s, &writefds))
            {
              int so_err = 0;
              socklen_t slen = sizeof (so_err);
              if (getsockopt (s, SOL_SOCKET, SO_ERROR, &so_err, &slen) == 0)
                {
                  if (so_err == 0)
                    {
                      /* connection established */
                      conns[i].retries = 0;
                      conns[i].next_retry_tick = 0;
                      extern void lhos_lua_enqueue_net_event (
                          int, const uint8_t *, size_t);
                      const char *msg = "conn_open";
                      lhos_lua_enqueue_net_event (
                          conns[i].id, (const uint8_t *)msg, strlen (msg));
                    }
                  else
                    {
                      /* connection failed */
                      close (s);
                      conns[i].sock = -1;
                    }
                }
            }

          if (FD_ISSET (s, &readfds))
            {
              /* read available data, allocate buffer and push to rxq */
              uint8_t buf[1024];
              ssize_t r = recv (s, buf, sizeof (buf), 0);
              if (r > 0)
                {
                  /* allocate buffer with length prefix and copy data */
                  size_t total = sizeof (size_t) + r;
                  uint8_t *copy = malloc (total);
                  if (copy)
                    {
                      *((size_t *)copy) = (size_t)r;
                      memcpy (copy + sizeof (size_t), buf, r);
                      /* If a Lua net callback is registered, transfer
                         ownership exclusively to the dispatcher and do NOT
                         enqueue to rxq. Otherwise enqueue to rxq for
                         synchronous `recv` calls. */
                      extern int lhos_lua_net_callback_registered (void);
                      extern int lhos_lua_net_callback_registered (void);
                      extern void lhos_lua_enqueue_net_event_owned (
                          int, void *, size_t);
                      /* only forward to dispatcher when the connection
                         explicitly prefers dispatcher delivery */
                      if (conns[i].use_dispatcher
                          && lhos_lua_net_callback_registered ())
                        {
                          lhos_lua_enqueue_net_event_owned (conns[i].id, copy,
                                                            r);
                          /* ownership transferred to dispatcher. Do not
                           * enqueue or free. */
                        }
                      else
                        {
                          void *item = copy;
                          if (xQueueSend (conns[i].rxq, &item, 0) != pdTRUE)
                            {
                              /* failed to enqueue to rxq: free buffer to avoid
                                 leak and optionally log a warning */
                              free (copy);
                            }
                          /* rxq consumer will free the buffer on success */
                        }
                      /* ownership transferred to dispatcher and rxq; do not
                       * free here */
                    }
                }
              else if (r == 0)
                {
                  /* peer closed */
                  close (s);
                  conns[i].sock = -1;
                  /* schedule reconnect if enabled */
                  if (conns[i].reconnect)
                    {
                      conns[i].retries++;
                      TickType_t now = xTaskGetTickCount ();
                      uint32_t backoff = conns[i].backoff_ms
                                         * (1u << (conns[i].retries - 1));
                      if (backoff > 60000)
                        backoff = 60000;
                      conns[i].next_retry_tick = now + pdMS_TO_TICKS (backoff);
                      extern void lhos_lua_enqueue_net_event (
                          int, const uint8_t *, size_t);
                      const char *msg = "conn_closed";
                      lhos_lua_enqueue_net_event (
                          conns[i].id, (const uint8_t *)msg, strlen (msg));
                    }
                  else
                    {
                      extern void lhos_lua_enqueue_net_event (
                          int, const uint8_t *, size_t);
                      const char *msg = "conn_closed";
                      lhos_lua_enqueue_net_event (
                          conns[i].id, (const uint8_t *)msg, strlen (msg));
                    }
                }
              else
                {
                  if (errno != EWOULDBLOCK && errno != EAGAIN)
                    {
                      /* fatal error on socket */
                      close (s);
                      conns[i].sock = -1;
                      if (conns[i].reconnect)
                        {
                          conns[i].retries++;
                          TickType_t now = xTaskGetTickCount ();
                          uint32_t backoff = conns[i].backoff_ms
                                             * (1u << (conns[i].retries - 1));
                          if (backoff > 60000)
                            backoff = 60000;
                          conns[i].next_retry_tick
                              = now + pdMS_TO_TICKS (backoff);
                        }
                    }
                }
            }
          /* process tx queue */
          struct tx_item titem;
          while (xQueueReceive (conns[i].txq, &titem, 0) == pdTRUE)
            {
              if (titem.data && titem.len > 0 && conns[i].sock >= 0)
                {
                  ssize_t sent
                      = send (conns[i].sock, titem.data, titem.len, 0);
                  (void)sent;
                }
              free (titem.data);
            }

          /* if socket is closed and reconnect is enabled, check retry timer */
          if (conns[i].sock < 0 && conns[i].reconnect)
            {
              TickType_t now = xTaskGetTickCount ();
              if (conns[i].next_retry_tick == 0
                  || now >= conns[i].next_retry_tick)
                {
                  /* attempt reconnect */
                  if (conns[i].host[0] != '\0' && conns[i].port > 0)
                    {
                      extern void lhos_lua_enqueue_net_event (
                          int, const uint8_t *, size_t);
                      const char *attempt = "conn_reconnect_attempt";
                      lhos_lua_enqueue_net_event (conns[i].id,
                                                  (const uint8_t *)attempt,
                                                  strlen (attempt));

                      struct addrinfo hints, *res = NULL;
                      char portstr[8];
                      memset (&hints, 0, sizeof (hints));
                      hints.ai_family = AF_INET;
                      hints.ai_socktype = SOCK_STREAM;
                      snprintf (portstr, sizeof (portstr), "%d",
                                conns[i].port);
                      int rc
                          = getaddrinfo (conns[i].host, portstr, &hints, &res);
                      if (rc == 0 && res)
                        {
                          int s2 = socket (res->ai_family, res->ai_socktype,
                                           res->ai_protocol);
                          if (s2 >= 0)
                            {
                              set_nonblocking (s2);
                              int cr = connect (s2, res->ai_addr,
                                                res->ai_addrlen);
                              if (cr == 0 || errno == EINPROGRESS)
                                {
                                  conns[i].sock = s2;
                                  /* reset retries only when fully connected;
                                   * we'll detect via writefds */
                                }
                              else
                                {
                                  close (s2);
                                  conns[i].retries++;
                                  uint32_t backoff
                                      = conns[i].backoff_ms
                                        * (1u << (conns[i].retries - 1));
                                  if (backoff > 60000)
                                    backoff = 60000;
                                  conns[i].next_retry_tick
                                      = now + pdMS_TO_TICKS (backoff);
                                }
                            }
                          freeaddrinfo (res);
                        }
                      else
                        {
                          conns[i].retries++;
                          uint32_t backoff = conns[i].backoff_ms
                                             * (1u << (conns[i].retries - 1));
                          if (backoff > 60000)
                            backoff = 60000;
                          conns[i].next_retry_tick
                              = now + pdMS_TO_TICKS (backoff);
                        }
                    }
                }
            }
        }
      xSemaphoreGive (conns_lock);
    }
}

int
lhos_net_connect (lua_State *L)
{
  const char *host = luaL_checkstring (L, 1);
  int port = (int)luaL_checkinteger (L, 2);

  /* allocate connection slot */
  xSemaphoreTake (conns_lock, portMAX_DELAY);
  int id = alloc_conn ();
  xSemaphoreGive (conns_lock);
  if (id < 0)
    {
      lua_pushnil (L);
      lua_pushstring (L, "no connection slots");
      return 2;
    }

  lhos_conn_t *c = get_conn_by_id (id);
  if (!c)
    {
      lua_pushnil (L);
      lua_pushstring (L, "internal error");
      return 2;
    }

  /* parse optional options table at arg 3 */
  if (lua_istable (L, 3))
    {
      lua_getfield (L, 3, "reconnect");
      if (lua_isboolean (L, -1))
        c->reconnect = lua_toboolean (L, -1);
      lua_pop (L, 1);
      lua_getfield (L, 3, "backoff_ms");
      if (lua_isnumber (L, -1))
        c->backoff_ms = (uint32_t)lua_tointeger (L, -1);
      lua_pop (L, 1);
      lua_getfield (L, 3, "max_retries");
      if (lua_isnumber (L, -1))
        c->max_retries = (int)lua_tointeger (L, -1);
      lua_pop (L, 1);
      lua_getfield (L, 3, "use_dispatcher");
      if (lua_isboolean (L, -1))
        c->use_dispatcher = lua_toboolean (L, -1);
      lua_pop (L, 1);
    }

  struct addrinfo hints, *res = NULL;
  char portstr[8];
  memset (&hints, 0, sizeof (hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  snprintf (portstr, sizeof (portstr), "%d", port);
  int rc = getaddrinfo (host, portstr, &hints, &res);
  if (rc != 0 || !res)
    {
      lua_pushnil (L);
      lua_pushstring (L, gai_errstr (rc));
      return 2;
    }

  int s = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
  if (s < 0)
    {
      lua_pushnil (L);
      lua_pushstring (L, strerror (errno));
      freeaddrinfo (res);
      return 2;
    }

  set_nonblocking (s);
  int connrc = connect (s, res->ai_addr, res->ai_addrlen);
  if (connrc != 0 && errno != EINPROGRESS)
    {
      int e = errno;
      close (s);
      freeaddrinfo (res);
      lua_pushnil (L);
      lua_pushstring (L, strerror (e));
      return 2;
    }

  freeaddrinfo (res);

  xSemaphoreTake (conns_lock, portMAX_DELAY);
  c->sock = s;
  /* store host/port for possible reconnection */
  strncpy (c->host, host, sizeof (c->host) - 1);
  c->host[sizeof (c->host) - 1] = '\0';
  c->port = port;
  c->retries = 0;
  c->next_retry_tick = 0;
  xSemaphoreGive (conns_lock);

  lua_pushinteger (L, id);
  return 1;
}

int
lhos_net_disconnect (lua_State *L)
{
  int id = (int)luaL_checkinteger (L, 1);
  xSemaphoreTake (conns_lock, portMAX_DELAY);
  lhos_conn_t *c = get_conn_by_id (id);
  if (!c)
    {
      xSemaphoreGive (conns_lock);
      lua_pushboolean (L, 0);
      lua_pushstring (L, "invalid conn id");
      return 2;
    }
  if (c->sock >= 0)
    close (c->sock);
  c->sock = -1;
  /* drain and free tx queue items */
  if (c->txq)
    {
      struct tx_item titem;
      while (xQueueReceive (c->txq, &titem, 0) == pdTRUE)
        {
          if (titem.data)
            free (titem.data);
        }
      vQueueDelete (c->txq);
      c->txq = NULL;
    }
  /* drain and free rx queue items */
  if (c->rxq)
    {
      void *item = NULL;
      while (xQueueReceive (c->rxq, &item, 0) == pdTRUE)
        {
          if (item)
            free (item);
        }
      vQueueDelete (c->rxq);
      c->rxq = NULL;
    }
  c->used = false;
  xSemaphoreGive (conns_lock);
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_net_shutdown_all (lua_State *L)
{
  xSemaphoreTake (conns_lock, portMAX_DELAY);
  for (int i = 0; i < LHOS_NET_MAX_CONNECTIONS; ++i)
    {
      if (!conns[i].used)
        continue;
      if (conns[i].sock >= 0)
        close (conns[i].sock);
      conns[i].sock = -1;
      if (conns[i].txq)
        {
          struct tx_item titem;
          while (xQueueReceive (conns[i].txq, &titem, 0) == pdTRUE)
            {
              if (titem.data)
                free (titem.data);
            }
          vQueueDelete (conns[i].txq);
          conns[i].txq = NULL;
        }
      if (conns[i].rxq)
        {
          void *item = NULL;
          while (xQueueReceive (conns[i].rxq, &item, 0) == pdTRUE)
            {
              if (item)
                free (item);
            }
          vQueueDelete (conns[i].rxq);
          conns[i].rxq = NULL;
        }
      conns[i].used = false;
    }
  xSemaphoreGive (conns_lock);
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_net_send (lua_State *L)
{
  int id = (int)luaL_checkinteger (L, 1);
  size_t len = 0;
  const char *data = luaL_checklstring (L, 2, &len);
  lhos_conn_t *c = get_conn_by_id (id);
  if (!c)
    {
      lua_pushboolean (L, 0);
      lua_pushstring (L, "invalid conn id");
      return 2;
    }
  void *copy = malloc (len);
  if (!copy)
    {
      lua_pushboolean (L, 0);
      lua_pushstring (L, "out of memory");
      return 2;
    }
  memcpy (copy, data, len);
  struct tx_item it = { .data = copy, .len = len };
  if (xQueueSend (c->txq, &it, 0) != pdTRUE)
    {
      free (copy);
      lua_pushboolean (L, 0);
      lua_pushstring (L, "tx queue full");
      return 2;
    }
  lua_pushinteger (L, (int)len);
  return 1;
}

int
lhos_net_recv (lua_State *L)
{
  int id = (int)luaL_checkinteger (L, 1);
  lhos_conn_t *c = get_conn_by_id (id);
  if (!c)
    {
      lua_pushnil (L);
      lua_pushstring (L, "invalid conn id");
      return 2;
    }
  void *item = NULL;
  if (xQueueReceive (c->rxq, &item, 0) != pdTRUE)
    {
      lua_pushnil (L);
      lua_pushstring (L, "no data");
      return 2;
    }
  /* item is malloc'd buffer with prefixed size_t length */
  size_t len = *((size_t *)item);
  char *buf = (char *)item + sizeof (size_t);
  lua_pushlstring (L, buf, len);
  free (item);
  return 1;
}

int
lhos_net_resolve (lua_State *L)
{
  const char *host = luaL_checkstring (L, 1);
  struct addrinfo hints, *res = NULL;
  memset (&hints, 0, sizeof (hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  int rc = getaddrinfo (host, NULL, &hints, &res);
  if (rc != 0 || !res)
    {
      lua_pushnil (L);
      lua_pushstring (L, gai_errstr (rc));
      return 2;
    }
  char buf[64];
  void *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
  inet_ntop (AF_INET, addr, buf, sizeof (buf));
  freeaddrinfo (res);
  lua_pushstring (L, buf);
  return 1;
}

int
lhos_net_set_use_dispatcher (lua_State *L)
{
  int id = (int)luaL_checkinteger (L, 1);
  int use = lua_toboolean (L, 2);
  xSemaphoreTake (conns_lock, portMAX_DELAY);
  lhos_conn_t *c = get_conn_by_id (id);
  if (!c)
    {
      xSemaphoreGive (conns_lock);
      lua_pushboolean (L, 0);
      lua_pushstring (L, "invalid conn id");
      return 2;
    }
  c->use_dispatcher = use ? true : false;
  xSemaphoreGive (conns_lock);
  lua_pushboolean (L, 1);
  return 1;
}

static void __attribute__ ((constructor))
lhos_net_init_constructor (void)
{
  /* initialize locks and zero conns */
  conns_lock = xSemaphoreCreateMutex ();
  for (int i = 0; i < LHOS_NET_MAX_CONNECTIONS; ++i)
    {
      conns[i].used = false;
      conns[i].sock = -1;
      conns[i].txq = NULL;
      conns[i].rxq = NULL;
    }
  /* start manager task */
  xTaskCreatePinnedToCore (manager, "lhos_net_mgr", 4096, NULL,
                           tskIDLE_PRIORITY + 2, &manager_task, 1);
}
