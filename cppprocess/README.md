
# Introduction

This sub-library is used to handle processes as in a shell execution
environment.

At this time, it includes two main parts:

1. the `process` class to run one or more processes (piped between each other);
2. the `process_info` class to read process information from `/proc`.

# Process

The `process` class is used to run a command (`fork()` + `execve()`). The
class is capable of handling the input and output/error streams via pipes
and files allowing you to send data to the command and receive the output
add errors.

You can also pipe multiple processes one after the other using the
`add_next_process()`. By default, this feature pipes each process to the
next. You can also add more than one next process in which case the list
of processes all receive a copy of the output of the previous process
(i.e. a `tee` feature).

## Example

To use the `process` class you can use your own pipes, in which case you
are responsible for reading and writing data from/to the process. If you
are not otherwise using the `ed::communicator`, you may also want to use
the `process::wait()` command. You can also let the `process` object
create internal pipes if you want to send stdin and receive the replies
in stdout and stderr.

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

    // you can add input using the io_data_pipe if you do not want to
    // handle a pipe yourself; this will create an automated input pipe;
    // the one drawback is you have to add all the data ahead of time
    // (before you call the process::start() function)
    //
    // the input can be a string or binary data
    //
    // otherwise, you can define your own pipe; derive it from
    // cppprocess::io_pipe and implement the process_write() as required
    // (i.e. process_write() because you are writing to the child process)
    //
    // if you do not create an input I/O object, then the process object uses
    // your stdin file
    //
    // as described below, you can add a _done_ callback to all I/O objects
    // which gets called once the object is done; it's generally less useful
    // for the input pipe
    //
    cppprocess::io_data_pipe::pointer_t input(std::make_shared<cppprocess::io_data_pipe>());
    input->add_input("Input here");
    p.set_input_io(input);

    // by default the output is sent to stdout, which in many cases is good
    // if you do not need it
    //
    // if instead you want to capture the output, you can either supply
    // your own io_pipe object or use the io_capture_pipe object;
    // if you use the io_capture_pipe, the whole output will be
    // available only once the command exited although you can try
    // to get partial data as it comes in; make sure to use the reset
    // if you do not want data to be repeated
    //
    cppprocess::io_capture_pipe::pointer_t output(std::make_shared<cppprocess::io_capture_pipe>());
    p.set_output_io(output);
    // and then later: output->get_output();
    // -- or --
    p.set_output_io(my_output_pipe);

    // to know when the output is ready, you can add a _done_ callback which
    // definition is:
    //
    //     bool (done_callback)(io *, done_reason_t);
    //
    // at the time the callback is called, the pipe was closed by the other
    // side so you can use the get_output() function to retrieve all the
    // output data
    //
    output->add_process_done_callback(...);

    // as with stdout, you can capture stderr and also add a callback
    //
    cppprocess::io_capture_pipe::pointer_t error(std::make_shared<cppprocess::io_capture_pipe>());
    p.set_error_io(error);
    // and then later: error->get_output();
    // -- or --
    p.set_error_pipe(my_error_pipe);

    // now that we are ready, we can start the process, it will run in
    // parallel with ed::communicator events; to know that it ends, though,
    // you want to setup a listener in the signal_child singleton; if you
    // are not using the ed::communicator, use the wait() instead
    //
    r = p.start();
    // if no ed::communicator, use: p.wait()
    if(r != 0)
    {
        ...handle error...
        return;
    }

    // with ed::communicator, you get a call to your listener (callback)
    // that you add to the signal_child object as below. Once you received
    // that signal, the child process died. If you pipe multiple processes,
    // you will receive one signal per process.
    //
    // note that the signal may be called when the process receives a signal
    // and does not actually die; specifically for the SIGSTOP, SIGCONT, and
    // SIGTRAP.
    //
    // NOTE: the p.wait() function uses these callback functions if
    //       you would like to see how it is done
    //
    // WARNING: the add_listener() has to be called after the start()
    //          because you need the pid_t of the child and for proper
    //          synchronization it must happen before you return back
    //          to the ed::communitor::run() loop; the signal_child is
    //          also expected to be added to the communicator and the
    //          add_listener() function does that automatically for us;
    //          it also gets removed automatically when the SIGCHILD is
    //          received (the resource is ref-counted)
    //
    ed::signal_child::pointer_t child_signal(ed::signal_child::get_instance());
    child_signal->add_listener(
              p.process_pid()
            , std::bind(&my_class::process_done, this, std::placeholders::_1));

    // to prematurely stop the process, send a signal with the kill()
    // function
    //
    p.kill(SIGTERM);

**WARNING:** The pipes added to a process will have one side closed at
the time the `process::start()` function gets called. The other side must
be closed as soon as you are done with it (especially the input since this
is how the child process gets an EOF on that stream). In other words,
a pipe can be used once. Each instance of a process, new or not, must
be given a new pipe. It is strongly advised that you use new `cppprocess`
objects each time you run a new command pipeline.

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

**IMPORTANT NOTE:** When running a command from a Qt environment, it may not
work if you do not reassign the input, output, and error stream. Therefore
this version may not work in a Qt application. We suggest that you define
all three pipes with Qt apps.

## Simulate a popen("cmd", "r") call

To simulate a `popen()` call with the `"r"` option, you want to capture the
output. If you just want to run one command and get the output as a string
all at once, then you can just use the capture flag like so:

    cppprocess::io_capture_pipe::pointer_t output(std::make_shared<cppprocess::io_capture_pipe>());
    
    cppprocess::process p("my-command");
    p.set_command("cat");
    p.add_argument("/proc/cpuinfo");
    p.set_output_io(output);
    p.start();
    p.wait();       // if you don't use ed::communicator
    
    std::string output(output->get_output());

You can also get the trimmed output, which removes extra spaces and new line
characters before returning the output string. If the output is binary data,
then use the `get_binary_output()` to get the `buffer_t` as is.

## Simulate a popen("cmd", "w") call

To simulate a `popen()` call with the `"w"` option, you want to add the
input to the process before you call the `start()` function.

    cppprocess::io_data_pipe::pointer_t input(std::make_shared<cppprocess::io_data_pipe>());
    input->add_input("Message to send to syslog");
    
    cppprocess::process p("my-command");
    p.set_command("logger");
    p.set_input_io(input);
    p.start();
    p.wait();       // if you don't use ed::communicator

Note that very many Unix commands output something. In this case, though,
it will go to your `stdout`.

## Shell Equivalents

### Input Pipe

Using an input pipe with in memory data is equivalent to:

    cat <file> | <command> ...

The `io_data_pipe::add_input()` and `process::set_input_io()` both give you
the same functionality. The `io_data_pipe::add_input()` uses an internal pipe
meaning that you do not need to supply anything more to make it all work.

### Input File

You can supply a path and filename as input. In this case, it has to be a
file on disk. This is equivalent to:

    <command> < <filename>

The file has to exist and be readable.

Use the `io_input_file` for this purpose.

### Same Input

By default, the input comes from `stdin`. This happens if you do not
supply input (`process::set_input_io()`).

### Output Pipe

Using the output pipe is equivalent to:

    <command> ... | [capture]

The capture is internal to your C++ process hence this `[capture]` entry.
If you have multiple commands, only the last one can have such a capture
entry.

If you use the tee capability, then you can have multiple _last command_.

### Output File

You can supply a path and filename as output. In this case, it writes the
output to that file. This is equivalent to:

    <command> > <filename>
     -- or --
    <command> >> <filename>

The file has to be created or exists and can be written to.

The `>>` works if you do set the truncate flag to false and the append
file to true.

### Same Output

By default, the output will be sent to `stdout`. This happens if you
do not ask for any capture (i.e do not call `set_output_io()`).

### Error Stream

Like the output, the error stream can use a pipe, a filename, or
the default `stderr`. The filename shell equivalent is like so:

    <command> 2> <filename>
    <command> 2>> <filename>

See the `process::set_error_io()` function.

### Piping

The `process` class allows for adding a _next process_. This is how
you create a pipe between multiple processes. The equivalent of:

    <command1> | <command2> | <command3> | ...

The output of `<command1>` will automatically be sent to the input
of `<command2>`, the output of `<command2>` will automatically be
sent to `<command3>`, etc.

You can still send data to the first command (see `io_data_pipe` or
`io_input_file`) and capture the final output (i.e. the output of the last
command with `io_capture_pipe` or `io_output_file`).

    [input] | <command1> | <command2> | <command3> | [output]

Every command can be given its own specific capture I/O for their error
stream, or they call can use the same error stream.

And since our system works with signals, you can simply add a
callback function which gets called once the output is ready
for consumption and delete the process chain at that point.


# Process Info

The `process_info` class reads the `/proc/<pid>/...` files about a
process and parses the data for you. You can then gather that data
using the class' member functions.

This gives you much information, such as how much memory, CPU, and
I/O the process has used so far.

To check out the currently running list of processes, use the `process_list`
class. This is an extension of `std::map<>` that reads the current list
of processes found under `/proc`. Each process is defined with a pointer to
a `process_info` object found in said map which you can search using the
process pid or basename.


# TODO

* Add support for an I/O which is a file descriptor.

* Clean up the I/O (docs, comments, etc.)

* Write a tool that would allow for pipelines with tees (which would also
  allow us to test that feature). The "piper" shell which allows for tees
  to be created as in:

        a | b | e
          |   | f
          | c | g
          |   | h
          |   | i
          | d

    In that example, command `a` output (stdout) is sent to command `b`.
    The second output of `a` (stderr) is sent to command `c`. A third output
    is created and sent to command `d`.

    Similarly, `b` sends its output to `e` and `f`.

    And `c` to `g`, `h`, `i`.

    We can also support redirections (<file, >file, >>file, 2>&1, etc.) and
    arguments as with a _regular_ shell.


# License

The project is covered by the GPL 2.0 license.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/eventdispatcher/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
