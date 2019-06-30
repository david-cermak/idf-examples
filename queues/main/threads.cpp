#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <queue>
#include <string>
#include <sstream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/timer.h"
// #include <esp_pthread.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <esp_log.h>


struct my_data {
    int counter;
    void * data;
};

typedef struct my_data * pdata;

int count = 0;

void test_task(std::queue<pdata> *pq)
{
    while (1) {
        // std::cout << "Couter:" << count++ << std::endl;
        pdata temp_data = (pdata)calloc(1, sizeof(struct my_data));
        temp_data->counter ++;
        pq->push(temp_data);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void test_task_2(std::queue<pdata> *pq)
{
    while (1) {
        // std::cout << "Task2:" << count << std::endl;
        // pdata temp_data = (pdata)malloc(sizeof(struct my_data));
        if (!pq->empty()) {
            pdata temp_data = pq->front();
            temp_data->counter ++;
            pq->push(temp_data);
        }
        // 
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int cpp_main()
{
    std::queue<pdata> q;
    std::thread test(test_task, &q);
    std::thread test2(test_task_2, &q);
    std::thread test3(test_task_2, &q);

    while (1) {
        if (!q.empty()) {
            std::cout << "From main: " << q.front()->counter << std::endl;
            q.pop();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    test.join();
    test2.join();
    return 0;

}

extern "C" void app_main() {
    cpp_main();
}