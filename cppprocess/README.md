
# Introduction

This sub-library is used to handle processes.

At this time, it includes two main parts:

1. the `process` class to run one or more processes (piped between each other);
2. the `process_info` class to read process information from `/proc`.

# Process

The `process` class is used to run a command (`fork()` + `execve()`). The
class is capable of handling the input and output/error streams via pipes
allowing you to send data to the command and receive the output.

You can also pipe multiple processes one after the other using the
`add_next_process()`. By default, this pipes each process to the next
process. You can also add more than one next process in which case
the list of processes all receive a copy of the output of the previous
process.

## Example

To use the `procss` class you can use your own pipes, in which case you
are responsible to reading and writing data from/to the process. If you
are not otherwise using the `ed::communicator`, you may also want to use
the `process::wait()` command. In that case, you can let the `process`
object create the necessary pipes internally.

    // create a process object
    //
    cppprocess::process p("my-command");

    // only set the command, optionally with it's full path
    //
    p.set_command("echo");
    // -- or --
    p.set_command("/bin/echo");

    // add argument, these have to be added in the correct order (as expected
    // by the command you are trying to run)
    //
    // here, arguments are already separated (i.e. no shell will intervene)
    // so you do not need anything to handle special characters (no quotes,
    // no backslash)
    //
    p.add_argument("-n");
    p.add_argument("Hello World!");

    // to add filenames with a pattern, use the second parameter; this will
    // expand the pattern in a way similar to `/bin/sh`
    //
    p.add_argument("cppprocess/cppprocess/*.cpp", true);

    // in a similar fashion, you can add/change environment variables
    //
    p.add_environ("HOME", "/var/www");

    // to prevent your environment variable from leaking in the child
    // process, you can turn off the default; in this case, your child
    // process will be in a similar state as a CRON process (i.e. nearly
    // no default environment)
    //
    // see also the `env -i ...` command line (`man env`)
    //
    p.set_forced_environment();

    // you can add input using the add_input() if you do not want to
    // handle a pipe yourself; this will create an internal input pipe
    //
    // the input can be a string or binary data
    //
    // otherwise, you can define your own pipe
    //
    // if you do not add any input and do not supply your own pipe,
    // your stdin file will be passed on to the child process
    //
    p.add_input("Input here");
    // -- or --
    p.set_input_pipe(my_input_pipe);

    // by default the output is sent to stdout, which is good if you
    // do not need it
    //
    // if instead you want to capture the output, you can either supply
    // your own pipe_connection object or use the internal object with;
    // if you use the auto-capture mode, the whole output will be
    // available only once the command exited although you can try
    // to get partial bits as it comes in
    //
    // the get_output() will not work if you used your own pipe since
    // in that case the data is directly sent to your pipe object
    //
    p.set_capture_output();
    // and then later: p.get_output();
    // -- or --
    p.set_output_pipe(my_output_pipe);

    // simlar to stdout, you can capture stderr
    //
    p.set_capture_error();
    // and then later: p.get_error();
    // -- or --
    p.set_error_pipe(my_error_pipe);

    // now that we are ready, we can start the process, it will run in
    // parallel with ed::communicator events; to know that it ends, though,
    // you want to setup a listener in the signal_child singleton; if you
    // are not using the ed::communicator, use the wait() instead
    //
    p.start();
    // if no ed::communicator: p.wait()

    // with ed::communicator, you get a call to your listener (callback)
    // that you add to the signal_child object like so
    //
    // WARNING: this has to be done after the start() (i.e. you need the
    //          pid_t of the child) and before you return back to the
    //          ed::communitor::run() loop
    //
    ed::signal_child::pointer_t child_signal(ed::signal_child::get_instance());
    child_signal->add_listener(f_child, std::bind(&my_class::process_done, this, std::placeholders::_1));
    ed::communicator::instance()->add_connection(child_signal);

    // to prematurely stop the process, send a signal with the kill()
    // function
    //
    p.kill(SIGTERM);

**WARNING:** The pipes added to a process will have one side closed at
the time the `process::start()` function gets called. The other side must
be closed as soon as your done with it (especially the input since this
is how the child process gets an EOF on that stream). In other words,
a pipe can be used once. Each instance of a process, new or not, must
be given a new pipe. It is strongly advised that you use a new `process`
object each time you run a command.

## Simulate a system() call

Assuming you do not need any input or output, just to run a command, you
can use the following:

    cppprocess::process p("my-command");
    p.set_command("mkdir");
    p.add_argument("-p");
    p.add_argument("tmp");
    p.start();
    p.wait();       // if you don't use ed::communicator

One advantage here is that the arguments do not need to be escaped in any way.

## Simulate a popen("cmd", "r") call

To simulate a `popen()` call with the `"r"` option, you want to capture the
output. If you just want to run one command and get the output as a string
all at once, then you can just use the capture flag like so:

    cppprocess::process p("my-command");
    p.set_command("cat");
    p.add_argument("/proc/cpuinfo");
    p.set_capture_output();
    p.start();
    p.wait();       // if you don't use ed::communicator
    std::string output(p.get_output());

You can also get the trimmed output, which removes extra spaces and new line
characters before returning the output string. If the output is binary data,
then use the `get_binary_output()` to get the `buffer_t` as is.

## Simulate a popen("cmd", "w") call

To simulate a `popen()` call with the `"w"` option, you want to add the
input to the process before you call the `start()` function.

    cppprocess::process p("my-command");
    p.set_command("logger");
    p.add_input("Message to send to syslog");
    p.start();
    p.wait();       // if you don't use ed::communicator

Note that very many Unix commands output something. In this case, though,
it will go to your `stdout`.


# Process Info

The `process_info` class reads the `/proc/<pid>/...` files about a
process and parses the data for you. You can then gather that data
using the class' member functions.

This gives you much information, such as how much memory, CPU, and
I/O the process has used so far.


# License

The project is covered by the GPL 2.0 license.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/eventdispatcher/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
