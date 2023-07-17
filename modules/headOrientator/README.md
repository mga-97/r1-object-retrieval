# headOrientator

Module to let the robot move its head around nine pre-defined orientations.
The positions of the head are, in order:
_ straight ahead
_ left
_ right
_ up
_ up left
_ up right
_ down
_ down left
_ down right

The head positions can be defined:
- setting manually the orientation of the head (pitch and yaw) in the HEAD_POSITIONS group of the .ini file 
- automatically, optimizing the head orientations considering the horizonatal and vertical fov of the camera

The module opens a RGBDCamera client and a Remote Control Board client which give access to the necessary methods.
An RPC port is also opened (default name: /headOrientator/request/rpc)

## Usage:
Possible RPC commands:
- `next` : responds the next unchecked orientation or "noOrient"
- `set <orientation> <status>` : sets the status of a location to unchecked, checking or checked
- `set all <status>` : sets the status of all orientations
- `list` : lists all the orientations and their status
- `home` : let the robot head go back to its home position
- `stop` : stops the headOrientator module
- `help` : gets this list

Each head orientation can have one of the following statuses: `unchecked`,`checking`, `checked` .
When the module is created, each orientation status is 'unchecked'.
When the `next` command is called, the first 'unchecked' orientation is returned to the asker, and its status is set to 'checking'.
It is supposed that an object recognition algorithm would set the orientation status to 'checked' after performing its task. This is possible with the command `set <orientation> checked`.
