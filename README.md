# SecureFace

## Requirments

## Software Requirment

### Esp32-CAM

We suggest to install the VScode extension called 
### Raspberry

## Project Layout

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
