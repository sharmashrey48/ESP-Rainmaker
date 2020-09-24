#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sdkconfig.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>

#include <app_wifi.h>
#include <driver/gpio.h>

#include "app_priv.h"

#define MOTION_PIN 34
#define LED 2
#define OUTPUT_GPIO 26
static bool g_power_state = DEFAULT_POWER;

static const char *TAG = "app_main";
static int g_motion = DEFAULT_MOTION;
esp_rmaker_device_t *switch_device;
esp_rmaker_device_t *motion_sensor_device;
esp_rmaker_param_t *Motion_Parameter;
void *a = &g_motion;

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                          const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (strcmp(esp_rmaker_param_get_name(param), "power") == 0)
    {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                 val.val.b ? "true" : "false", esp_rmaker_device_get_name(device),
                 esp_rmaker_param_get_name(param));
        // app_driver_set_state(val.val.b);
        if (val.val.b == true)
        {
            gpio_set_level(OUTPUT_GPIO, 1);
            ESP_LOGI(TAG, "Switch On");
        }
        if (val.val.b == false)
        {
            ESP_LOGI(TAG, "Switch off");
            gpio_set_level(OUTPUT_GPIO, 0);
        }

        esp_rmaker_param_update_and_report(param, val);
    }
    return ESP_OK;
}

void motion(void *pvParameters)
{
    for (;;)
    {
        g_motion = gpio_get_level(MOTION_PIN);
        if (g_motion == 1)
        {
            ESP_LOGE(TAG, "MOTION DETECTED");
            printf("Turning on LED\n");
            gpio_set_level(LED, 1);
            esp_rmaker_param_update_and_report(Motion_Parameter, esp_rmaker_int(g_motion));
            esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_name(switch_device, "power"),
                esp_rmaker_bool(true));
            gpio_set_level(OUTPUT_GPIO, 1);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        else
        {
            gpio_set_level(LED, 0);
            esp_rmaker_param_update_and_report(Motion_Parameter, esp_rmaker_int(g_motion));
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

void app_main()

{

    gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(MOTION_PIN);
    gpio_set_direction(MOTION_PIN, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(OUTPUT_GPIO);
    gpio_set_direction(OUTPUT_GPIO, GPIO_MODE_OUTPUT);

    xTaskCreatePinnedToCore(motion, "motion", 5000, NULL, 0, NULL, 1);

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_init()
     */
    app_wifi_init();

    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Motion Sensor", "Motion Sensor");
    if (!node)
    {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

    /* Create a Motion Sensor device and add the relevant parameters to it */
    motion_sensor_device = esp_rmaker_device_create("Motion Sensor", "Sensor", a);
    Motion_Parameter = esp_rmaker_param_create("Motion", "Value", esp_rmaker_int(g_motion), PROP_FLAG_READ);
    esp_rmaker_device_add_param(motion_sensor_device, Motion_Parameter);
    esp_rmaker_device_assign_primary_param(motion_sensor_device, Motion_Parameter);
    esp_rmaker_node_add_device(node, motion_sensor_device);

    /* Create a Switch device and add the relevant parameters to it */
    switch_device = esp_rmaker_switch_device_create("Switch", NULL, DEFAULT_POWER);
    esp_rmaker_device_add_cb(switch_device, write_cb, NULL);
    esp_rmaker_node_add_device(node, switch_device);

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }
}