
# Client / Server

The client.cpp and server.cpp are examples of client and server (daemon,
service) using the event dispatcher library.

The important part to notice here is that the whole system is built on
messages after initialization (up to the `communicator::run()` call)
and until the destruction (which in this case is relatively small,
see the `my_daemon::quit()` and `my_client::quit()` functions and
respective callers.)

## Initialization

The initialization process creates the necessary objects and add the
connections to the communicator. Once that is done, we simply call
the `run()` function which takes over and assuming no messages was
received yet, puts the process to sleep (it uses no CPU).

### Client

The client initializes a log rotate UDP connection, if a client runs
for a long time (this one does if there is no server, it will try
to connect again and again until the server starts), then the log
may get rotated under its feet. This connection allows the logrotate
service to send us a `LOG` message and as a side effect requests
the snaplogger to close the current output file and open a new one
(that is, if such a snaplogger appender is running).

Next the client initializes a permanent client connection. This
type of connection never really dies (well, when it can't connect
to the server, your client sleeps for a small amount of time and
tries again). This type of connection allows you to not worry about
whether the server is up and running at the time you start your
client. **Why is that important?** Because the service to which
your client is connected my get upgraded. When that happens, the
service is stopped, upgraded/configured, and finally restarted.
It is extremely practical to have your service auto-reconnect
in such a situation.

Finally, the client calls the `communicator::run()` function.
This is where all the magic happens. It can wait on a timer
(which is what happens with the permanent client connection)
and listen for events to happen on sockets. (**Note:** The
communicator is capable of listening on way more things than
just network socket.)

Once connected to the server, we expect to receive a `HI` message.
To that we send a reply: `DAD`, to which the server replies with
a `CLIMB`, so the client tells it to go to the `TOP`, the server
expected more info, requests a name with a `WHO` message, the
name `MOM` is sent back and the server is not happy and sends us
a `BYE`. At that point the client decides to `quit()`.

     Client          Server

    connect    ->    connected

               <-    HI

    DAD        ->

               <-    CLIMB

    TOP        ->

               <-    WHO

    MOM        ->

               <-    BYE

    quit

## Server

The server is initialized in a similar manner, only it creates
a listener instead of a permanent client. The listener will
do an `accept()` and wait for client to connect.

When `accept()` is triggered, we have a new pending connection.
We retrieve it and create a new object which is also called a
client. Only in this case it is a "server-client". The main
difference is that client doesn't try to connect to anything.
Instead, it receives a pointer to a BIO object with the new
client connection.

That connection is the other side that communicate with the
Client process we just mentioned. The number of such
connections is not limited by the library (your daemon can,
of course, impose various limits; we have daemons that accept
only one connection at a time).

As shown above the server understands some messages (DAD, TOP,
MOM) which the client can send.

To end the server, you can also send the `QUIT` message. This one
removes all the connection s from the communicator which then
has nothing to do in its `run()` function and thus it returns.

## Running the Client / Server pair

You can either start the client or the server first. Either
way works because both sides are programmed in a resilient
way.

That being said, the first time, it probably makes more sense
to start the server first:

    examples/server --trace

This starts the server and prints some information about it
such as its version.

Next you can run the client once or more:

    examples/client --trace

This does a clean run of the client as shown above.

As mentioned, you can start as many clients as you'd like.
The server will take it as long as your network and computer
are powerful enough. The Linux kernel, though, will stop
accepting connections when receiving around 128 connections
which are still unanswered.

When the server is running, the client will end automatically
at the time the server sends the `BYE` message.

If the server is not running, the client sits there until the
server starts. At that point, the client receives the `HI`
message and quickly ends up receiving the `BYE` message.

**Note:** As implemented, the client is not that resilient in
case the sequence of messages stops midway.

To stop the server, you can send it the `QUIT` message. This
will start the necessary cleanup process. To so so, you can
use the `ed-signal` tool like so:

    ed-signal --trace --server 127.0.0.1:3001 --type tcp --message QUIT

The `--trace` in all of these calls is a snaplogger command
line option which switched the severity to `TRACE` which
allows you to see a log entry in your console for each message
received by a process. This happens because of this call:

    #ifdef _DEBUG
        f_dispatcher->set_trace();
    #endif



# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/eventdispatcher/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
