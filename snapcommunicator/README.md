
# Introduction

The Snap! Communicator daemon is a way to send messages to any one of your
services on your entire network. What you need to do for this to work is:

1. Link against the communicator library
2. Register your services with the local Snap! Communicator
3. Send messages to the Snap! Communicator
4. List for messages from the Snap! Communicator
5. Tell each Snap! Communicator where the others are

Point (5) is only if you have more than one computer.

Points (1) to (4) are to make it works where all your service only need to
know about the Snap! Communicator. All the other services are auomatically
messaged through the communicator.


# Library

The project includes a library extension which allows you to connect to
the controller totally effortlessly.

    # Basic idea at the moment...
    class MyService
        : public sc::communicator("<name>")
    {
        sc::add_communicator_options(f_opts);
        ...
        if(!sc::process_communicator_options(f_opts, "/etc/snapwebsites"))
        {
            // handled failure
            ...
        }

        // from here on, the communicator is viewed as connected
        // internally, the communicator started a connection and it
        // will automatically REGISTER itself
    }

It also simplifies sending message as you don't have to know everything
about the eventdispatcher library to send messages with this library
extension.

    SNAP_COMMUNICATOR_MESSAGE("COMMAND")
        << sc::param("name", "value")
        << ...
        << sc::cache
        << sc::...
        << SNAP_COMMUNICATOR_SEND;

So, something similar to the snaplogger feature, but for messages in the
eventdispatcher (actually, this may be a feature that can live in the
eventdispatcher in which case we could use `SNAP_ED_MESSAGE` and
`SNAP_ED_SEND`, the TCP connection to the communicator would be the
default place where these messages would go).

Another idea, if we do not need a copy, would be to use a `message()`
function which returns a reference which we can use just like the `<<`
but the syntax would be more like:

    communicator.message("COMMAND").param("name", "value").cache();

Thinking about it, though, it's not really any simpler than:

    ed::message msg("COMMAND");
    msg.add_parameter("name", "value");
    connection.send_message(msg, true);


# Daemon

The Snap! Communicator is primarily a deamon which can graphically map your
network and the location of each of your services to allow for messages to
seamlessly travel between all the services.


# Tools

The projects comes with a few tools can are useful to send messages in
script (such as the logrotate script) and debug your systems by sniffing
the traffic going through the Snap! Communicator.


# License

The project is covered by the GPL 2.0 license.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/eventdispatcher/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
