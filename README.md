
<p align="center">
<img alt="eventdispatcher" title="C++ Event Dispatcher using poll() to write concurrent tasks in a single thread."
src="https://raw.githubusercontent.com/m2osw/eventdispatcher/master/doc/eventdispatcher.png" width="321" height="320"/>
</p>

# Introduction

The eventdispatcher is our Snap! network library. It allows us to
communicate between all our service between any number of computers.

It was first part of our libsnapwebsites as:

* `snap_communictor.cpp/.h`
* `snap_communictor_dispatcher.cpp/.h`
* `tcp_client_server.cpp/.h`
* `udp_client_server.cpp/.h`

Now these are all broken up in separate files (84 of them at time of
writing) and work with support from libraries found in our contrib folder.

## Features

The Event Dispatcher is capable of many feats, here are the main features
found in this library:

* TCP -- plain and encrypted (TLS)
* UDP -- direct and broadcast
* RPC -- text based message communication
* dispatcher -- automatically dispatch RPC messages
* events -- support many file descriptor based events
  - accept socket (server)
  - socket read
  - socket write
  - permanent socket (auto-reconnect on loss of connection)
  - unix signal
  - unix pipe (read/write)
  - thread done
  - listen to file changes on your local storage devices
  - any number of timers
* priority control -- objects can be given a priority
* easy enable/disable of objects

There are also research projects  which are not yet fully functional but
if you need such functionality you may be interested in finding out how
to make it work from our existing code:

* attempt to avoid the SIGPROF from breaking our code by handling the signal
* snaploggerd -- a service to send log messages over the network


# License

The project is covered by the GPL 2.0 license.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/eventdispatcher/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
