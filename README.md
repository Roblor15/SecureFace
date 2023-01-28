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

Finally, we need to install `ffmpeg` software in order to create the video from the Esp-Cam images :

```bash
$ sudo apt install ffmpeg
```

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

## Getting Started

### Esp32-CAM

client part here ...............

### Raspberry

The first step is to train the facial recognition model. In order to run the training, a dataset is needed, so you have to create a `dataset` folder in which all data must be uploaded. For each person that the model have to recognize create a **folder** inside the `dataset` folder with all the training set images. The name of the folders will be the output of the recognition script when it recognizes someone. Instead, `unknown` will be the output when the script does not recognize anyone. 

```bash
$ cd facial_req
$ mkdir dataset
$ cd dataset
$ mkdir person1
```

In our project we upload a hundred images for each person with an acceptable accuracy result. Now run the `train_model.py` to train the model on the dataset created before.

```bash
$ python3 train_model.py
```

When the training is completed the file `encoding.pickle` will be created. This file will be used from the recognition script to recognize face from an image.

Now we need to compile `server_rec.c` `server_video.c` files :

```bash
$ cd Server
$ gcc server_rec.c -o server_rec
$ gcc server_video.c -o server_video
```

### Server_rec

`server_rec` script open a **socket** and wait for a connection with client. When a connection is accepted correctly a child is created to menage the connection. 

```c
// Accept the data packet from client and verification
        connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
        if (connfd < 0)
        {
            printf("server accept failed...\n");
            exit(0);
        }
        else
            printf("server accept the client...\n");

        // Start new child process that handles the connection
        int fid = fork();
```

The child create a temp folder where create the image files received from the client. The data bytes are read from the socket and the images are created with `save_photo()` function. Once the data transmission is terminated the `run_req.py` to recognize face from images.

```c
#define REC_PROGRAM "../facial_req/run_req.py"
// Path to the .pickle file returned from facial recognition training
#define PICKLE_FILE "../facial_req/encodings.pickle"

// Arguments to pass to the recognition program
char *argv_recognition[ARG_LEN] = {"python3", REC_PROGRAM, PICKLE_FILE, "-d"};

// main()
if (fid == 0)
        {
            // Buffer used to receive photos
            uint8_t buff[MAX];

            // Creation of temporary directory for incoming photos
            char *dir_name = mkdtemp(template);

            // Add directory to face recognition arguments
            argv_recognition[ARG_LEN - 2] = dir_name;

            // Bytes read
            int bytes;
            // Read untill EOF or an error
            while ((bytes = read(connfd, buff, MAX)) > 0)
            {
                if (!save_photos(buff, bytes, dir_name))
                    // Break if the photos are finished
                    break;
            }
    
            // Overwrite the stdout with the connection fd to send the response
            dup2(connfd, STDOUT_FILENO);
            // Execute the recognition program
            execvp("python3", argv_recognition);
        }
```

In the argument of python script a `-d` flag are add to enable the erase of temp folder after recognition. This script read each images with `opencv` and find a match in the trained model. EspCam will send more then a single image, so the result will be the most common result.

### Server_video

