import face_recognition
import pickle
import cv2


# Initialize 'currentname' to trigger only when a new person is identified.
currentname = "unknown"
# Determine faces from encodings.pickle file model created from train_model.py
encodingsP = "encodings.pickle"

# load the known faces and embeddings along with OpenCV's Haar
# cascade for face detection
data = pickle.loads(open(encodingsP, "rb").read())

# vid = cv2.VideoCapture(0)
frame = cv2.imread('../Server/foto.jpg')
frame = cv2.resize(frame, (500, 500))

# Detect the fce boxes
boxes = face_recognition.face_locations(frame)
    # compute the facial embeddings for each face bounding box
encoding = face_recognition.face_encodings(frame, boxes)[0]
names = []


matches = face_recognition.compare_faces(data["encodings"],
                                            encoding)
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
    
print(name)