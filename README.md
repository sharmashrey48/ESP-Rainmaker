# ESP-Rainmaker

In this project we will be working on ESP-IDF and learning about the ESP-Rainmaker

We'll be building a motion sensor device that will detect the motion and update the information on the Rainmaker mobile app. As soon as the Motion is detected it will turn on the LED.

we can also control the LED from the Rainmaker mobile app. 

## Code Explaination: 

### Open the app_main.c file and you'll find the source code for the project

`#include <string.h>`
`#include <freertos/FreeRTOS.h>`
`#include <freertos/task.h>`
`#include <esp_log.h>`
`#include <nvs_flash.h>`
`#include <sdkconfig.h>`
`#include <esp_rmaker_core.h>`
`#include <esp_rmaker_standard_params.h>`
`#include <esp_rmaker_standard_devices.h>`
`#include <app_wifi.h>`
`#include <driver/gpio.h>`
`#include "app_priv.h"`

Here we need to define all the library files that are required for our project. 

`#define MOTION_PIN 34
#define LED 2
#define OUTPUT_GPIO 26
static bool g_power_state = DEFAULT_POWER;

static const char *TAG = "app_main";
static int g_motion = DEFAULT_MOTION;
esp_rmaker_device_t *switch_device;
esp_rmaker_device_t *motion_sensor_device;
esp_rmaker_param_t *Motion_Parameter;
void *a = &g_motion;`

Here we've defined all the variables that are required in the program. 

`static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
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
}`
This is the callback function from where we will be updating the parameters, it is similar to callback function that we use in PubSubClient.h library to receive the data over MQTT. Here we will be receiving the values from the ESP-Rainmaker mobile app and we will be updating the changes in the device according to the values received. 

`void motion(void *pvParameters)
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
}` 

Here we have defined the **motion** Task, we are using FreeRTOS in this program to run this motion task on a different core so that it will continously scan for the motion and when the motion is detected it will update the values on the rainmaker mobile app. 

  > Now let's start the app_main() function. 
  
  ` gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(MOTION_PIN);
    gpio_set_direction(MOTION_PIN, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(OUTPUT_GPIO);
    gpio_set_direction(OUTPUT_GPIO, GPIO_MODE_OUTPUT);` 
    
Here we are configuring the GPIO Pins and selecting the INPUT and OUTPUT Modes for the pins that we will be using, for the Motion sensor we will be using the INPUT mode because the sensot will only take the input from the surrounding and update it on the app and for LEDs we are using OUTPUT mode because we want to take the action on the board itself and obviously, the LED is an OUTPUT device. 

`xTaskCreatePinnedToCore(motion, "motion", 5000, NULL, 0, NULL, 1);`

This is FreeRTOS Task create function that will create a task and pin it to a specific core, so that we can utilize both the cores on the ESP32. 
Here we have some parameters, let's have a look at them. 
- pvTaskCode : This is the task entry function and it must not return any value. 
- TaskName : A descriptive name for the task
- StackDepth : Here we need to define the size of stack in number of bytes. 
- pvParameters :  Pointers that will be used as a parameter for the task that we have already created. 
- uxPriority : Here we need to define the priority of the task that we just created. 
- pvCreatedTask : used to pass back a handle by which created task can be referenced. 
- xCoreID : Here we will define the core on which we want to run our task. 

## Till Now we've defined all the basic things and now it is time to start working on the ESP-Rainmaker. 

`esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);` 
    
Here we need to initialise the Non Volatile Stograge partition to store our Wifi Credentials and if we receive any type of error then it will erase the complete NVS and initialise it again. 

`app_wifi_init();`
With this fucntion we are initialising the Wifi and this process must be done before initialising the ESP-Rainmaker. 


`esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };`
    
Now we need to define the configuration for ESP-Rainmaker, this config is used to synchronise the time. If it is true it will fetch the time from SNTP (Simple Network Time Protocol) before initializing the ESP-Rainmaker. 

`esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Motion Sensor", "Motion Sensor");
    if (!node)
    {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }`
Now we need to initialise the Node so that the Rainmaker Mobile app will recognize our device. It is used to define the Name and type of our device. 

    `motion_sensor_device = esp_rmaker_device_create("Motion Sensor", "Sensor", a);
    Motion_Parameter = esp_rmaker_param_create("Motion", "Value", esp_rmaker_int(g_motion), PROP_FLAG_READ);
    esp_rmaker_device_add_param(motion_sensor_device, Motion_Parameter);
    esp_rmaker_device_assign_primary_param(motion_sensor_device, Motion_Parameter);
    esp_rmaker_node_add_device(node, motion_sensor_device);

    switch_device = esp_rmaker_switch_device_create("Switch", NULL, DEFAULT_POWER);
    esp_rmaker_device_add_cb(switch_device, write_cb, NULL);
    esp_rmaker_node_add_device(node, switch_device);`

Here we have created two device and defined their paramaters for the Rainmaker app, One is the Motion Sensor which has a parameter by the name of **Motion_Parameter** so whenever the motion is detected it will return and update the Value "1" otherwise "0". 

At the Same time we've created another device which is present in the Pre-defined device types of the esp_rmaker_standart_device.h Library and we are also using the pre-defined parameters from esp_rmaker_standart_param.h Library. 

First device is a motion sensor and Second Device is a normal Switch with the Power parameter. 

`esp_rmaker_start();`

Finally we are using this funciton to initialise the ESP-Rainmaker. 


    
