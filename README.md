# SecureFace

## Requirements

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
    -DCORE_DEBUG_LEVEL=5
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

## 
