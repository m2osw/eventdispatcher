
# Testing Services

In order to test services, it is best to have a unit test mechanism allowing
us to verify all the messages, especially when a chain of messages is expected
and that chain can break at any moment. For full coverage unit tests, we need
all the messages to be received by our services. It is very difficult to
maintain tests that start the communicator and other daemons in order to run
a service test. Instead, we want to have a simulator of all the other services
and only test the current service being developed and maintained. To do so we
use the **reporter** test class.

# Basics of Reporter Language

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

To allow for out of sequence execution, we support labels and goto
instructions. Both use the same syntax as above:

    label(name: <label-name>)
    goto(label: <label-name>)

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

# Instructions

## Label

Define a label you can go to.

    label(name: <label-name>)

## Goto

Go to a label unconditionally.

    goto(label: <label-name>)

## If

On a condition, go to one label (true) or another (false).

The `if()` uses the result of the last `compare_...()`, `has_...()` and
other similar instructions to decide where to go.

    if(true: <label-name>, false: <label-name>)

Only one of `true:` or `false:` is required. If both are specified, then
the code after the `if()` will not be reached unless it is a `label()`.

## Sleep

Sleep for the specified amount of time.

    sleep(seconds: <double>)

## Wait

Wait for a message to arrive on our connection.

    wait(timeout: <double>)

The `timeout` parameter is how much time we can wait before failing.

## Run

It may be useful to execute some instructions before starting the thread
runner. This instruction is here to switch between the main test thread
to this reporter thread runner.

    run()

## Listen

Prepare a connection used to listen for messages coming from your service.
The `listen` can be used to specify the type of connection to create (TCP,
UDP, Unix, etc.) through the use of the scheme in the URI.

    listen(uri: <uri>)

## Done

Indicate that the reporter does not expect to receive any more messages
from the client. If it does receive anything from the client after this
instruction, the test fails.

    done(timeout: <double>)

This `timeout` parameter is the same as the one on the `wait()` instruction.
It uses exactly the same polling mechanism with that timeout. The `done()`
expects the timeout to happen, however. If it does not happen, we detected
an error.

## Error

Immediately exit the reporter with the specified error message.

    error(message: <message>)

## Verify Message

When the `wait()` command returns, it received a message. This instruction
checks that last message parameters against the `verify_message()` parameters.

    verify_message(
        sent_server: <name>,
        sent_service: <name>,
        server: <name>,
        service: <name>,
        command: <name>,
        required_parameters: <name>=<value> ...,
        optional_parameters: <name>=<value> ...,
        forbidden_parameters: <name> ...)

If the verification fails, then an error occurs and the test failed.

Note that as far as the reporter language is concerned, the
`required_parameters`, `optional_parameters`, and `forbidden_parameters`
variables each have one single value. The `verify_message()` further parses
that value to distinguish each parameter.

The `forbidden_parameters` is generally not necessary. This is only useful
if that specific message cannot use a globally defined parameter.

Except for global parametrs, all the parameters supported by a message must
be included in the list of `required_parameters` or `optional_parameters`.
A parameter can only be defined once (for example, it cannot appear in the
`required_parameters` and the `optional_parameters`).

## Send Message

Once we received a message, we are likely to need to send a reply. This
instruction is used for that purpose.

    send_message(
        sent_server: <name>,
        sent_service: <name>,
        server: <name>,
        service: <name>,
        command: <name>,
        parameters: <name>=<value> ...)

Note that as far as the reporter language is concerned, the `parameters`
variable has one single value. The `send_message()` further parses that
value to distinguish each parameter.

## Compare Message Command

Check whether we received a certain message command.

    compare_message_command(command: <name>)

This is most often followed by an `if(false: <label-name>)` instruction.

## Has Parameter

Check whether we the message includes this specific parameter.

    has_parameter(name: <parameter-name>)

This is most often followed by an `if(false: <label-name>)` instruction.

## Has Parameter with Value

Check whether the message has the specified parameter and that parameter
is set to the specified value. If both of these are true then the function
sets the current result to true.

    has_parameter_with_value(name: <parameter-name>, value: <value>)

This is most often followed by an `if(false: <label-name>)` instruction.

## Set Variable

Set a variable to a specified value.

    set_variable(name: <variable-name>, value: <value>)

## Save Parameter Value

Save the value of the specified parameter to the named variable.

    save_parameter_value(parameter: <parameter-name>, name: <variable-name>)

## Call

At times, it is useful to create a function like section of code to avoid
repeating code over and over again. i.e. in some cases, you may receive
certain repetitive messages at any time and these can be processed in a
single section.

    call(label: <label-name>)

Note: it is suggested that the labels used for calls start with the letters
"func_" to represent the fact that these are functions.

## Return

Once done in a function, you can return. At the moment you cannot return
a value. The instruction just continues just after the last `call()`.

    return()

# Implementation

The implementation is pretty straight forward. The reporter reads a script
from your `tests/...` directory and parses it in tokens. It memorize all
the `label()` with their location so it can quickly execute a `goto()`,
`if()`, `call()`, etc.

Other projectss can also provide additional instructions by creating
their own instruction factories.


# vim: ts=4 sw=4 et
