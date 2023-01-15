import face_recognition
import pickle
import cv2
import sys
import collections
import os


def recognition(path):
    # vid = cv2.VideoCapture(0)
    frame = cv2.imread(path)
    frame = cv2.resize(frame, (500, 500))

    # Detect the fce boxes
    boxes = face_recognition.face_locations(frame)

    # compute the facial embeddings for each face bounding box
    encoding = face_recognition.face_encodings(frame, boxes)[0]

    matches = face_recognition.compare_faces(data["encodings"], encoding)
    name = "Unknown"  # if face is not recognized, then print Unknown

    # check to see if we have found a match
    if True in matches:

        # find the indexes of all matched faces then initialize a
        # dictionary to count the total number of times each face
        # was matched
        matchedIdxs = [i for (i, b) in enumerate(matches) if b]
        counts = {}

        # loop over the matched indexes and maintain a count for
        # each recognized face face
        for i in matchedIdxs:
            name = data["names"][i]
            counts[name] = counts.get(name, 0) + 1

        # determine the recognized face with the largest number
        # of votes (note: in the event of an unlikely tie Python
        # will select first entry in the dictionary)
        name = max(counts, key=counts.get)

    return name


def remove(path):
    if os.path.isdir(path):
        for p in os.listdir(path):
            remove(os.path.join(path, p))

        os.rmdir(path)
    elif os.path.exists(path):
        os.remove(path)


# Determine faces from encodings.pickle file model created from train_model.py
encodingsP = sys.argv[1]

# load the known faces and embeddings along with OpenCV's Haar
# cascade for face detection
data = pickle.loads(open(encodingsP, "rb").read())

photos = sys.argv[2:]

default_photo = 'foto.jpg'

delete = False

if len(photos) == 0:
    print(recognition(default_photo))
else:
    names = []

    for path in photos:
        if path == '--delete' or path == '-d':
            delete = True
        elif os.path.isdir(path):
            for photo in os.listdir(path):
                try:
                    names.append(recognition(os.path.join(path, photo)))
                except:
                    names.append("Unknown")

        elif os.path.exists(path):
            try:
                names.append(recognition(path))
            except:
                names.append("Unknown")

    if delete:
        for path in photos:
            if path != '--delete' and path != '-d':
                remove(path)

    counter = collections.Counter(names)
    print(counter.most_common(1)[0][0])
