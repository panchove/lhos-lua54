/* Lua binding for LED (WS2812B) using lhos_ws2812b component */
#include "lhos_lua_led.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lauxlib.h"
#include "lhos_ws2812b.h"
#include "lua.h"
#include <math.h>
#include <stdlib.h>

static int
push_error (lua_State *L, const char *msg)
{
  lua_pushnil (L);
  lua_pushstring (L, msg);
  return 2;
}

/* track last color to allow fade_from current state */
static uint8_t last_r = 0;
static uint8_t last_g = 0;
static uint8_t last_b = 0;

struct fade_params
{
  uint8_t start_r, start_g, start_b;
  uint8_t end_r, end_g, end_b;
  uint32_t duration_ms;
  uint16_t steps;
};

static void
fade_task (void *arg)
{
  struct fade_params *p = (struct fade_params *)arg;
  if (!p)
    vTaskDelete (NULL);

  uint16_t steps = p->steps ? p->steps : 30;
  uint32_t step_ms = p->duration_ms / (steps ? steps : 1);

  float dr = ((float)p->end_r - (float)p->start_r) / (float)steps;
  float dg = ((float)p->end_g - (float)p->start_g) / (float)steps;
  float db = ((float)p->end_b - (float)p->start_b) / (float)steps;

  float cur_r = (float)p->start_r;
  float cur_g = (float)p->start_g;
  float cur_b = (float)p->start_b;

  for (uint16_t i = 0; i <= steps; ++i)
    {
      uint8_t rr = (uint8_t)fmaxf (0.0f, fminf (255.0f, roundf (cur_r)));
      uint8_t gg = (uint8_t)fmaxf (0.0f, fminf (255.0f, roundf (cur_g)));
      uint8_t bb = (uint8_t)fmaxf (0.0f, fminf (255.0f, roundf (cur_b)));
      lhos_ws2812b_set_color (((uint32_t)rr << 16) | ((uint32_t)gg << 8)
                              | (uint32_t)bb);
      last_r = rr;
      last_g = gg;
      last_b = bb;
      if (i == steps)
        break;
      vTaskDelay (pdMS_TO_TICKS (step_ms ? step_ms : 10));
      cur_r += dr;
      cur_g += dg;
      cur_b += db;
    }

  free (p);
  vTaskDelete (NULL);
}

int
lhos_lua_led_set (lua_State *L)
{
  int r = (int)luaL_checkinteger (L, 1);
  int g = (int)luaL_checkinteger (L, 2);
  int b = (int)luaL_checkinteger (L, 3);
  esp_err_t err = lhos_ws2812b_set_color (((uint32_t)(uint8_t)r << 16)
                                          | ((uint32_t)(uint8_t)g << 8)
                                          | (uint32_t)(uint8_t)b);
  if (err != ESP_OK)
    return push_error (L, "failed to set color");
  last_r = (uint8_t)r;
  last_g = (uint8_t)g;
  last_b = (uint8_t)b;
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_led_off (lua_State *L)
{
  esp_err_t err = lhos_ws2812b_off ();
  if (err != ESP_OK)
    return push_error (L, "failed to turn off LED");
  last_r = last_g = last_b = 0;
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_led_config (lua_State *L)
{
  (void)L;
  esp_err_t err = lhos_ws2812b_init ();
  if (err != ESP_OK)
    return push_error (L, "failed to init ws2812b");
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_led_get (lua_State *L)
{
  (void)L;
  /* Not supported by underlying driver */
  return push_error (L, "get not supported");
}

int
lhos_lua_led_set_enum (lua_State *L)
{
  int idx = (int)luaL_checkinteger (L, 1);
  esp_err_t err = lhos_ws2812b_set_color_enum (idx);
  if (err != ESP_OK)
    return push_error (L, "failed to set enum color");
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_led_flash (lua_State *L)
{
  int flashes = (int)luaL_checkinteger (L, 1);
  uint32_t on_ms = (uint32_t)luaL_checkinteger (L, 2);
  uint32_t off_ms = (uint32_t)luaL_checkinteger (L, 3);
  esp_err_t err = lhos_ws2812b_flash ((uint8_t)flashes, on_ms, off_ms);
  if (err != ESP_OK)
    return push_error (L, "failed to flash led");
  lua_pushboolean (L, 1);
  return 1;
}

/* Fade helpers exposed to Lua */
int
lhos_lua_led_fade_to (lua_State *L)
{
  int r = (int)luaL_checkinteger (L, 1);
  int g = (int)luaL_checkinteger (L, 2);
  int b = (int)luaL_checkinteger (L, 3);
  uint32_t dur = (uint32_t)luaL_optinteger (L, 4, 500);
  uint16_t steps = (uint16_t)luaL_optinteger (L, 5, 30);

  struct fade_params *p = malloc (sizeof (struct fade_params));
  if (!p)
    return luaL_error (L, "out of memory");
  p->start_r = last_r;
  p->start_g = last_g;
  p->start_b = last_b;
  p->end_r = (uint8_t)r;
  p->end_g = (uint8_t)g;
  p->end_b = (uint8_t)b;
  p->duration_ms = dur;
  p->steps = steps;

  if (xTaskCreate (fade_task, "led_fade", 2048, p, tskIDLE_PRIORITY + 1, NULL)
      != pdPASS)
    {
      free (p);
      return push_error (L, "failed to start fade task");
    }

  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_led_fade_in (lua_State *L)
{
  int r = (int)luaL_checkinteger (L, 1);
  int g = (int)luaL_checkinteger (L, 2);
  int b = (int)luaL_checkinteger (L, 3);
  uint32_t dur = (uint32_t)luaL_optinteger (L, 4, 500);
  uint16_t steps = (uint16_t)luaL_optinteger (L, 5, 30);

  struct fade_params *p = malloc (sizeof (struct fade_params));
  if (!p)
    return luaL_error (L, "out of memory");
  p->start_r = 0;
  p->start_g = 0;
  p->start_b = 0;
  p->end_r = (uint8_t)r;
  p->end_g = (uint8_t)g;
  p->end_b = (uint8_t)b;
  p->duration_ms = dur;
  p->steps = steps;

  if (xTaskCreate (fade_task, "led_fade_in", 2048, p, tskIDLE_PRIORITY + 1,
                   NULL)
      != pdPASS)
    {
      free (p);
      return push_error (L, "failed to start fade task");
    }

  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_led_fade_out (lua_State *L)
{
  uint32_t dur = (uint32_t)luaL_optinteger (L, 1, 500);
  uint16_t steps = (uint16_t)luaL_optinteger (L, 2, 30);

  struct fade_params *p = malloc (sizeof (struct fade_params));
  if (!p)
    return luaL_error (L, "out of memory");
  p->start_r = last_r;
  p->start_g = last_g;
  p->start_b = last_b;
  p->end_r = 0;
  p->end_g = 0;
  p->end_b = 0;
  p->duration_ms = dur;
  p->steps = steps;

  if (xTaskCreate (fade_task, "led_fade_out", 2048, p, tskIDLE_PRIORITY + 1,
                   NULL)
      != pdPASS)
    {
      free (p);
      return push_error (L, "failed to start fade task");
    }

  lua_pushboolean (L, 1);
  return 1;
}

void
lhos_lua_led_register (lua_State *L)
{
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_led_set);
  lua_setfield (L, -2, "set");
  lua_pushcfunction (L, lhos_lua_led_off);
  lua_setfield (L, -2, "off");
  lua_pushcfunction (L, lhos_lua_led_config);
  lua_setfield (L, -2, "config");
  lua_pushcfunction (L, lhos_lua_led_get);
  lua_setfield (L, -2, "get");
  lua_pushcfunction (L, lhos_lua_led_set_enum);
  lua_setfield (L, -2, "set_enum");
  lua_pushcfunction (L, lhos_lua_led_flash);
  lua_setfield (L, -2, "flash");
  lua_pushcfunction (L, lhos_lua_led_fade_in);
  lua_setfield (L, -2, "fade_in");
  lua_pushcfunction (L, lhos_lua_led_fade_out);
  lua_setfield (L, -2, "fade_out");
  lua_pushcfunction (L, lhos_lua_led_fade_to);
  lua_setfield (L, -2, "fade_to");
  lua_setglobal (L, "led");
}
