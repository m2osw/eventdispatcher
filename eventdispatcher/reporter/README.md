
# Testing Services

In order to test services using the eventdispatcher library, it is best to
have unit tests allowing verifying all the messages the service makes use
of, especially when a chain of messages is expected and that chain can break
at any moment (see cluck as a prime example).

For full coverage of the service code, it is necessary for all the messages
to be sent and received by the service being tested. It would be very
difficult to maintain full integration tests that start the communicator
and any other daemon in order to run a full coverage test.

Instead, we want to have a simulator for all the other services and only
test one service, the one being developed and maintained. To do so we
use the **reporter** test classes.

# Basics of Reporter Language (.rprtr files)

The reporter class creates a thread and runs code in parallel. This allows
our service to really connect to a TCP or UDP port through the network or
a Unix socket.

In order to make it easier, the reporter supports a very basic language.
The language is a set of instructions with parameters. One instruction
ends with a closing parenthesis. Since these look like function calls,
the syntax looks as follow:

    <instruction> '(' <parameter name> ':' <value> ',' ... ')'

There is no default parameter order, so all parameters to a function must
have a parameter name. The value can be quoted (" or '). It must be quoted
if it includes a quote, a comma, or a parenthesis.

To allow for out of sequence execution, we support the `label()`, `goto()`,
and `if()` instructions. All use the same syntax as above:

    label(name: <label-name>)
    goto(label: <label-name>)
    if(true: <label-name>, false: <label-name>)


# Variable Support

The instructions support variables in their value. Those are written with
a starting `$` sign and a name. You can write the name between curly braces,
which is useful if the variable is followed by other characters.

You can set a variable using the `set_variable()` instruciton. It is also
possible for the tester to define variables before calling the
`reporter.start()` function.

Variable are also transformed when found in double quoted (") strings.
Single quoted (') strings ignore the `$` character.

# Global Parameters

Message support certain global parameters such as the `version=...` parameter.
These are defined by the reporter object. If you extend the reporter, you
can add your own global parameters (i.e. the communicatord adds the `cache=...`
parameter).

# Comments

The language supports C++ like comments introduced by "//" and ending with
the next newline.

# Instructions

* `call()`
* `clear_data()`
* `clear_message()`
* `error()`
* `exit()`
* `goto()`
* `has_data()`
* `has_message()`
* `has_type()`
* `message_has_parameter()`
* `message_has_parameter_with_value()`
* `if()`
* `label()`
* `listen()`
* `print()`
* `return()`
* `run()`
* `save_parameter_value()`
* `send_data()`
* `send_message()`
* `set_variable()`
* `show_data()`
* `show_message()`
* `sleep()`
* `unset_variable()`
* `verify_data()`
* `verify_message()`
* `wait()`

## Call

At times, it is useful to create a function like section of code to avoid
repeating code over and over again. i.e. in some cases, you may receive
certain repetitive messages at any time and these can be processed in a
single section.

    call(label: <label-name>)

**Note:** it is suggested that the labels used for calls start with the
          letters "func_" to represent the fact that these are functions.

## Clear Data

Once in a while, you may run a test that leaks some binary data. This
call will reset the pipe so you can start testing the next step without
having to run a brand new script. We do not expect this will work in all
cases.

    clear_data()

## Clear Message

Once done with a message, you may want to clear it before using the
`wait()` instruction again. This gives you the ability to use the
`has_message()` again as expected.

    clear_message()

## Disconnect

The `listen()` instruction creates a socket to listen for incoming
connections. The `disconnect()` can be used to remove that connection.

    disconnect()

    TODO: support the name parameter list in the listen()

After a `disconnect()` you can still use the `wait()` instruction as long
as there is at least one client connected or the `drain` mode is used.

**Note:** at the moment you cannot disconnect clients.

## Exit

Exit the reporter. The `exit()` command can be used in three different
modes:

* Success

  In this case, use the `exit()` command with no parameters:

      exit()

  It will simply return without error and consider that the reporter script
  was successful.

* Failure

  If you detect a failure (i.e. received the wrong message), then you can
  immediately stop the script using this failure mode. In this case, you
  pass an error message to the command:

      exit(error_message: "<the error description>")

  This makes the process exit immediately.

  This instruction can be used as a _not reached_ statement. For example,
  if you have an `if()` which is expected to always branch, you can place
  an `exit()` with an error message just after it:

      if(ordered: go_here, unordered: go_there)
      exit(error_message: "logic error: statement should not be reached")

* Timeout

  Whenever you use the `run()` command, you may then end the transmission
  and clearly not expect any additional messages from the other end. To
  make sure that works as expected, you can wait a second or two before
  exiting. If while waiting a message is received from the other end, then
  it is viewed as an error and the script fails.

      exit(timeout: "2s")

  The `timeout` parameter accepts a duration as defined in the advgetopt
  duration validator. It is used the same way as in the `wait()`
  instruction.

  **Note:** A `listen()` must have occurred in order for this feature to
            be used.

Note that the `error_message` and `timeout` parameters are mutually
exclusive. If both are specified, you get a script error.

## Goto

Go to a label unconditionally.

    goto(label: <label-name>)

## If

On a condition, go to the corresponding label.

The `if()` uses the result of the last `compare_...()`, `<object>_has_...()`
and other similar instructions to decide where to go.

    if(variable: <name>, <operator>: <label-name>)

The `variable` parameter is optional. It can be used to test using the value
of an integer variable as the input instead of the last compare value. If
the variable is undefined, then the operator `unordered` will be a match.
If the variable is not an integer, the script fails. If it is an integer,
0 is transformed in `equal`, negative numbers in `less` and positive
numbers in `greater`. Note: to test whether a variable is defined, use
the `has_type()` instruction and then `if([un]ordered: ...)`.

The `<operator>` is one of:

* `less` -- go to `<label-name>` if less (compare result is -1)
* `less_or_equal` -- go to `<label-name>` if less (compare result is -1 or 0)
* `greater` -- go to `<label-name>` if greater (compare result is 1)
* `greater_or_equal` -- go to `<label-name>` if greater (compare result is 0 or 1)
* `equal` or `false` -- go to `<label-name>` if equal (compare result is 0)
* `not_equal` or `true` -- go to `<label-name>` if not equal (compare result is not 0)
* `ordered` -- go to `<label-name>` if ordered (compare result is not -2)
* `unordered` -- go to `<label-name>` if unordered (compare result is -2)

Multiple `<operator>` can be used within a single `if()`. They each must be
distinct and not overlapped (i.e. `less` and `equal` can be used together,
`greater_or_equal` and `not_equal` overlap since "greater" also represents
"not equal").

The `false` and `true` labels can be used for functions such as the
`has_message()` function. This makes it easier to read than using the
corresponding `equal` or `not_equal` labels.

If the compare value is still undefined and not variable name was specified,
then the function fails.

## Has Data

Check whether data was received on a TCP connection.

    has_data(min_size: <size>)

The `min_size` parameter is optional. If the number of bytes is less than
`<size>`, then the function returns false.

You are expected to use the `if()` instruction as follow:

    if(false: not_enough_data, true: got_the_data)

This command works in a way similar to the `has_message()` command.

## Has Message

Check whether a message was received.

    has_message(command: <name>)

This sets the condition to either `false` (no message) or `true`
(a message is present) if the `command` parameter is omitted. In other
words, you may use the `if()` instruction like so after this instruction:

    if(false: still_no_message, true: got_a_message)

If the `command` parameter is used, then `true` is returned only if there
is a message and it has that specific command. The `<name>` is likely all
uppercase words separated by underscores.

Note that once a message was received, it sticks to the state until cleared.
To forget about the last message received, make sure to use the
`clear_message()` instruction before the next `wait()`.

Here is an example showing how one can use the `has_message()` instruction:

    listen()
    run()
    label(name: wait_message)
    clear_message()
    wait(timeout: 10.0)
    has_message()
    if(false: wait_message)
    has_message(command: REGISTER)
    if(false: other_message)
    verify_message(command: REGISTER, ...)
    send_message(command: READY)
    goto(label: wait_message)
    label(name: other_message)
    ...

## Has Type

Check the type of a variable and set the compare value to `unordered`,
`true`, or `false`.

    has_type(name: <variable-name>, type: <type-name>)

`<type-name>` is expected to be an identifier. The currently supported
types are:

* `address`
* `floating_point`
* `integer`
* `list`
* `string`
* `timestamp`
* `void`

You can use this instruction to determine whether a variable is set or
not like so:

    has_type(name: <variable-name>, type: void)
    if(unordered: variable_is_not_set, ordered: variable_is_defined)

## Label

Define a label you can go to.

    label(name: <label-name>)

## Listen

Prepare a connection used to listen for messages coming from your service.
The `listen` creates the connection with the specified address. You can
change the type of connection with the `connection_type()` instruction.
The default is TCP.

    listen(address: <address>)

    TODO: add support for certificate: ... & key: ...
    TODO: add support for a type (TCP, UDP, Pipe, Unix...)
    TODO: add support for a name so multiple listen()-ing connections can be
          created

A `listen()` must appear before the first `wait()` call. You can use the
`disconnect()` function to stop listening for new connections.

## Message Has Parameter

Check whether we the message includes this specific parameter.

    message_has_parameter(name: <parameter-name>)

This is most often followed by an `if(false: <label-name>)` instruction.

## Message Has Parameter with Value

Check whether the message has the specified parameter and that parameter
is set to the specified value. If both of these are true then the function
sets the current result to true.

    message_has_parameter_with_value(name: <parameter-name>, value: <value>)

This is most often followed by an `if(false: <label-name>)` instruction.

## Print

Print message on the screen.

    print(message: <string>)

## Return

Once done in a function, you can return. At the moment you cannot return
a value. The instruction just continues just after the last `call()`.

    return()

## Run

It may be useful to execute some instructions before starting the thread
runner. This instruction is here to switch between the main test thread
to this reporter thread runner.

    run()

## Save Parameter Value

Save the value of the specified parameter to the named variable.

    save_parameter_value(parameter: <parameter-name>, name: <variable-name>)

## Set Variable

Set a variable to a specified value.

    set_variable(name: <variable-name>, value: <value>)

## Send Data

Once we received binary data, we are likely to need to send a reply. This
instruction is used for that purpose.

    send_data(
        values: [ <expr>, <expr>, ... ] )

The `values` parameter is mandatory.

**Note:** Until a client connects, the `send_data()` is not going to work.

## Send Message

Once we received a message, we are likely to need to send a reply. This
instruction is used for that purpose.

    send_message(
        sent_server: <name>,
        sent_service: <name>,
        server: <name>,
        service: <name>,
        command: <name>,
        parameters: { <name>: <value>, ... } )

The `command` parameter is mandatory.

**Note:** Until a client connects, the `send_message()` is not going to work.

## Show Data

Once binary data was received, you generally want to verify it with
`verify_data()`. At times, though, the `verify_data()` may be
difficult to write if you do not know all the binary data expected.
This is where this instruction becomes handy. It allows you to write
the data in your console and thus see the complete list of bytes it
includes and write your test accordingly.

    show_data(size: <number of bytes>)

## Show Message

Once a message was received, you generally want to verify it with
`verify_message()`. At times, though, the `verify_message()` may be
difficult to write if you do not know all the exact parameters it
is expected to receive. This is where this instruction becomes handy.
It allows you to write the message in your console and thus see the
complete list of parameters it includes and write your test accordingly.

    show_message()

## Sleep

Sleep for the specified amount of time.

    sleep(seconds: <double>)

## Verify Data

When the `wait()` command returns, it received data. This instruction
checks the data against the `verify_data()` parameters.

    verify_data(
        values: [ <expr>, <expr>, ... ] )

If the verification fails, then an error occurs and the test failed.

## Verify Message

When the `wait()` command returns, it received a message. This instruction
checks that last message parameters against the `verify_message()` parameters.

    verify_message(
        sent_server: <name>,
        sent_service: <name>,
        server: <name>,
        service: <name>,
        command: <name>,
        required_parameters: { <name>: <value>, ... },
        optional_parameters: { <name>: <value>, ... },
        forbidden_parameters: { <name>, ... } )

If the verification fails, then an error occurs and the test failed.

Note that as far as the reporter language is concerned, the
`required_parameters`, `optional_parameters`, and `forbidden_parameters`
variables each have one single value. The `verify_message()` further parses
that value to distinguish each parameter.

The `forbidden_parameters` is generally not necessary. This is only useful
if that specific message cannot use a globally defined parameter.

Except for global parameters, all the parameters supported by a message must
be included in the list of `required_parameters` or `optional_parameters`.
A parameter can only be defined once (for example, it cannot appear in the
`required_parameters` and the `optional_parameters`).

## Wait

Wait for a message to arrive on our connection.

    wait(
        timeout: <double>,
        mode: wait | drain)

The `timeout` parameter is how much time we can wait before failing. It
is mandatory.

The `mode` defines how to wait. At this point we have the default, which
is `wait` and the special `drain` mode which allows for draining the
last `send_message()` and not accept new connections or messages.

In `wait` mode, there must be at least one connection available. This
means the `listen()` must have been called and possibly a client
connected.

**WARNING:** The `wait()` instruction waits and processes one single event.
This could be a connection from a client, a message (or part of a large
message), a signal, etc. It is likely that you will have to loop until
a specific event occurs (i.e. receive the `STOP` or `DISCONNECT` message).

# Implementation

The implementation is pretty straight forward. The reporter reads a script
from your `tests/...` directory and parses it in tokens. It memorize all
the `label()` with their location so it can quickly execute a `goto()`,
`if()`, `call()`, etc.

Other projects can also provide additional instructions by creating
their own instruction factories.


# vim: ts=4 sw=4 et
