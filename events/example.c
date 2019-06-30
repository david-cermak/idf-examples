/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"


static const char *TAG = "example";

typedef struct {
    int          group;
    int          timer;
    int          count;
    TaskHandle_t thnd;
} example_event_data_t;


static void example_timer_init(int timer_group, int timer_idx, uint32_t period)
{
    timer_config_t config;
    uint64_t alarm_val = (period * (TIMER_BASE_CLK / 1000000UL)) / 2;

    config.alarm_en = 1;
    config.auto_reload = 1;
    config.counter_dir = TIMER_COUNT_UP;
    config.divider = 2;     //Range is 2 to 65536
    config.intr_type = TIMER_INTR_LEVEL;
    config.counter_en = TIMER_PAUSE;
    /*Configure timer*/
    timer_init(timer_group, timer_idx, &config);
    /*Stop timer counter*/
    timer_pause(timer_group, timer_idx);
    /*Load counter value */
    timer_set_counter_value(timer_group, timer_idx, 0x00000000ULL);
    /*Set alarm value*/
    timer_set_alarm_value(timer_group, timer_idx, alarm_val);
    /*Enable timer interrupt*/
    timer_enable_intr(timer_group, timer_idx);
}

static void example_timer_rearm(int timer_group, int timer_idx)
{
    if (timer_group == 0) {
        if (timer_idx == 0) {
            TIMERG0.int_clr_timers.t0 = 1;
            TIMERG0.hw_timer[0].update = 1;
            TIMERG0.hw_timer[0].config.alarm_en = 1;
        } else {
            TIMERG0.int_clr_timers.t1 = 1;
            TIMERG0.hw_timer[1].update = 1;
            TIMERG0.hw_timer[1].config.alarm_en = 1;
        }
    }
    if (timer_group == 1) {
        if (timer_idx == 0) {
            TIMERG1.int_clr_timers.t0 = 1;
            TIMERG1.hw_timer[0].update = 1;
            TIMERG1.hw_timer[0].config.alarm_en = 1;
        } else {
            TIMERG1.int_clr_timers.t1 = 1;
            TIMERG1.hw_timer[1].update = 1;
            TIMERG1.hw_timer[1].config.alarm_en = 1;
        }
    }
}

static void example_timer_isr(void *arg)
{
    example_event_data_t *tim_arg = (example_event_data_t *)arg;

    if (tim_arg->thnd != NULL) {
        if (tim_arg->count++ < 10) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            if (xTaskNotifyFromISR(tim_arg->thnd, tim_arg->count, eSetValueWithOverwrite, &xHigherPriorityTaskWoken) != pdPASS) {
                ESP_EARLY_LOGE(TAG, "Failed to notify task %p", tim_arg->thnd);
            } else {
                // ESP_EARLY_LOGW(TAG, ".");
                if (xHigherPriorityTaskWoken == pdTRUE) {
                    portYIELD_FROM_ISR();
                }
            }
        }
    }
    // re-start timer
    example_timer_rearm(tim_arg->group, tim_arg->timer);
}

static void example_task(void *p)
{
    example_event_data_t *arg = (example_event_data_t *) p;
    timer_isr_handle_t inth;

    ESP_LOGI(TAG, "%p: run task", xTaskGetCurrentTaskHandle());

    esp_err_t res = timer_isr_register(arg->group, arg->timer, example_timer_isr, arg, 0, &inth);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "%p: failed to register timer ISR", xTaskGetCurrentTaskHandle());
    } else {
        res = timer_start(arg->group, arg->timer);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "%p: failed to start timer", xTaskGetCurrentTaskHandle());
        }
    }

    while (1) {
        uint32_t event_val;
        xTaskNotifyWait(0, 0, &event_val, portMAX_DELAY);
        ESP_LOGI(TAG, "Task[%p]: received event %d", xTaskGetCurrentTaskHandle(), event_val);
        if (event_val > 5) {
            void * test = NULL;
            printf("test %s\n", (char*)test);
        }
    }
}


void app_main()
{

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d \n", chip_info.revision);

    static example_event_data_t event_data[portNUM_PROCESSORS] = {
        {
            .group = TIMER_GROUP_1,
            .timer = TIMER_0,
        },
#if CONFIG_FREERTOS_UNICORE == 0
        {
            .group = TIMER_GROUP_1,
            .timer = TIMER_1,
        },
#endif
    };

    example_timer_init(TIMER_GROUP_1, TIMER_0, 3000);
    example_timer_init(TIMER_GROUP_1, TIMER_1, 3000);

    xTaskCreatePinnedToCore(example_task, "task1", 2048, &event_data[0], 3, &event_data[0].thnd, 0);
    ESP_LOGI(TAG, "Created task %p", event_data[0].thnd);
#if CONFIG_FREERTOS_UNICORE == 0
    xTaskCreatePinnedToCore(example_task, "task2", 2048, &event_data[1], 3, &event_data[1].thnd, 1);
    ESP_LOGI(TAG, "Created task %p", event_data[1].thnd);
#endif

}
