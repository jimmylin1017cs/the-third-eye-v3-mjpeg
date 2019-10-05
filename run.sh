#!/bin/bash

export PYTHONPATH=/home/jimmy/hscc/the-third-eye-v3-mjpeg/src

make -j8

./darknet detector demo cfg/coco.data cfg/yolov3-tiny.cfg weights/yolov3-tiny.weights /home/jimmy/hscc/test.mp4 -thresh 0.3

#./darknet detector demo cfg/coco.data cfg/yolov3.cfg weights/yolov3.weights test.MTS -thresh 0.3

#./darknet detector demo cfg/nctu_delta.data cfg/yolov3.cfg weights/nctu_delta_200.weights test.MTS -thresh 0.3
