#include "HardwareSerial.h"
#include "esp32-hal-log.h"
#include "esp32-hal.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "sys/_stdint.h"
#include <Arduino.h>
#include <cstddef>
#include <esp_camera.h>
#include "CircularBuffer.h"
#include <unity.h>

int size = 10;
uint64_t start_time, finish_time;

SemaphoreHandle_t semaphore_sender = nullptr, semaphore_receiver = nullptr;

void circular_buffer_receiver(void *circular_buffer) {
    xSemaphoreTake(semaphore_receiver, portMAX_DELAY);
    CircularBuffer<uint32_t> *c = (CircularBuffer<uint32_t> *) circular_buffer;
    
    start_time = millis();
    for (int i = 0; i < 10; ++i) {
        c->push(100);
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    xSemaphoreGive(semaphore_receiver);
}

void circular_buffer_sender(void *circular_buffer) {
    xSemaphoreTake(semaphore_sender, portMAX_DELAY);
    CircularBuffer<uint32_t> *c = (CircularBuffer<uint32_t> *) circular_buffer;
    for (int i = 0; i < 10; ++i) {
        c->pop();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    finish_time = millis();

    xSemaphoreGive(semaphore_sender);
}

void circular_buffer_manager(void) {
    CircularBuffer<uint32_t> buffer(size, true);
    TaskHandle_t sender_handler, receiver_handler;

    xTaskCreate(circular_buffer_receiver, "Buffer receiver", 2000, &buffer, 5, &receiver_handler);
    xTaskCreate(circular_buffer_sender, "Buffer sender", 2000, &buffer, 5, &receiver_handler);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    xSemaphoreTake(semaphore_sender, portMAX_DELAY);
    xSemaphoreGive(semaphore_sender);
    xSemaphoreTake(semaphore_receiver, portMAX_DELAY);
    xSemaphoreGive(semaphore_receiver);

    Serial.println(finish_time - start_time);
}


void setup()
{
    delay(2000); // service delay
    Serial.begin(115200);
    semaphore_sender = xSemaphoreCreateBinary();
    semaphore_receiver = xSemaphoreCreateBinary();
    UNITY_BEGIN();

    RUN_TEST(circular_buffer_manager);

    UNITY_END(); // stop unit testing
}

void loop()
{
}

