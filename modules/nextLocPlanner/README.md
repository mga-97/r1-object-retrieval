# nextLocPlanner

Module which loads locally the locations of a map to retrieve the next robot target location to inspect.

The module opens a Navigation client which gives access to the necessary methods.
An RPC port is also opened (default name: /nextLocPlanner/request/rpc)

## Usage:
Possible RPC commands:
- `next` : responds the next unchecked location or noLocation
- `set <locationName> <status>` : sets the status of a location to unchecked, checking or checked
- `set all <status>` : sets the status of all locations
- `list` : lists all the locations and their status
- `list2` : lists all the locations divided by their status
- `stop` : stops the nextLocationPlanner module
- `help` : gets this list

Each location can have one of the following statuses: `unchecked`,`checking`, `checked` .
When the module is created, each location status is 'unchecked'.
When the `next` command is called, the first 'unchecked' location is returned to the asker, and its status is set to 'checking'.
It is supposed that a navigation orchestrator would set the location status to 'checked' after performing some task. This is possible with the command `set <locationName> checked`.
