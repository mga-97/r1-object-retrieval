# YARP MDETR

## Requirements
- opencv
- see python_code/requirements.txt

## Usage
When running the Python script, three ports will be created:
- the input image port
- the output image port, where the result of the inference is plotted
- the input command RPC port

The commands which can be sent to the RPC port are:
- `caption <something>`: one or more bounding boxes for "something" are created on the output image
- `where <something>`: one or more bounding boxes for "something" are created on the output image, and the pixel coordinates of the bbox with the highest confidence are returned