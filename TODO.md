
* Full IPv6 Support

  We need to make sure that all our classes support IPv6 as expected.
  It should already be in place, it's a matter of testing to make sure
  it works 100% as expected.

* A permanent connection makes use of the connection timer

  I have had many problems with that one, I am actually thinking that
  maybe having a timer in each connection is not a good idea at all
  (i.e. that functionality should probably be 100% separate).

  Now that we have the ability to create a standalone timer and setup
  a callback using std::bind(), we should switch the permanent connection
  timer to an internal timer which doesn't make use of the permanent
  connection timer itself so the users of that connection could now
  make use of said timer (although from the statement above, we probably
  should 100% stop such uses).

* Over time, update the address array of a permanent connection

  The `tcp_client_permanent_message_connection` class supports any number
  of addresses to attempt to connect to. The problem here is that most
  often a DNS will return a set of addresses and not too long later,
  a different set of IPs (i.e. if you deploy on a Google Cloud system
  with kubernetes, your pods are assigned new IPs each time you do that;
  this is because the old IP is kept while the old instance keeps running
  until the new instance is available and in the DNS, then the old instance
  gets removed, so the new pod cannot be using the same IP as the old pod).

  So we need a way to update those IP addresses after the TTL given to
  us by the DNS, which says the old IP addresses are not valid anymore.

  **Note:** these addresses may also change with time. So if you requests
  a domain to give you an IP address at some point, it may be necessary
  to request for new IPs when you attempt to reconnect much later. This
  is also not currently supported as input domain names are transformed to
  IP addresses and they stay that way _forever_ (until you restart or
  as a programmer, until you create a new object with that domain name).

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

* Replace the time/date with a `timespec_ex` and use `ppoll()` which matches
  one to one (instead of our manually handled `timeout` variable).

* Add support for UDP, Unix stream, Unix UDP, etc. to the reporter language.
  (i.e. the newer version of communicatord uses the Unix stream for all
  local services so it would be best to be able to test that type of
  connection in the tests to verify the class properly)

* Track which message was ever sent & received (i.e. a form of
  message coverage) so we can verify that all messages were checked
  (i.e. for debug purposes).

* Verify the message config definitions with the verify-message-definitions
  tool at compile time so we are sure that we install valid files and don't
  break an entire system by installing invalid files (i.e. we throw when we
  find errors).

  Services with messages:
  - cluck **[DONE]**
  - communicatord **[DONE]**
  - eventdispatcher **[DONE]**
    + snaploggerserver **[DONE]**
  - fastjournal
  - fluid-settings **[DONE]**
  - iplock
    + ipwall
  - prinbee
  - snaprfs

* The message definitions currently verify that some parameters are present
  (when marked as required); it can also verify the type of the data if
  defined; however, it does not check whether a parameter is not defined
  (i.e. if a parameter is defined in a message and not in its definition,
  it is always accepted); there is a glitch here because the communicator
  daemon tends to add all sorts of other parameters to manage its cache
  and broadcasting and error reporting... those need to be acceptable
  parameters no matter what; so it is a bit more complicated than what
  we have now, but certainly doable. Maybe have a separate set of .conf
  files that define extra parameters that can happen on any message

* The inotify listens to either a specific file or a directory. It does not
  listen for events in sub-directories. We want to add a RECURSIVE flag to
  support that feature. This means whenever we add a new watch, we also need
  to emit events for files found in those sub-directories at the time we add
  the new watch (in which case we may want another feature which is to send
  an event for all existing files after starting a new watch, with a type such
  as EXISTS).

* Local sockets represented by files can use permissions to control the
  security (under Linux). Right now, I use 0666 in the communicatord in
  order to let people connect to the service. I would like to instead
  make use of a specific group (i.e. something like "communicator-user")
  and make sure that users of the communicatord are part of that group.
  Here we would need to support changing the group of the socket and
  then set the permissions accordingly (i.e. 0660 instead of 0666).

* A service to track changes to the list of opened/closed network connections

  My existing code does not work properly, we can determine what is opened
  and closed, but we do not get events when the state changes... I got a
  message about it on Stackoverflow and a link to here:

      https://github.com/sivasankariit/iproute2/blob/master/misc/ss.c
      https://stackoverflow.com/questions/68425240/can-the-netlink-sock-diag-interface-be-used-to-listen-for-listen-and-close?noredirect=1#comment132999023_68425240

  which may be useful to fix the existing code and make it work.

  One way may be to listen for file changes under /proc/... as it may have the
  effect we want (i.e. whenever a new file is created there, such as a UDP
  connection, then we would receive a message and can tranform that in a
  signal about new/removed TCP/UDP/... connections (however
  `file_change`--a.k.a. inotify does not work on `/proc`).

* Finish the snaplogger network appender extension (mainly testing now).

* Consider moving the cppprocess `tee_pipe` to eventdispatcher.

* Update all the `\file documentation` to match the corresponding class.
  These were copied/pasted in all the files when I did the big break up
  and most are still not updated.

* Update the dispatcher classes documentation to match the new scheme (the
  callback instead of a function offset in one class).

* Consider using the `SO_LINGER` to not wait on a `close(socket)` (i.e. turn
  off the lingering). This means the kernel closes the socket in the
  background and we can move on with other work.

* Rate limit transfers by sending X bytes every N milliseconds on that given
  connection (i.e. as long as there is something to write and it can be written
  to that file descriptor, sleep N milliseconds and then send another X bytes).

  As our `poll()` loop stands, this is not quite feasible. But we can look at
  testing whether the connection has data to be written, then also check on
  the "rate limited" feature, if turned on (N is not 0?), then check whether
  we should wait on the fd to become available (i.e. time to send more data)
  or view the connection as a timer (i.e. we still need to wait some more).
  The current use of the timer right now is a form of Inclusive OR, this other
  method would make it a form of Exclusive OR. But we probably want to use a
  different timestamp tracker which get reassigned a "now + N" whenever a
  write happened. But for the limit to work properly, we need to know the
  amount of data sent and whether we can send more and how much more...

* Short `connect()` timeout is possible by making the socket non-blocking
  before calling the function; then we can `poll()` on the socket and get
  a POLLOUT event once the socket is connected; this can be useful in some
  situations, many times it's probably not that necessary, except that has
  been a bottleneck in the permanent TCP connection implementation and is
  why we use a thread for the connect to happen in parallel... that would
  not be required anymore (but it adds a new state "in-limbo while
  connecting").

  From the `connect()` man page:

       EINPROGRESS
              The socket is nonblocking and the connection cannot be
              completed immediately. It is possible to select(2) or
              poll(2) for completion by selecting the socket for writing.
              After select(2) indicates writability, use getsockopt(2) to
              read the SO_ERROR option at level SOL_SOCKET to determine
              whether connect() completed successfully (SO_ERROR is zero)
              or unsuccessfully (SO_ERROR is one of the usual error codes
              listed here, explaining the reason for the failure).

  **IMPORTANT:** when `connect()` returns, it may already have connected and
                 return 0. You need to enter the intermediate state only if
                 `connect()` returns `EINPROGRESS`.

  See: https://stackoverflow.com/questions/2597608/c-socket-connection-timeout

* Add support for any number of timers in a connection. I often run in
  problems with this because I need two or three different timers then
  I have to create sub-objects, which are separate connection timers by
  themselves. We can have an identifier to recognize which timer times
  out and pass that parameter to the `process_timeout()` function.
  Another solution would be to have a way to quickly create a timer
  without having to create a sub-class, so that way we could keep it
  separate (clean) and have a callback instead of a virtual function
  (and we have a callback thingy to manage lists of callbacks in snapdev).
  This is what version 2 would be about!

* Consider removing the stop() and ready() functions because it would be
  as easy to use a CALLBACK which means anyone receives a call whenever
  those messages arrive without the lower level systems having to do
  anything special about it. This may be a good tweak when we build the
  whole thing using templates (next entry). In that case, it's clearly
  just callbacks instead of reimplementation of a virtual function.

* Ideas on how to re-implement the whole thing using templates to avoid
  duplicating the buffering & message handling in each class (instead pass
  a "trait" or something of the sort) a.k.a. version 2.

  See a compiling version in `connection_t.h/cpp`. This would allow me to
  have one class create "many" connections and handle all the events as it
  sees fit instead of having to derive from each one of these "many"
  connections and reimplement the callbacks in each one of them.

  current design:

      connection
        ^
        +-- tcp_client_connection
              ^
              +-- tcp_client_buffer_connection
                    ^
                    +-- tcp_client_message_connection
                          ^
                          +-- tcp_client_permanent_message_connection

  and I repeat that for local_stream, udp, pipe, etc. when the only thing
  we really need is a connection implementation, then everything else can
  be done using a trait

  the basic connection remains pretty much the same, these are viewed as
  connection traits:

      base_connection
        ^
        +-- tcp_client_connection
        |
        +-- udp_client_connection
        |
        +-- ...

  the rest becomes template classes where ConnectionT is one of the basic
  connections defined above:

      template<
            typename BaseConnectionT
          , typename EventHandlerT>
      class connection
          : public BaseConnectionT
      {
          connection(EventHandlerT * event_handler)
              : f_event_handler(event_handler)
          {
          }

          virtual void process_timer() override
          {
              if constexpr (std::is_member_function_pointer_v<decltype(&EventHandlerT::process_timer)>)
              {
                  f_event_handler.process_timer();
              }
              else
              {
                  SNAP_LOG_WARNING
                      << "connection \""
                      << get_name()
                      << "\" received a process_timer() event without a corresponding event handler."
                      << SNAP_LOG_SEND;
              }
          }

          ...repeat for each possible event in BaseConnectionT...
      };

      template<
            typename BaseConnectionT
          , typename EventHandlerT>
      class buffering
          : public connection<BaseConnectionT, EventHandlerT>
      {
      };

      template<
            typename BaseConnectionT
          , typename EventHandlerT>
      class messenger
          : public buffering<BaseConnectionT, EventHandlerT>
      {
      };

  The traits are plain classes and we can test whether they have a given
  function to know whether we can call it or not.

    using a template we can have "traits" that allow us to have implementations
    of the connection, read, write, buffering, etc. so for messages we can have
    one implementation instead of one per connection type

    the result is a way to create connections with a set of traits more or
    less like so:

      template<
          class ConnectionT,    // Timer, TCP, UDP, Unix, Pipe, SignalFD, ...
          class Traits,         // read/write, buffering, message
          class Allocator = std::allocator<CharT>>
      class connection
      {
      };


vim: ts=4 sw=4 et
