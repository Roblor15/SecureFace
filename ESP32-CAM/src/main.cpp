#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>

// Enable Video
#define VIDEO
// Enable Recogniton
#define RECOGNITION
// If display is used
#define LCD_DISPLAY

// The name of the Wifi network
#define WIFI_SSID "XXXXXXXXXXXXX"
// The password of the Wifi network
#define WIFI_PSW "XXXXXXXXXXXXX"

#ifdef VIDEO

// The video server IP
#define HOST_VIDEO "XXXXXXXXXXXXX"
// The video server port
#define PORT_VIDEO 8080
// Number of frames per video
#define N_VIDEO_FRAMES 200

// Counter for video frames
int video_count = 0;

#endif

#ifdef RECOGNITION

#include <HCSR04.h>

// The video server IP
#define HOST_PHOTO "XXXXXXXXXXXXX"
// The video server port
#define PORT_PHOTO 8081

// Distance in centimeters that enbles the face recognition
#define PHOTO_TRIGGER 40
// Number of frames per video
#define N_RECOGNITION_FRAMES 10

// Initialisation of Ultrasonic distance sensor (triggerPin, echoPin)
UltraSonicDistanceSensor distanceSensor(13, 15);
// Counter for recognition frames
int recognition_count = 0;

#endif

#ifdef LCD_DISPLAY

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Object that contols the LCD display (address, columns, lines)
LiquidCrystal_I2C lcd(0x27, 16, 2);

#endif

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

// Dimension of photos' queue
#define N_BUF 100

// Bitmask (pin 12) if pins used to wake up
#define BUTTON_PIN_BITMASK GPIO_SEL_12

// Define the purpose for the photo
typedef enum Purpose
{
  Video,
  Recognition,
  RecognitionEnd,
  Both
} Purpose;

// Structure that contains the Photo
typedef struct Photo
{
  uint8_t *buffer;
  size_t len;
} Photo;

// Structure that contains a Photo and its Purpose
typedef struct PhotoSend
{
  Photo *photo;
  Purpose purpose;
} PhotoSend;

// Setup the camera module
void setupCamera();
// Send the Photos to the servers
void task_0_function(void *);
// Deallocate the photo buffer
void photo_deallocator(Photo *p);
// Copy len bytes of src in dst
void copy_buffer(uint8_t *dst, uint8_t *src, size_t len);

// Queue for sending photos from a task to the other
QueueHandle_t photo_queues;

// Task handle of the sending task
TaskHandle_t task_0;

void setup()
{
  // Initialise the Serial Communication
  Serial.begin(115200);

  // Setup the camera
  setupCamera();

#ifdef LCD_DISPLAY

  // Set pins for I2C: SDA, SCL
  Wire.begin(2, 14);

  // Ititialise the display
  lcd.init();
  lcd.backlight();
  lcd.clear();

#endif

  // Create the queue with N_BUF size
  photo_queues = xQueueCreate(N_BUF, sizeof(PhotoSend *));

  // pinMode(LED, OUTPUT);

  // Wake up when high voltage is found on pins defined in BUTTON_PIN_BITMASK
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);

  // Create task for sending photos and pin to Core 0 (Main task is executed on Core 1)
  xTaskCreatePinnedToCore(
      task_0_function,
      "Send photos",
      3050,
      NULL,
      0,
      &task_0,
      0);

  delay(1000);
}

// Boolean value that indicate when send photos
bool send = true;
// Boolean value that indicate if task_0 has finished
bool finished = false;

// Value used to save the time when the photo is taken
unsigned long photo_timestamp = 0;
// Value set when the main task has finished
unsigned long finished_time = 0;

void loop()
{

  uint64_t now = millis();

  // Execute this every 100ms
  if (send && now - photo_timestamp > 100)
  {
    // Update photo_timestamp
    photo_timestamp = now;

    // Take a photo
    camera_fb_t *fb = nullptr;

    fb = esp_camera_fb_get();

    // Restart if some errors
    if (!fb)
    {
      Serial.println("Camera capture failed");
#ifdef LCD_DISPLAY
      // Print on the display the error
      lcd.clear();
      lcd.print("Camera capture failed");
      delay(1000);
#endif

      ESP.restart();
      return;
    }

    // Allocate space in the psram for copying the photo
    Photo *p = (Photo *)ps_malloc(sizeof(Photo));

    // Copy the photo
    p->buffer = (uint8_t *)ps_malloc(sizeof(uint8_t) * fb->len);
    copy_buffer(p->buffer, fb->buf, fb->len);
    p->len = fb->len;

    // Prepare the camera for the next photo
    esp_camera_fb_return(fb);

    log_d("heap : %d", ESP.getFreeHeap());
    log_d("psram: %d", ESP.getFreePsram());

    PhotoSend *photo_send = new PhotoSend;
    photo_send->photo = p;

#if defined(RECOGNITION) && defined(VIDEO)
    // Set the purpose of the photo
    if (recognition_count || distanceSensor.measureDistanceCm(25) <= PHOTO_TRIGGER)
    {
      log_d("sending recognition");
      photo_send->purpose = Purpose::Recognition;
      ++recognition_count;

#ifdef LCD_DISPLAY
      if (recognition_count == 1)
      {
        // Print on the display that the cam is making the face recognition
        lcd.clear();
        lcd.print("Recognition....");
      }
#endif
    }
    else
    {
      log_d("sending video");
      photo_send->purpose = Purpose::Video;
      ++video_count;

#ifdef LCD_DISPLAY
      if (video_count == 1)
      {
        // Print on the display that the cam is recording
        lcd.clear();
        lcd.print("Recording....");
      }
#endif
    }
#elif defined(RECOGNITION)
    // The fist time wait for the face to recognise
    ++recognition_count;

    if (recognition_count == 1)
    {
#ifdef LCD_DISPLAY
      if (recognition_count == 1)
      {
        // Print that the face is too distant for recognition
        lcd.clear();
        lcd.print("Face too distant");
      }
#endif

      while (distanceSensor.measureDistanceCm(25) > PHOTO_TRIGGER)
      {
        log_d("face too distant");
        delay(1000);
      }
    }

    log_d("sending recognition");
    photo_send->purpose = Purpose::Recognition;

#ifdef LCD_DISPLAY
    if (recognition_count == 1)
    {
      // Print on the display that the cam is making the face recognition
      lcd.clear();
      lcd.print("Recognition....");
    }
#endif

#elif defined(VIDEO)
    log_d("sending video");
    photo_send->purpose = Purpose::Video;
    ++video_count;

#ifdef LCD_DISPLAY
    // Print on the display that the cam is recording
    if (video_count == 1)
    {
      lcd.clear();
      lcd.print("Recording....");
    }
#endif
#endif

    // Send the photo to the sending task
    if (!xQueueSend(photo_queues, (void *)&photo_send, 2 * 60 * 1000 / portTICK_PERIOD_MS))
    {
      vTaskDelete(task_0);
      log_d("going to sleep");

#ifdef LCD_DISPLAY
      lcd.noBacklight();
#endif

      delay(2000);
      esp_deep_sleep_start();
    }

#ifdef VIDEO
    // If the wanted number of frames is reached set send = false
    if (video_count == N_VIDEO_FRAMES)
    {
      send = false;
      finished_time = millis();
    }
#endif
#ifdef RECOGNITION
    if (recognition_count == N_RECOGNITION_FRAMES)
    {
      // Send the end of the frames to the server
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

#ifdef LCD_DISPLAY
        lcd.clear();
        lcd.print("Going to sleep..");
        lcd.noBacklight();
        lcd.clear();
#endif

        delay(2000);
        esp_deep_sleep_start();
      }

      send = false;
      finished_time = millis();
    }
#endif
  }
  // If sending task has finished delete it end go to sleep
  else if (finished)
  {
    vTaskDelete(task_0);
    log_d("going to sleep");

#ifdef LCD_DISPLAY
    lcd.clear();
    lcd.print("Going to sleep..");
    lcd.noBacklight();
    lcd.clear();
#endif

    delay(2000);
    esp_deep_sleep_start();
  }
  // If the sending task has not finished in 2 minutes, end it and go to sleep
  else if (!send && now - finished_time > 60 * 2 * 1000)
  {
    vTaskDelete(task_0);
    log_d("going to sleep");

#ifdef LCD_DISPLAY
    lcd.clear();
    lcd.print("Going to sleep..");
    lcd.noBacklight();
    lcd.clear();
#endif

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

  /*   sensor_t *s = esp_camera_sensor_get();
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
    s->set_vflip(s, 0);                      // 0 = disable , 1 = enable
    s->set_dcw(s, 1);                        // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);                   // 0 = disable , 1 = enable */
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
  // Connect to the video server
  while (!video_client.connect(HOST_VIDEO, PORT_VIDEO))
  {
    Serial.println("Connecting to video host");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
#endif

#ifdef RECOGNITION
  // Connect to the recognition server
  while (!recognition_client.connect(HOST_PHOTO, PORT_PHOTO))
  {
    Serial.println("Connecting to recognition host");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  // Set the timeout to 20s (used with readString)
  recognition_client.setTimeout(120);
#endif

  for (;;)
  {
    log_d("%d", send);
    PhotoSend *photo_send = nullptr;

    // Receive photo from the queue
    int r = xQueueReceive(photo_queues, &(photo_send), 10 * 1000 / portTICK_PERIOD_MS);

    // If there are no photos stop connections with servers and Wifi
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

      // Set that the task has finished
      finished = true;
      log_d("heap: %d", ESP.getFreeHeap());
      log_d("psram: %d", ESP.getFreePsram());

      return;
    }
    else
    {
      if (r)
      {
        // Get photo
        Photo *p = photo_send->photo;

        // Send the photo based on its purpose
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

          String name = recognition_client.readStringUntil('\n');

          log_d("%s", name);

#ifdef LCD_DISPLAY
          lcd.clear();
          if (name != "Unknown")
          {
            lcd.print("Welcome");
            lcd.setCursor(0, 1);
            lcd.print(name);
          }
          else
          {
            lcd.print("User not");
            lcd.setCursor(0, 1);
            lcd.print("identified");
          }
#endif
#endif
        }
        else if (photo_send->purpose == Purpose::Both)
        {
#ifdef defined(RECOGNITION) && defined(VIDEO)
          video_client.write(p->buffer, p->len);
          video_client.flush();

          recognition_client.write(p->buffer, p->len);
          recognition_client.flush();

          log_d("sended video and recognition");
#endif
        }

        log_d("deallocator");
        // Deallocate the buffer of the photo
        photo_deallocator(p);
        // Deallocate photo_send
        free(photo_send);
      }
      else
      {
        log_d("photo not available");
      }
    }

    // 10ms delay
    vTaskDelay(10 / portTICK_PERIOD_MS);
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