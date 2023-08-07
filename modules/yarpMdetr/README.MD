# YARP MDETR

## Requirements
- opencv
- see mdetr_python_code/requirements.txt

## Usage
When running the Python script, four ports will be created:
- the input image port
- the input command RPC port
- the output image port, where the result of the inference is plotted
- the output bottle port, where the pixel coordinates of the searched object are returned

The commands which can be sent to the RPC port are:
- `label <something>`: one or more bounding boxes for "something" are created on the output image
- `where <something>`: one or more bounding boxes for "something" are created on the output image, and the pixel coordinates of the bbox with the highest confidence are returned