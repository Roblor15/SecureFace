import face_recognition
import pickle
import cv2
import sys
import collections
import os

# MIT License

# Copyright (c) 2021 Caroline Dunn

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

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


def remove(path):
    if os.path.isdir(path):
        for p in os.listdir(path):
            remove(os.path.join(path, p))

        os.rmdir(path)
    elif os.path.exists(path):
        os.remove(path)


# Determine faces from encodings.pickle file model created from train_model.py
encodingsP = sys.argv[1]

# Load the known faces and embeddings along with OpenCV's Haar
# cascade for face detection
data = pickle.loads(open(encodingsP, "rb").read())

# Photos or directory of photo to analyse
photos = sys.argv[2:]

# Remove photos after recognition
delete = False

if len(photos) == 0:
    print("No photo")
else:
    # List of results of evre face recognition
    names = []

    # Iterates over the arguments
    for path in photos:
        if path == '--delete' or path == '-d':
            delete = True
        # If directory analyse its content
        elif os.path.isdir(path):
            for photo in os.listdir(path):
                try:
                    names.append(recognition(os.path.join(path, photo)))
                except:
                    names.append("Unknown")
        # Check if photo exists
        elif os.path.exists(path):
            try:
                names.append(recognition(path))
            except:
                names.append("Unknown")

    # If wanted delete the photos
    if delete:
        for path in photos:
            if path != '--delete' and path != '-d':
                remove(path)

    # Counts the occurances of the results
    counter = collections.Counter(names)

    # Print the most common result
    print(counter.most_common(1)[0][0])
