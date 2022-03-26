#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <sdkconfig.h>

#include <app_reset.h>
#include <ws2812_led.h>

#define DEFAULT_SATURATION  100
#define DEFAULT_BRIGHTNESS  50
#define DEFAULT_HUE 90

static uint16_t g_hue = DEFAULT_HUE;
static uint16_t g_saturation = DEFAULT_SATURATION;
static uint16_t g_value = DEFAULT_BRIGHTNESS;

static void app_rgbled_update()
{
    ws2812_led_set_hsv(DEFAULT_HUE, g_saturation, g_value);
}

esp_err_t rgbled_init(void)
{
    esp_err_t err = ws2812_led_init();
    if (err != ESP_OK) {
        return err;
    }

    // TODO: Do something more fancy here and perhaps related with modbus/status.
    ws2812_led_set_hsv(g_hue, g_saturation, g_value);
    return ESP_OK;
}

void app_rgbled_init()
{
    rgbled_init();
    app_rgbled_update();
}
