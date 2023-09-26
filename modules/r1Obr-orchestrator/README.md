# r1Obr-orchestrator

## General description
Module to orchestrate the search for an object with the robot R1.

This has been designed as a plug-in module where the input commands can be received from various sources, and the final behaviour of the robot is defined by other modules.

While the search is performed by the goAndFindIt module, this one acts as interface with the user.

## Usage:

### Inputs
There are two ways to send commands to this module:

Sending an input bottle to the ports `/r1Obr-orchestrator/input_command:i` or `/r1Obr-orchestrator/voice_command:i` (the names can be modified in the .ini file).
The first port is designed to send commands via 'yarp write', while the second one is opened to receive voice commands (not yet implemented).
The commands that can be sent to these ports are:
- `search <object> <where>`: starts looking for "object" at location "where". In this case, if "object" is not found, the module will ask you if you want to continue the search in other locations. In this case you can answer `yes` or `no` using one of the input ports available.
- `search <object>`: starts looking for "object" in all the map starting from the closest location
- `stop`: stops the robot
- `resume`: resumes a stopped search
- `reset`: stops the robot without the possibility to resume it
- `navpos`: sets the robot in a navigation position.

Sending an RPC command to the port `/r1Obr-orchestrator/rpc`:
- `search <what>` starts looking for "what" in all the map starting from the closest location
- `search <what> <where>`: starts looking for "what" at location "where". In this case, if "what" is not found, the module will ask you if you want to continue the search in other locations. In this case you can answer `yes` or `no` using one of the input ports available.
- `what`: returns the object of the current search
- `where`: returns the location of the current search
- `stop`: stops the robot
- `resume`: resumes stopped search
- `reset`: stops the robot without the possibility to resume it
- `help`: gets this list
- `status`: returns the current status. The robot could be: idle, asking network, searching (navigating or effectively searching around), waiting for an answer, performing some kind of motion when the object is found or not found.
 - `info`: get the information about what, where and status

### Outputs
A search can have a negative or a positive outcome.
In the first case, if you have specified a location where to perform the search, you will be asked if you want to continue the search in other locations.

When a search ends without having found the object of the search, an output message is sent to the negative outcome port (default name `/r1Obr-orchestrator/negative_outcome:o`).
While if the object is found, an output message is sent to the positive outcome port (default name `/r1Obr-orchestrator/positive_outcome:o`) containing the name of the object followed by a Bottle with its pixel coordinates in the camera reference frame. 
Output format:  `<objectName> (<coordsX> <coordsY>)`
Example:        `ball (156 203)`

### Continous Search 
The continous search is an optional feature of this orchestrator. 
If it set as active, the orchestrator will check constantly, during the navigation of the robot, if the object of the search can already be found without waiting for the robot to reach a certain location.
If `<where>` has been specified in the `search` command, the continuous search will activate only in proximity of the specified location.

### Integration of the Sensor Network
Before starting looking around for the object, this module asks to the Sensor Network if it can find it, so that the robot can directly navigate to it.
[NOT IMPLEMENTED YET]