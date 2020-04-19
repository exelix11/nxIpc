# nxIpc
This is a simple header-only library for hosting IPC services on switch. 

The main purpose is building simple homebrew services without having to include libstratosphere, that's why this library is very barebones and only supports a limited set of the HOS ipc system, capabilities may or may not be extended in the future.

# Credits
The ipc server over at [sys-clk](https://github.com/retronx-team/sys-clk/) was a great documentation on implementing an IPC service