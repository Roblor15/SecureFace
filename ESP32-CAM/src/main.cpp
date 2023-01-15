#include "esp32-hal-log.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include <Arduino.h>
#include <WiFi.h>
#include <cstddef>
#include <esp_camera.h>
#include <HCSR04.h>

#define VIDEO
#define RECOGNITION

#define WIFI_SSID "Lorenzon-Home"
#define WIFI_PSW "DaMiAnO199411"
#define HOST "roblor-matebook"
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
#define N_VIDEO_FRAMES 200

#define N_RECOGNITION_FRAMES 10

// Dimension of CircularBuffer
#define N_BUF 100

// Bitmask (pin 12) if pins used to wake up
#define BUTTON_PIN_BITMASK 0x000001000

typedef enum Purpose
{
  Video,
  Recognition,
  RecognitionEnd,
  Both
} Purpose;

typedef struct Photo
{
  uint8_t *buffer;
  size_t len;
} Photo;

typedef struct PhotoSend
{
  Photo *photo;
  Purpose purpose;
} PhotoSend;

void setupCamera();
void task_0_function(void *);
void photo_deallocator(Photo *p);
void copy_buffer(uint8_t *dst, uint8_t *src, size_t len);

// Queue for sending phtos from a task to the other
QueueHandle_t photo_queues;

TaskHandle_t task_0;

UltraSonicDistanceSensor distanceSensor(13, 15);

void setup()
{
  Serial.begin(115200);

  photo_queues = xQueueCreate(N_BUF, sizeof(PhotoSend *));

  // pinMode(LED, OUTPUT);

  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);

  xTaskCreatePinnedToCore(
      task_0_function,
      "Send photo",
      3500,
      NULL,
      0,
      &task_0,
      0);

  setupCamera();

  delay(1000);
}

// Counter for video frames
int video_count = 0;

// Boolean value that indicate when send photos
bool send = true;
// Boolean value that indicate if task_0 has finished
bool finished = false;

unsigned long message_timestamp = 0;
unsigned long finished_time = 0;

uint8_t recognition_count = 0;

void loop()
{
  uint64_t now = millis();

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

    p->buffer = (uint8_t *)ps_malloc(sizeof(uint8_t) * fb->len);
    copy_buffer(p->buffer, fb->buf, fb->len);
    p->len = fb->len;

    esp_camera_fb_return(fb);

    log_d("len photo %d: %d", video_count + recognition_count, p->len);
    log_d("heap : %d", ESP.getFreeHeap());
    log_d("psram: %d", ESP.getFreePsram());

    PhotoSend *photo_send = new PhotoSend;
    photo_send->photo = p;

    if (recognition_count || distanceSensor.measureDistanceCm(25) < 20)
    {
      log_d("sending recognition");
      photo_send->purpose = Purpose::Recognition;
      ++recognition_count;
    }
    else
    {
      log_d("sending video");
      photo_send->purpose = Purpose::Video;
      ++video_count;
    }

    if (!xQueueSend(photo_queues, (void *)&photo_send, 2 * 60 * 1000 / portTICK_PERIOD_MS))
    {
      vTaskDelete(task_0);
      log_d("going to sleep");
      delay(2000);
      esp_deep_sleep_start();
    }

    if (video_count == N_VIDEO_FRAMES)
    {
      send = false;
      finished_time = millis();
    }
    if (recognition_count == N_RECOGNITION_FRAMES)
    {
      Photo *p = (Photo *)ps_malloc(sizeof(Photo));
      p->len = 2;
      p->buffer = (uint8_t *)ps_malloc(sizeof(uint8_t) * 2);
      (p->buffer)[0] = 0xff;
      (p->buffer)[1] = 0xd9;

      PhotoSend *photo_send = new PhotoSend;
      photo_send->photo = p;
      photo_send->purpose = Purpose::RecognitionEnd;

      if (!xQueueSend(photo_queues, (void *)&photo_send, 2 * 60 * 1000 / portTICK_PERIOD_MS))
      {
        vTaskDelete(task_0);
        log_d("going to sleep");
        delay(2000);
        esp_deep_sleep_start();
      }

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

  sensor_t *s = esp_camera_sensor_get();
  s->set_brightness(s, 0);                 // -2 to 2
  s->set_contrast(s, 0);                   // -2 to 2
  s->set_saturation(s, 0);                 // -2 to 2
  s->set_special_effect(s, 0);             // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, 1);                   // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);                   // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);                    // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);              // 0 = disable , 1 = enable
  s->set_aec2(s, 0);                       // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);                   // -2 to 2
  s->set_aec_value(s, 300);                // 0 to 1200
  s->set_gain_ctrl(s, 1);                  // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);                   // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0); // 0 to 6
  s->set_bpc(s, 0);                        // 0 = disable , 1 = enable
  s->set_wpc(s, 1);                        // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);                    // 0 = disable , 1 = enable
  s->set_lenc(s, 1);                       // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);                    // 0 = disable , 1 = enable
  s->set_vflip(s, 1);                      // 0 = disable , 1 = enable
  s->set_dcw(s, 1);                        // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);                   // 0 = disable , 1 = enable
}

void task_0_function(void *pv)
{
  log_d("Task 0 running on task %d\n", xPortGetCoreID());

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PSW);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());

#ifdef VIDEO
  WiFiClient video_client;
#endif
#ifdef RECOGNITION
  WiFiClient recognition_client;
#endif

#ifdef VIDEO
  while (!video_client.connect(HOST, PORT_VIDEO))
  {
    Serial.println("Connecting to video host");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
#endif

#ifdef RECOGNITION
  while (!recognition_client.connect(HOST, PORT_PHOTO))
  {
    Serial.println("Connecting to recognition host");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  recognition_client.setTimeout(20);
#endif

  for (;;)
  {
    log_d("%d", send);
    PhotoSend *photo_send = nullptr;
    int r = xQueueReceive(photo_queues, &(photo_send), 10 * 1000 / portTICK_PERIOD_MS);
    // if (send == false && !buffer.can_pop())
    if (!r && send == false)
    {
      log_d("end sending");
#ifdef VIDEO
      video_client.stop();
#endif
#ifdef RECOGNITION
      recognition_client.stop();
#endif
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
      if (r)
      {
        Photo *p = photo_send->photo;

        if (photo_send->purpose == Purpose::Video)
        {
#ifdef VIDEO
          video_client.write(p->buffer, p->len);
          log_d("sended video");
          video_client.flush();
#endif
        }
        else if (photo_send->purpose == Purpose::Recognition)
        {
#ifdef RECOGNITION
          recognition_client.write(p->buffer, p->len);
          log_d("sended recognition");
          recognition_client.flush();
#endif
        }
        else if (photo_send->purpose == Purpose::RecognitionEnd)
        {
#ifdef RECOGNITION
          recognition_client.write(p->buffer, p->len);
          log_d("sended end recognition");
          recognition_client.flush();

          String s = recognition_client.readStringUntil('\n');

          log_d("%s", s);
#endif
        }
        else if (photo_send->purpose == Purpose::Both)
        {
#ifdef RECOGNITION
#ifdef VIDEO
          video_client.write(p->buffer, p->len);
          video_client.flush();

          recognition_client.write(p->buffer, p->len);
          recognition_client.flush();

          log_d("sended video and recognition");
#endif
#endif
        }

        log_d("deallocator");
        photo_deallocator(p);
        free(photo_send);
      }
      else
      {
        log_d("photo not available");
      }
    }

    vTaskDelay(20);
  }
}

void copy_buffer(uint8_t *dst, uint8_t *src, size_t len)
{
  for (size_t j = 0; j < len; j++)
  {
    dst[j] = src[j];
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