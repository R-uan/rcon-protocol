### Rcon Protocol Implementation
My attempt at the implementation of Valve's RCON Protocol [specification](https://developer.valvesoftware.com/wiki/Source_RCON_Protocol#Requests_and_Responses).

#### Features
- Thread pooling, reducing the overhead of creating and destroying threads per operation.
- Async logging, avoids the chopping of logs from multiple threads.

#### TODO
- Easier extension of commands
- Support for windows
- Multi-packet response
