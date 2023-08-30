# approachObject

## General description
This module has been designed to perform the final movement of the robot at the end of a successful search.
When the search object is found, this module brings the robot closer to it, so that it can grab it or point it more accurately with its hand.

## Usage:
The module expects as input the name of the object followed by its pixel coordinates (camera reference frame).
Input format:   `<objectName> (<coordsX> <coordsY>)`
Example:        `ball (156 203)`

In this way, the coordinates define a point in space to which the robot approaches keeping a safe distance (editable in the .ini file).
Once approached to the object, the robot searches for the object again and returns its pixel coordinates in an output port.

You should use this module:
- using as input what is returned by the positive outcome port of the r1Obr-orchestrator module
- connecting the output to the input port of the cer handPointing module

