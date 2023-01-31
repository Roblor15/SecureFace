# SecureFace

SecureFace is an application that recognizes a person's identity by their face and creates a short video when motion is detected. When a person comes within 30 cm of the ultrasonic sensor, the camera is activated and takes photos. The Esp32cam integrated with the camera sends the photos over Wi-Fi to a Raspberry device, which uses a recognition function to identify the face and return the result. The LCD then displays the recognized person's name or an error message. Instead if motion is detected using the PIR sensor, a batch of images is sent to the Raspberry server, which creates and stores the video of the suspicious scene.

## Requirements

### Materials

For this project we used:

- Raspberry (suggested pi 3/4 )
- Esp32cam Ai-Thinker board
- FTDI232 (USB to serial converter)
- PIR sensor (HC-SR501)
- Ultrasonic sensor (HC-SR04)
- LCD Display (1602) with I2C module

If an other version of the board is used you have to modify the cam's pins with the right [ones](ESP32-CAM/src/main.cpp#L63).

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

If you choose another GPIO pin remeber to change also the value of the [bitmask](ESP32-CAM/src/main.cpp#L84).

#### Ultrasonic Sensor

We simply connected the trigger pin to GPIO13 and the echo pin to GPIO15 as you can see [here](ESP32-CAM/src/main.cpp#L46).

#### LCD Display

In order to use less pins (the pins of the esp32cam were not enough) we used the I2C adapater for the display, that uses only two pins GPIO14 and GPIO2 (SDA and SCL). The [address](ESP32-CAM/src/main.cpp#L58) of our display is 0x27 but it could be another one, tipically 0x3F.

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
lib_deps = 
    martinsos/HCSR04@^2.0.0
    marcoschwartz/LiquidCrystal_I2C@^1.1.4
```

Otherwise you can use also the [PlatformIO CLI](https://docs.platformio.org/en/stable/core/index.html).

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
└── facial_rec
    ├── encodings.pickle                	# faces train model
    ├── haarcascade_frontalface_default.xml	# frontal face trained model
    ├── run_rec.py       			# script to recognize faces
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

Before uploading the code there are some macros that you have to set:

- [WIFI_SSID](ESP32-CAM/src/main.cpp#L13) your wifi network name
- [WIFI_PSW](ESP32-CAM/src/main.cpp#L15) you wifi password
- [HOST_VIDEO](ESP32-CAM/src/main.cpp#L20) the IP or the hostname of the video server
- [HOST_PHOTO](ESP32-CAM/src/main.cpp#L36) the IP or the hostname of the recognition server

In order to upload the code on the board you can proceed either with the VS-code extension or with the CLI, using the [FTDI232](https://randomnerdtutorials.com/program-upload-code-esp32-cam/).

#### VS-code extension

In VS-code you will see the toolbar shown down here. To upload the code click the arrow (third item in the toolbar)

<img src="Images/platformio-ide-vscode-toolbar.png" alt="toolbar" width="300"/>

#### CLI

With the terminal go in the [ESP32-CAM](ESP32-CAM/) and run:

```bash
$ pio run -t upload
```

### Raspberry

The first step is to train the facial recognition model. To run the training, a dataset is needed. To create this dataset, you must create a `dataset` folder, and then add a folder for each person that the model must recognize. The name of these folders will be the output of the recognition script when it recognizes someone, while "unknown" will be the output when the script doesn't recognize anyone. In our project, we uploaded a hundred images per person with an acceptable accuracy result.

```bash
$ cd facial_rec
$ mkdir dataset
$ cd dataset
$ mkdir person1
```

Next, run `train_model.py` to train the model on the dataset you created.

```bash
$ python3 train_model.py
```

When the training is complete, the file `encoding.pickle` will be created. This file will be used to recognize faces.

Now you need to compile the `server_rec.c` and `server_video.c` files:

```bash
$ cd Server
$ gcc server_rec.c -o server_rec
$ gcc server_video.c -o server_video
```

### Server_rec.c

The `server_rec.rec` script opens a socket and waits for a connection from a client. When a connection is established, a child process is created to manage the connection.

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

The child process creates a temporary folder where it saves the image files received from the client. The data bytes are read from the socket and saved as images with the `save_photo()` function. Once the data transmission is complete, the `run_req.py` script is used to recognize faces by matching source images with the `encodings.pickle` file.

```c
#define REC_PROGRAM "../facial_rec/run_rec.py"
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

In the argument of python script a `-d` flag are add to enable the erase of temp folder after recognition.

### Server_video.c

The `server_video` script opens a socket and waits for a connection from a client. When a connection is established, a child process is created. This child process calls `ffmpeg` to create a video from the image bytes received. The child process overwrites `stdin` with the socket stream so that `ffmpeg` can take the image bytes from the input stream.

```c
// If child process
        if (fid == 0)
        {
            // Overwrite the stdin with the connection fd to receive the photos
            dup2(connfd, 0);

            // Create the filename for the video
            char filename[20];
            sprintf(filename, "video-%d.avi", counter);

            // Execute ffmpeg with images as input
            execlp("ffmpeg", "ffmpeg", "-loglevel", "debug", "-y", "-f", "image2pipe", "-vcodec", "mjpeg", "-r",
                   "10", "-i", "-", "-vcodec", "mpeg4", "-qscale", "5", "-r", "10", filename, NULL);
        }
```

### Run_rec.py

The `run_rec.py` script calls the `recognition()` function on each image in the input folder. The function uses `OpenCV` to open the image, detect face bounding boxes, and the `face_recognition` library compute matches with the `encodings.pickle` file. The function returns the name of the best matching index.

```python
def recognition(path):
    # Load the image
    frame = cv2.imread(path)
    # Resize the image
    frame = cv2.resize(frame, (500, 500))
    # Detect the face boxes
    boxes = face_recognition.face_locations(frame)
    # Compute the facial embeddings for each face bounding box
    encoding = face_recognition.face_encodings(frame, boxes)[0]
    matches = face_recognition.compare_faces(data["encodings"], encoding)
    name = "Unknown"  # If face is not recognized, then print Unknown
    # Check to see if we have found a match
    if True in matches:
        # Find the indexes of all matched faces then initialize a
        # dictionary to count the total number of times each face
        # was matched
        matchedIdxs = [i for (i, b) in enumerate(matches) if b]
        counts = {}
        # Loop over the matched indexes and maintain a count for
        # each recognized face face
        for i in matchedIdxs:
            name = data["names"][i]
            counts[name] = counts.get(name, 0) + 1
        # Determine the recognized face with the largest number
        # of votes (note: in the event of an unlikely tie Python
        # will select first entry in the dictionary)
        name = max(counts, key=counts.get)
    return name
```

The script returns the most common name returned by the `recognition()` function from all images.

```python
for photo in os.listdir(path):
                try:
                    names.append(recognition(os.path.join(path, photo)))   # call recognition function for each image
                except:
                    names.append("Unknown")
                    
# -------------------------------------------------------------
 # Counts the occurances of the results
    counter = collections.Counter(names)

    # Print the most common result
    print(counter.most_common(1)[0][0]) # compute the most common index
```
## Other material
[SecureFace Presentation](SecureFace_presentation.pdf)

[SecureFace video](https://www.youtube.com/watch?v=Rwgl8lMb24U&ab_channel=FabioGrotto)

## Acknowledgments

Students: <a href="https://github.com/enrico-car">Enrico Carnelos</a> - <a href="https://github.com/Roblor15">Roberto Lorenzon</a> - <a href="https://github.com/fabiogr8">Fabio Grotto</a>

Embedded Software for the Internet of Things Course - Professor: <a href="https://webapps.unitn.it/du/it/Persona/PER0212812/Didattica">Yildrim Kasim Sinan</a>

<a href="https://www.unitn.it/"><img src="./Images/unitn_logo.png" width="300px"></a>

## Copyright
MIT Licence or otherwise specified. See [license file](LICENCE) for details.
