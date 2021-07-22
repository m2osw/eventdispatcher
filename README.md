
<p align="center">
<img alt="eventdispatcher" title="C++ Event Dispatcher using poll() to write concurrent tasks in a single thread."
src="https://raw.githubusercontent.com/m2osw/eventdispatcher/master/doc/eventdispatcher.png" width="321" height="320"/>
</p>

# Introduction

The eventdispatcher is our Snap! network library. It allows us to
communicate between all our services between any number of computers.

We wanted all of our services to be event based. This library does that
automatically with a very large number of features (TCP, UDP, FIFO, files,
timers, etc.)

It was first part of our libsnapwebsites as:

* `snap_communictor.cpp/.h`
* `snap_communictor_dispatcher.cpp/.h`
* `tcp_client_server.cpp/.h`
* `udp_client_server.cpp/.h`

Now these are all broken up in separate files (84 of them at time of
writing) and work with support from libraries found in our
[snapcpp](https://github.com/m2osw/) contrib folder.


## Features

The Event Dispatcher is capable of many feats, here are the main features
found in this library:

* TCP -- plain and encrypted (TLS with OpenSSL)
* UDP -- direct and two broadcast methods
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


## Library Status

The library is considered functional. It is already in use in a couple of
projects and works as expected.

The library being really large, all the features are not fully tested in
those projects, so if you run into issues, let us know (see Bug Report
below).

We plan in having full test coverage one day, but we have a problem with
the fact that the communicator is a singleton. So we've been thinking about
how to properly handle the testing of such a complex library.


#  Basic Principals

The library comes with three main parts:

* Utilities -- a few of the functions are just utilities, such as helper
functions and base classes

* Connections -- many of the classes are named `<...>_connection`; these
are used to create a connection or a listener; we currently support TCP,
UDP, Unix sockets, FIFO, Unix signals, file system events

* Communicator -- the communictor which is the core of the system; you can
get its instance and call the run() function to run your process loop; the
run function exits once all the connections are closed and removed from
the communicator

The idea is to create processes which are 100% event based. These work by
creating at least one connection and then adding that connection to the
communicator. Then you call the `run()` function of the communicator. Your
connection `process_<name>()` functions will then get called as events occur.

While running, you can add and remove connections at will.

Connections can be disabled and assigned a priority. The priority is useful
if you want some of your `process_<name>()` called in a specific order.


## Timers

The base `connection` class is viewed as a timer, so that means all your
connections also support a timer. If you create a connection object which
is just a timer, then use the `timer` class. It will make your intend clearer
and the class will simplify your own implementation.

All of our timers make use of the `poll()` timeout feature. We manage when/how
timers trigger the `process_timeout()` function in the communicator `run()`
function.

We have two types of timers. One which is triggered only once and one which
can be triggered at specific times. The first one will always be triggered
at least once when its time comes up. The second may miss triggers,
especially if the service processing is generally slow or the time elapsed
between triggers is very small.

### Events Without Timers

In most cases you should strive in creating systems where timers are not
required. The use of a timer usually means you are polling for a resource
when in most likelihood you should be listening for an event and only use
the resource once available.

There are, of course, exception to the rule. For example, our TCP client
permanent message connection does a poll based on a timer. There is just
no way to know whether a remote service is currently listening or not.
So the only way to test that is attempt a connection. If that attempt
fails, sleep a little and try again. In our case, we like to use a
slippery time wait. The very first time, try to connect immediately. If
that fails, we wait just 1 second and try again. For each new attempt,
wait two times more than last time (so 2, 4, 8 etc. seconds) up to a
maximum (say 1 hour between attempts). This generally works well,
especially if you know that the service may be gone for good, trying to
connect once per second is awful for everyone (client, server, and all
the electronics in between which creates interferences on all your servers
on your LAN).


## Connection Layers

### TCP

The TCP classes have five layers:

* TCP class -- connect/disconnect sockets
* Base class -- this is the simplest which just connects and calls your
`process_read()` and `process_write()` functions
* Buffered Class -- this class implements the `process_read()` and
`process_write()` functions and bufferize the data for you.
* Message Class -- the bufferized classes are further derived to create a
message class which sees the incoming and ougoing data as IPC like messages
(see the `message.h/cpp` files for details about the message support)
* Permanent Class -- the TCP Message Class can also be made _permanent_; this
means if the connection is lost, the class automatically takes care of
reconnecting which all happens under the hood for you.
* Blocking Message Class -- this classs is similar to the Message Class except
it is possible to send a message and wait for the reply with a standard C++
call.

We have two types of clients. The ones that a client creates (such as
the permanent message connection) and the ones that a server creates
(the `tcp_server_client_connection`). The server client does not include
a permanent message connection since it is not responsible to re-connect
such a connection if it loses it.

The TCP server does not have any kind of buffer or message support since
the only thing is does is a `listen()` and an `accept()`.

### UDP

The UDP classes are more limited than the TCP classes, especially since it
doesn't require a permanent connection class (i.e. it is a connectionless
protocol).

* Base Class -- handle some common functions for the client & server
* Client Message -- the UDP protocol being connectionless, we just offer a
standalone function to send messages over UDP; keep in mind that UDP packets
are small and we have no special handling for large message (i.e. the limit
is around 1.5Kb)
* Server Message -- the UDP server has a specific message connection and is
capable of using the message dispather.

### Unix Sockets

We have support for the stream Unix sockets. This is similar to the TCP
socket. Like with any Unix socket, it is only available to clients
running on the same computer the services they want to connect to.

Many of our services want to connect to the communicator service which
is then in charge of forwarding the messages to other computers. That
simplifies many of the communications. It is also capable of broadcasting
as per rules setup in the communicator.

The Unix socket implementation is often used as an extra connection
availability on a service. That way a service can be used via TCP or
a Unix socket (and at times also with UDP).

The following are the classes available with the streaming Unix socket
implementation:

* Base class -- this is the simplest which just connects and calls your
`process_read()` and `process_write()` functions
* Buffered Class -- this class implements the `process_read()` and
`process_write()` functions and bufferize the data for you.
* Message Class -- the bufferized classes are further derived to create a
message class which sees the incoming and ougoing data as IPC like messages
(see the `message.h/cpp` files for details about the message support)
* Permanent Class -- the Message Class can also be made _permanent_; this
means if the connection is lost, the class automatically takes care of
reconnecting which all happens under the hood for you.
* Blocking Message Class -- this classs is similar to the Message Class except
it is possible to send a message and wait for the reply with a standard C++
call.

The Permanent Class is useful when your client connects to a service which
may be restarted at any time (which is pretty much 100% of the time!) This
makes your clients very much more stable.

### Unix FIFO

The Unix FIFOs (see `pipe(2)`) can be used with the library. They also
have multiple layers:

* Base Class -- a simple read/write FIFO, you are 100% responsible for the
handling of the data
* Buffer Class -- a simple read/write FIFO which bufferize the data for you
* Message Class -- the full featured FIFO implementation which accepts and
sends messages
* Permanent Class -- the full featured FIFO implementation accepting and
sending messages which auto-reconnect on a loss of connectivity


## Message Dispatcher

For the connections that support messages, they are actually implemented
along the message dispatcher. This allows you to create a table of message
names attached to a function to execute when that message is received.
As a result, you totally avoid having a large `switch()` statement in your
code.

The dispatcher message matching supports patterns and user callbacks, so
multiple messages can call the same function (i.e. you could implement a
function that accepts a message named `EVENT<number>`, said function can
then transform `<number>` to an integer and use that to further handle the
message; however, in most cases you should use a message parameter for such
a feature).

The dispatcher is very powerful and includes many default message handling.
For example, our convention supports a HELP message which has to reply
with a list of all the messages that your service supports. This allows
us to know whether to forward a given message or not (i.e. if you do not
support a given message, sending it to you would result in an UNKNOWN
reply which is useless).



# Missing Features

* Full IPv6 Support

    We need to make sure that all our classes support IPv6 as expected.

* Use of libaddr everywhere

    The snapcommunicator version used a string for the address and an
    integer for the port. At this time the library still accepts those
    parameters.

    The new version should accept an `addr::addr` instead leaving the
    parsing work to the caller (so it can be specialized as per the
    caller's need).

    This will also help with the IPv6 support since the `addr::addr`
    objects make all addresses an IPv6 address.

* Full multithread support

    The library depends on our `cppthread` library so it has full access
    to all the multithread features we have available. However, at the
    moment, most of the functions are not multithread safe. We need to
    add such support (i.e. mutex + guards).

    Until this is complete you either have to protect your threads really
    well or use your own layer to handle messages in one specific thread
    through the `ed::communicator` and your other thread to do work. Our
    current implementation, though, works really well with services that
    use threads only to do _instant_ work or do not really need to use
    threads at all. We've successfully use the library in all three types
    of processes.





# Research Projects

There are also research projects, which are not yet fully functional but
if you need such functionality you may be interested in finding out how
to make it work from our existing code:

* attempt to avoid the SIGPROF from breaking our code by handling the signal

* poll for `listen()` calls and signal when detected (see `socket_events`)

    We should be able to implement this as a push instead of a poll,
    unfortunately, it doesn't look like Linux offers such a NETLINK event
    yet (it looks like it is very close, though); our existing class works,
    but uses a timer


# Extensions

This library includes a few extensions which are not part of the main
library. A few extend other libraries (snaplogger) and a few are tools
that were found in our snapwebsites environment which can now be available
to anyone (i.e. are not as focused to only work with snapwebsites).

* `logrotate_udp_messenger` -- the library comes with this specialized class
which we use to force the logger to auto-re-open output files that were
rotated
* snaploggerd -- a service to send log messages over the network
* fluid-settings -- a service to manage settings through a service so all
the same settings can be used from any computer without having to duplicate
them


# License

The project is covered by the GPL 2.0 license.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/eventdispatcher/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
