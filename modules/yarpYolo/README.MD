# YARP YOLO

## Requirements
- opencv
- ultralytics requirements

## Usage
When running the Python script, four ports will be created:
- the input image port
- the input command RPC port
- the output image port, where the result of the inference is plotted
- the output bottle port, where the pixel coordinates of the searched object are returned

The commands which can be sent to the RPC port are:
- `detect`: restore the default situation, detecting all the objects in the input image
- `label <something>`: detect all the occurrencies of "something" in the input image and the corresponding bounding boxes are showed on the output image
- `where <something>`: detect all the occurrencies of "something" in the input image and the corresponding bounding boxes are showed on the output image. Moreover, the pixel coordinates of the bbox with the highest confidence are returned