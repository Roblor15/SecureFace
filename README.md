# SecureFace

## Requirements

### Materials

For this project we used:

- Esp32cam Ai-Thinker board.
- PIR sensor (HC-SR501)
- Ultrasonic sensor (HC-SR04)
- LCD Display (1602) with I2C module

If an other version of the board is used you have to modify the cam's pins with the right [ones](ESP32-CAM/src/main.cpp#L61).

```c
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
```

The ultrasonic sensor is required only if the face recognition is active in order to understand if a person is in front of the cam.

The LCD Display is optional, if you can't use one you can see the response of the face recognition in the serial monitor.

### Connections

You can modify all the pins we used, but with the esp32cam Ai-Thinker we suggest to not use GPIO0 and GPIO16. The former is used as source of the clock for the camera and the latter is used to activate the PSRAM (if your board has one).

#### PIR Sensor

You can follow [this guide](http://win.adrirobot.it/sensori/pir_sensor/pir_sensor_hc-sr501_arduino.htm) to set the sensor in auto-reset mode and, if you want, to adjust the sensitivity and the output timing. We used the default values. Connect the output pin to GPIO12.

If you choose another GPIO pin remeber to change also the value of the [bitmask](ESP32-CAM/src/main.cpp#L82).

#### Ultrasonic Sensor

We simply connected the trigger pin to GPIO13 and the echo pin to GPIO15 as you can see [here](ESP32-CAM/src/main.cpp#L46).

#### LCD Disply

## Software Requirement

### Esp32-CAM

We suggest to install the VS-code extension called [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) where import the `ESP-32CAM` folder project. With this extention all the library and the architecture dependencies will be import automatically. Other IDE could be used instead, all the dependencies and configuration are written in the `platformio.ini` file.

```ini
[env:esp32cam]
platform = espressif32
board = esp32cam
framework = arduino
monitor_speed = 115200
board_build.partitions = min_spiffs.csv
build_flags =
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
    # -DCORE_DEBUG_LEVEL=5        # activate to receive debugging infos
lib_deps = martinsos/HCSR04@^2.0.0
```

### Raspberry

The raspberry environment only need python and a cpp compiler that are already installed in the [Raspbian](https://www.raspbian.org/) distro and some python library that could  be easily installed with [PIP](https://pypi.org/).

```bash
$ sudo apt update && sudo apt upgrade
$ sudo apt install python3-pip
```

You need to install `opencv-headless` and  `face-recognition`, but before it is needed to expand the swapfile from `100` to `2048`:

```bash
$ sudo nano /etc/dphys-swapfile
```

```bash
# /etc/dphys-swapfile - user settings for dphys-swapfile package
# author Neil Franklin, last modification 2010.05.05
# copyright ETH Zuerich Physics Departement
#   use under either modified/non-advertising BSD or GPL license

# this file is sourced with . so full normal sh syntax applies

# the default settings are added as commented out CONF_*=* lines


# where we want the swapfile to be, this is the default
#CONF_SWAPFILE=/var/swap

# set size to absolute value, leaving empty (default) then uses computed value
#   you most likely don't want this, unless you have an special disk situation
# CONF_SWAPSIZE=100		#COMMENT THIS LINE
CONF_SWAPSIZE=2048
```

And restart swapfile to take effect :

```bash
$ sudo systemctl restart dphys-swapfile
```

Now we can run the `requirements.txt` file to install the dependencies :

```bash
$ cd SecureFace/facial_req/
$ pip install -r requirements.txt
```

The installation process could need some hour to be complete. After the installation, we need to **restore the swapfile with `100`** by running the same two commands.

After these commands the installation process is terminated.

## Project Layout!

```
├── README.md                
├── ESP-32CAM
│   ├── platformio.ini				# platformio configuration file
│   ├── include                
│   ├── lib 
│   ├── test             
│   └── src
│   	└── main.cpp				# script for Esp32-CAM                           
├── Server
│   ├── test
│   │	└── send_photo.c			# test server                
│   ├── server_video.c 			# script for video                  
│   └── server_rec.c				# script for face recognition
└── facial_request
    ├── encodings.pickle                	# faces train model
    ├── haarcascade_frontalface_default.xml	# frontal face trained model
    ├── run_req.py       			# script to recognize faces
    ├── train_model.py           		# script for training model
    └── shell.nix 				# configuration file for nix-shell

```

## Getting started

### Esp32Cam

The code for the booard is made in a modulare way. You can comment/uncomment the following [macros](ESP32-CAM/src/main.cpp#L6) to enable/disable some features.

```c
// Enable Video
#define VIDEO
// Enable Recogniton
#define RECOGNITION
// If display is used
#define LCD_DISPLAY
```

When VIDEO is enable the camera starts taking photos for the server that makes the video.

When RECOGNITION is enable only if the ultrasonic sensor measure a distance minor than [PHOTO_TRIGGER](ESP32-CAM/src/main.cpp#L41), the camera interrupt the video (if enabled) and start taking photos for the recognition server.

The LCD_DISPLAY macro enable the print on an external display.

Before uploading the code there are some mocros that you have to set:

- [WIFI_SSID](ESP32-CAM/src/main.cpp#L13) your wifi network name
- [WIFI_PSW](ESP32-CAM/src/main.cpp#L15) you wifi password
- [HOST_VIDEO](ESP32-CAM/src/main.cpp#L20) the IP or the hostname of the video server
- [HOST_PHOTO](ESP32-CAM/src/main.cpp#L36) the IP or the hostname of the recognition server

In order to upload the code on the board you can proceed either with the VS-code extension or with the CLI.

#### VS-code extension

In VS-code you will see the toolbar shown down here. To upload the code click the arrow (third item in the toolbar)

<img src="Images/platformio-ide-vscode-toolbar.png" alt="toolbar" width="300"/>

#### CLI

With the terminal go in the [ESP32-CAM](ESP32-CAM/) and run:

```bash
$ pio run -t upload
```
