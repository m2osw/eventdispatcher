
# Introduction

The Snap! Logger daemon is implemented in the eventdispatcher project
since this very project depends on the snaplogger project. It is also
a great example of how one can use the TCP and UDP objects available
in the eventdispatcher project.

# Snap! Logger

The Snap! Logger project is a tool used to manage logs. The base
implementation sends the logs to files, your console, and the syslog
facility.

The base implementation allows for additional appenders which can be
used to redirect the logs to yet other locations. This very project
can be used to send the data to a log server on another computer.

_**Note:** if you use syslog already, you can also setup syslog to share
the logs between multiple computers. This will not include all the
capabilities offered by our server, but you'll get logs from all
your services (not just Snap! based service)._

# Daemon Appender

The project comes with two sides:

* A new appender which receives the logs and sends them to the snaploggerd
  server via either TCP or UDP messages.
* A daemon which you can use to receive those TCP and UDP messages.

The following is about the appenders which you link in your projects.

_**Note:** The appender library has to be linked to your project to be
available to your users. At some point, we will port the libsnapwebsites
dynamic loader will be used for appenders so these can be added to a
directory and the base library can automatically pick them up from that
directory._

## Options

The appender supports the following options:

### Server IP & Port

The server IP address and port, where to send the log data, are defined
as follow:

    server_tcp=<ip-address>:<port>
    server_udp=<ip-address>:<port>

As you can see, the service supports both, the TCP and UDP protocols.
Since you can enter multiple definitions (with different names), it
is possible to send the log data to several servers.

    [serverA]
    type=tcp
    server_address=127.0.0.1:4043

    [serverB]
    type=udp
    server_address=10.0.3.11:4043

### Fallback

In case the UDP `send()` function fails, we can fallback to printing the
message in `stdout`. This is done by setting the following parameter to
`true`:

    fallback_to_console=true

### Acknowledge UDP Messages

This parameter defines whether the snaplogger appender should acknowledge
arrival of UDP packets it sends. The messages will include the necessary
information to send the acknowledgement. If you do not mind too much losing
some packets, then turning off the acknowledgement signals will ease the
load over your network.

    acknowledge=none|severity|all
    acknowledge_severity=<severity>

The value is an enumeration.

* none -- do not request any acknowledgement
* severity -- only request acknowledgement if message severity is equal or
  over `acknowledge_severity`
* all -- request acknowledgement for all messages

For example, to only acknowledge messages representing an `ERROR`, a
`CRITICAL` error, an `ALERT`, an `EMERGENCY`, or a `FATAL` error you
would use:

    acknowledge=severity
    acknowledge_severity=ERROR

Note that the `acknowledge_severity` is ignored if the `acknowledge`
parameter is not set to `severity`.


# Server

The actual server is a simple daemon which listen for log messages on a
TCP and a UDP port.

At this time there is no protection as the server is expected to be running
in a local network environment. A later version will implement an OAuth2
authentication system.

It is expected that the server directly saves the data to a file. But it
is a lot easier to have the server use the snaplogger library, so we do
that. You can change the settings to avoid the double timestamp and other
such potential issues.

    format=${message}

The server also offers additional variables such as the IP addess of the
computer that sent the message.

## Variable: `souce_ip`

The `source_ip` parameter is the IP address of the computer that sent a
given log message. It can be displayed using the following syntax:

    format=${message} from ${source_ip}

The port is not made available. It would not be useful.

## Variable: `protocol`

The `protocol` used to send the message, either `"tcp"` or `"udp"`.

## Variable: `serial`

Each message is given a serial number. This is an important part of the
protocol so a message can be acknowledge using its serial number.

The number is made available through the `serial` variable. It can be
useful if you want to sort messages in the order they were sent. The
serial number starts at 1 and increases by 1 with each message being
sent.


# License

The project is covered by the GPL 2.0 license.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/eventdispatcher/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
