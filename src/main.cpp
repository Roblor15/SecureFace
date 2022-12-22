#include "esp32-hal-log.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "sys/_stdint.h"
#include <Arduino.h>
#include <WiFi.h>
#include <cstddef>
#include <esp_camera.h>
#include <HCSR04.h>
// #include <CircularBuffer.h>

#define WIFI_SSID "Lorenzon-Home"
#define WIFI_PSW "DaMiAnO199411"
#define HOST "raspberrypi"
#define PORT_VIDEO 8080
#define PORT_PHOTO 8081
#define PHOTO_TRIGGER 30

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// Pin flash
// #define LED 4

// Number of frames per video
#define N_FRAMES 200

// Dimension of CircularBuffer
#define N_BUF 120

// Bitmask (pin 12) if pins used to wake up
#define BUTTON_PIN_BITMASK 0x000001000

typedef struct Photo
{
  uint8_t *buffer;
  size_t len;
} Photo;

typedef struct TaskParameters
{
  int queue;
  uint16_t host_port;
} TaskParameters;

void setupCamera();
void task_0_function(void *);
void photo_deallocator(Photo *p);
void copy_buffer(uint8_t *dst, uint8_t *src, size_t len);
void task_send(void *argv);

// Counter for frames
int i = 0;

// Boolean value that indicate when send photos
bool send = true;
// Boolean value that indicate if task_0 has finished
bool finished = false;

// buffer for saving photos while sending them
// CircularBuffer<Photo *> buffer(N_BUF, photo_deallocator, true);
// 0: Video Queue | 1:Photo Queue
QueueHandle_t queues[2];

TaskHandle_t task_0;
TaskHandle_t task_video;
TaskHandle_t task_photo;

UltraSonicDistanceSensor distanceSensor(13, 12);

void setup()
{
  Serial.begin(115200);

  queues[0] = xQueueCreate(120, sizeof(Photo *));
  queues[1] = xQueueCreate(10, sizeof(Photo *));

  // pinMode(LED, OUTPUT);

  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);

  xTaskCreate(
      task_0_function,
      "Invio photo",
      3500,
      NULL,
      0,
      &task_0);

  setupCamera();

  delay(1000);
}

unsigned long message_timestamp = 0;
unsigned long finished_time = 0;

void loop()
{
  //double distance = distanceSensor.measureDistanceCm(20);
  //Serial.println(distance);
  uint64_t now = millis();
  if (distanceSensor.measureDistanceCm(20) > PHOTO_TRIGGER) {
    if (send && now - message_timestamp > 100)
    {
      message_timestamp = now;

      camera_fb_t *fb = nullptr;

      fb = esp_camera_fb_get();
      if (!fb)
      {
        Serial.println("Camera capture failed");
        ESP.restart();
        return;
      }

      Photo *p = (Photo *)ps_malloc(sizeof(Photo));
      log_d("%d", p);
      p->buffer = (uint8_t *)ps_malloc(sizeof(uint8_t) * fb->len);
      copy_buffer(p->buffer, fb->buf, fb->len);
      p->len = fb->len;

      esp_camera_fb_return(fb);

      // buffer.push(p);
      log_d("len %d", p->len);
      xQueueSend(queues[0], (void *) &p, 20000);

      if (++i == N_FRAMES)
      {
        send = false;
        finished_time = millis();
      }
    }
    else if (finished)
    {
      vTaskDelete(task_0);
      log_d("going to sleep");
      delay(2000);
      esp_deep_sleep_start();
    }
    else if (!send && now - finished_time > 60 * 2 * 1000)
    {
      vTaskDelete(task_0);
      log_d("going to sleep");
      delay(2000);
      esp_deep_sleep_start();
    }
  }
  else {
    Serial.println("Riconoscimento");
    camera_fb_t *fb = nullptr;

    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      ESP.restart();
      return;
    }

    Photo *p = (Photo *)ps_malloc(sizeof(Photo));
    log_d("%d", p);
    p->buffer = (uint8_t *)ps_malloc(sizeof(uint8_t) * fb->len);
    copy_buffer(p->buffer, fb->buf, fb->len);
    p->len = fb->len;

    xQueueSend(queues[1], (void *) &p, 20000);
  }
}

void setupCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_SVGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

void task_0_function(void *pv)
{
  // Constants depending on own connections
  const char *ssid = WIFI_SSID;
  const char *password = WIFI_PSW;

  Serial.printf("Task 0 running on task %d\n", xPortGetCoreID());

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());

  TaskParameters taskParametersVideo;
  taskParametersVideo.queue = 0;
  taskParametersVideo.host_port = PORT_VIDEO;

  xTaskCreate(
      task_send,
      "Invio Video",
      3500,
      &taskParametersVideo,
      5,
      &task_video);

  TaskParameters taskParametersPhoto;
  taskParametersPhoto.queue = 1;
  taskParametersPhoto.host_port = PORT_PHOTO;

  xTaskCreate(
      task_send,
      "Invio Photo",
      3500,
      &taskParametersPhoto,
      5,
      &task_photo);
}

void copy_buffer(uint8_t *dst, uint8_t *src, size_t len)
{
  for (size_t j = 0; j < len; j++)
  {
    dst[j] = src[j];
  }
}

void task_send(void *argv)
{
  WiFiClient client;
  TaskParameters taskParameters = *(TaskParameters *)argv;
  
  QueueHandle_t queue = queues[taskParameters.queue];
  uint16_t port = taskParameters.host_port;

  Serial.println(port);

  while (!client.connect(HOST, port))
  {
    Serial.println("Connecting to host");
    delay(500);
    // digitalWrite(LED, HIGH);
    delay(500);
    // digitalWrite(LED, LOW);
  }

  Serial.printf("Connected to %s:%d\n", HOST, port);

  for (;;)
  {
    log_d("%d", send);
    Photo *p = NULL;
    int r = xQueueReceive(queue, &(p), 10000);
    // if (send == false && !buffer.can_pop())
    if (send == false && !r)
    {
      log_d("fine invio");
      client.stop();
      while (!WiFi.disconnect(true, true))
      {
        log_d("disconnecting");
        delay(1000);
      }

      finished = true;
      log_d("heap: %d", ESP.getFreeHeap());
      log_d("psram: %d", ESP.getFreePsram());

      return;
    }
    else
    {
      log_d("pre invio");
      // Photo *p = buffer.pop();
      if (r) {
        log_d("invio");
        log_d("len %d", p->len);
        client.write(p->buffer, p->len);
        log_d("inviato");
        client.flush();
        log_d("deallocator");
        photo_deallocator(p);
      }
      else {
          log_d("photo not available");
      }
    }

    vTaskDelay(20);
  }
}

void photo_deallocator(Photo *p)
{
  if (p != nullptr)
  {
    if (p->buffer != nullptr)
    {
      free(p->buffer);
    }
    free(p);
  }
}