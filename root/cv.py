import cv2
import numpy as numpy

cap=cv2.VideoCapture('xxx.mp4')
while cap.isOpened():
    ret,frame=cap.read()
    print(frame.shape)
