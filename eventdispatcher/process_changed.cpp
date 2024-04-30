// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/** \file
 * \brief Process changed class.
 *
 * Class used to listen on process events. This connection is useful to
 * know each time a process is created or stopped. Note, however, that your
 * process needs to be root at the time you create the connection.
 *
 * This connection sends you a process identifier (pid) and a thread
 * identifier (tgid). You can use these identifiers with the
 * cppprocess::process_info class to retrieve all the available process
 * information.
 *
 * Based on code found here:
 * http://bewareofgeek.livejournal.com/2945.html
 */

// self
//
#include    <eventdispatcher/process_changed.h>

#include    <eventdispatcher/exception.h>
#include    <eventdispatcher/utils.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C
//
#include    <linux/connector.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include    <linux/cn_proc.h>
#pragma GCC diagnostic pop
#include    <linux/netlink.h>
#include    <linux/version.h>
#include    <sys/socket.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{
namespace
{



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
constexpr char const * const g_process_event_to_string[] =
{
    [static_cast<int>(process_event_t::PROCESS_EVENT_UNKNOWN )] = "UNKNOWN",
    [static_cast<int>(process_event_t::PROCESS_EVENT_NONE    )] = "NONE",
    [static_cast<int>(process_event_t::PROCESS_EVENT_FORK    )] = "FORK",
    [static_cast<int>(process_event_t::PROCESS_EVENT_EXEC    )] = "EXEC",
    [static_cast<int>(process_event_t::PROCESS_EVENT_UID     )] = "UID",
    [static_cast<int>(process_event_t::PROCESS_EVENT_GID     )] = "GID",
    [static_cast<int>(process_event_t::PROCESS_EVENT_SESSION )] = "SESSION",
    [static_cast<int>(process_event_t::PROCESS_EVENT_PTRACE  )] = "PTRACE",
    [static_cast<int>(process_event_t::PROCESS_EVENT_COMMAND )] = "COMMAND",
    [static_cast<int>(process_event_t::PROCESS_EVENT_COREDUMP)] = "COREDUMP",
    [static_cast<int>(process_event_t::PROCESS_EVENT_EXIT    )] = "EXIT",
};
#pragma GCC diagnostic pop



} // no name namespace



/** \brief Convert a process event number to a string.
 *
 * This function transforms a process event to a human readable string.
 *
 * \exception parameter_error
 * If an invalid event number is specified, then this error is raised.
 *
 * \exception implementation_error
 * If we have a missing name, this exception is raised. This should never
 * happen.
 *
 * \param[in] event  The event to convert.
 *
 * \return A bare pointer to a static string with the name.
 */
char const * process_event_to_string(process_event_t event)
{
    if(static_cast<std::size_t>(event) >= std::size(g_process_event_to_string))
    {
        throw parameter_error(
                  "event number ("
                + std::to_string(static_cast<int>(event))
                + ") out of range.");
    }

    char const * name(g_process_event_to_string[static_cast<std::size_t>(event)]);
    if(name == nullptr)
    {
        throw implementation_error(
                  "event number ("
                + std::to_string(static_cast<int>(event))
                + ") is missing its name in process_event_to_string().");
    }

    return name;
}




/** \brief Get the event type.
 *
 * The event time is a process_event_t enum value. This value is important
 * since in most cases the other parameters depend on it.
 *
 * * process_event_t::PROCESS_EVENT_NONE
 *
 *   This event comes with an error code you can retrieve using the
 *   get_exit_code() function. This should always be 0 unless something
 *   changed in the kernel breaking our send() function call.
 *
 * * process_event_t::PROCESS_EVENT_UNKNOWN
 *
 *   If there is a new kernel even we do not know about, this event is sent
 *   to you. The get_exit_code() will give you the exact kernel number.
 *
 *   \note The first such even is reported through a warning in the logs.
 *
 * * process_event_t::PROCESS_EVENT_FORK
 *
 *   The FORK event supports the child pid/tgid and the parent pid/tgid.
 *   The child pid/tgid are retrieved with the get_pid() and get_tgid().
 *   The parent are retrieved using the get_parent_pid() and
 *   get_parent_tgid().
 *
 * * process_event_t::PROCESS_EVENT_EXEC
 *
 *   The EXEC event tells you that a process started executing a new
 *   program (i.e. a fork() + execve() or similar).
 *
 *   The event include the process pid/tgid which can be retrieved
 *   using get_pid() and get_tgid(). It will always be preceeded by
 *   a FORK event and it will end with an EXIT or COREDUMP event.
 *
 * * process_event_t::PROCESS_EVENT_UID
 *
 *   The UID event tells you that a process just changed from one user
 *   to another. In most cases, this means the euid changed. The ruid
 *   changes in a forked process whenever the process forfeits its
 *   ability to become a different user (usually root).
 *
 *   You can retrieve the information with the get_ruid() and get_euid()
 *   functions.
 *
 *   The affected process identifiers can be retrieved using the get_pid()
 *   and get_tgid() functions.
 *
 * * process_event_t::PROCESS_EVENT_GID
 *
 *   The GID event is like the UID event for the main process group. The
 *   get_rgid() and get_egid() functions are used to retrieve the information.
 *
 * * process_event_t::PROCESS_EVENT_SESSION
 *
 *   The SESSION event happens when a child process becomes the main process
 *   and thus its PID now represents a session (a group of processes). The
 *   get_pid() and get_tgid() functions can be used to determine the process
 *   and thus the new session numbers.
 *
 * * process_event_t::PROCESS_EVENT_PTRACE
 *
 *   A process trace started. You can retrieve the identifiers of the process
 *   being traced with get_pid() and get_tgid(). The tracer process information
 *   is retrieved using the get_parent_pid() and get_parent_tgid().
 *
 * * process_event_t::PROCESS_EVENT_COMMAND
 *
 *   The COMMAND event is emitted whenever the `/proc/<pid>/comm` is updated
 *   with a new name. You can retrieve the new name using the get_command()
 *   function. The get_pid() and get_tgid() give you the identifiers of the
 *   corresponding process.
 *
 * * process_event_t::PROCESS_EVENT_COREDUMP
 *
 *   The process generated a coredump. This event gives you the process
 *   identifiers through get_pid() and get_tgid().
 *
 * * process_event_t::PROCESS_EVENT_EXIT
 *
 *   The process exited either normally or with a signal. To know which,
 *   do a get_exit_signal(), if this value is -1, then the process exited
 *   normally and the get_exit_code() returns the exit value as passed
 *   to the exit(3) function or a return statement in the main() function.
 *
 *   The get_pid() and get_tgid() functions return the process identifiers
 *   of the process that just exited.
 *
 * This value is always defined. Also the CPU and timestamps are also always
 * defined.
 *
 * \return The process event type.
 */
process_event_t process_changed_event::get_event() const
{
    return f_event;
}


/** \brief Set the type of event.
 *
 * This function is used to change the process_event_t value. By default
 * this is set to UNKNOWN.
 *
 * \param[in] event  The new event type.
 */
void process_changed_event::set_event(process_event_t event)
{
    f_event = event;
}


/** \brief Get the CPU handling that process.
 *
 * This field includes the CPU which handles the concerned process.
 * All events are assigned a CPU number.
 *
 * On a machine with 1 CPU, this value is always zero.
 *
 * \return The CPU handling this process.
 */
std::uint32_t process_changed_event::get_cpu() const
{
    return f_cpu;
}


/** \brief Set the CPU number.
 *
 * Whenever an event is received, this value is set to the CPU handling
 * the request.
 *
 * \param[in] cpu  Zero based CPU number handling the process.
 */
void process_changed_event::set_cpu(std::uint32_t cpu)
{
    f_cpu = cpu;
}


/** \brief Time at which the event occurred in nanoseconds.
 *
 * This function returns the timestamp when the event occurred in the kernel.
 *
 * This value is always set.
 *
 * \warning
 * This is the number of nanoseconds since the last boot. It is not the
 * current UTC time. See the get_realtime() function for the UTC time.
 *
 * \return The timestamp when the event occurred.
 */
std::uint64_t process_changed_event::get_timestamp() const
{
    return f_timestamp;
}


/** \brief Get realtime when the event happened.
 *
 * This function returns the timestamp as an equivalent to a time_t in
 * a snapdev::timespec_ex object. This can then be converted to a date
 * and time as follow:
 *
 * \code
 *     std::string const date(event.get_realtime().to_string("%D %T.%N", true));
 * \endcode
 *
 * \return The timestamp as a snapdev::timespec_ex value representing a UTC
 * time.
 */
snapdev::timespec_ex process_changed_event::get_realtime() const
{
    snapdev::timespec_ex result(static_cast<std::int64_t>(f_timestamp));

    snapdev::timespec_ex realtime;
    clock_gettime(CLOCK_REALTIME, &realtime);

    snapdev::timespec_ex boottime;
    clock_gettime(CLOCK_BOOTTIME, &boottime);

    result += realtime - boottime;

    return result;
}


/** \brief Set the time when the event occurred in nanoseconds.
 *
 * This function saves the \p timestamp of when the event occurred.
 *
 * \param[in] timestamp  The timestamp of when the even occurred.
 */
void process_changed_event::set_timestamp(std::uint64_t timestamp)
{
    f_timestamp = timestamp;
}


/** \brief The PID of the concerned process.
 *
 * Most event have this parameter set (exceptions are NONE and UNKNOWN).
 * It represents the process identifier (pid_t) of the object that generated
 * this event.
 *
 * \return Process identifier or -1 if no process is attached to this event.
 */
pid_t process_changed_event::get_pid() const
{
    return f_pid;
}


/** \brief Set the event process identifier.
 *
 * This function is used to set the process identifier in relation to
 * a process event.
 *
 * This value is set to -1 if no process relates to the event.
 *
 * \param[in] pid  The process concerned with this event.
 */
void process_changed_event::set_pid(pid_t pid)
{
    f_pid = pid;
}


/** \brief The TID of the concerned thread.
 *
 * Most event have this parameter set (exceptions are NONE and UNKNOWN).
 * It represents the thread identifier (pid_t of a specific thread in a
 * process group) of the object that generated this event.
 *
 * \return Thread identifier or -1 if no process is attached to this event.
 *
 * \sa cppthread::gettid()
 */
pid_t process_changed_event::get_tgid() const
{
    return f_tgid;
}


/** \brief Set the event thread identifier.
 *
 * This function is used to set the thread identifier in relation to
 * a process event.
 *
 * This value is set to -1 if no process relates to the event.
 *
 * \param[in] tgid  The thread concerned with this event.
 */
void process_changed_event::set_tgid(pid_t tgid)
{
    f_tgid = tgid;
}


/** \brief The PID of the parent process.
 *
 * A few events have a parent process concept and use this parameter to
 * define that parent's process identifier (pid_t).
 *
 * The FORK event defines this parameter for the process that called the
 * fork() function.
 *
 * The PTRACE event defines this parameter with the pid_t of the process
 * tracing (i.e. the tracer).
 *
 * \return Parent process identifier or -1 if no such process is defined.
 */
pid_t process_changed_event::get_parent_pid() const
{
    return f_parent_pid;
}


/** \brief Set the event parent's process identifier.
 *
 * This function is used to set the process identifier of the parent process
 * in relation to an event.
 *
 * This value is set to -1 if no parent process relates to the event.
 *
 * \param[in] pid  The parent process concerned with this event.
 */
void process_changed_event::set_parent_pid(pid_t pid)
{
    f_parent_pid = pid;
}


/** \brief The TID of the parent process.
 *
 * A few events have a parent process concept and use this parameter to
 * define that parent's thread identifier.
 *
 * The FORK event defines this parameter for the process that called the
 * fork() function.
 *
 * The PTRACE event defines this parameter with the pid_t of the process
 * tracing (i.e. the tracer).
 *
 * \return Parent thread identifier or -1 if no such thread is defined.
 */
pid_t process_changed_event::get_parent_tgid() const
{
    return f_parent_tgid;
}


/** \brief Set the event parent's thread identifier.
 *
 * This function is used to set the thread identifier of the parent process
 * in relation to an event.
 *
 * This value is set to -1 if no thread process relates to the event.
 *
 * \param[in] tgid  The parent thread concerned with this event.
 */
void process_changed_event::set_parent_tgid(pid_t tgid)
{
    f_parent_tgid = tgid;
}


/** \brief Retrieve the real user identifier.
 *
 * The UID event defines the real user identifier which can be retreived
 * with this function. The value represents the real user identifier at
 * the time the event occurred. Most often, this value does not change
 * unless the process wants to drop all privileges forever.
 *
 * For all other events, this value is set to snapdev::NO_UID.
 *
 * \return The new real user identifier.
 */
uid_t process_changed_event::get_ruid() const
{
    return f_ruid;
}


/** \brief Set the real user identifier.
 *
 * This function is used to set the real user identifier.
 *
 * \param[in] uid  The new real user identifier.
 */
void process_changed_event::set_ruid(uid_t uid)
{
    f_ruid = uid;
}


/** \brief Retrieve the effective user identifier.
 *
 * The UID event defines the effective user identifier which can be retreived
 * with this function. The value represents the effective user identifier at
 * the time the event occurred. Most often, this value changes when a process
 * requires more privileges and then back to its normal user. For example,
 * the snapdev::as_root class does that to change to root (or some other user)
 * and back on destruction.
 *
 * For all other events, this value is set to snapdev::NO_UID.
 *
 * \return The new effective user identifier.
 */
uid_t process_changed_event::get_euid() const
{
    return f_euid;
}


/** \brief Set the effective user identifier.
 *
 * This function is used to set the effective user identifier.
 *
 * \param[in] uid  The new effective user identifier.
 */
void process_changed_event::set_euid(uid_t uid)
{
    f_euid = uid;
}


/** \brief Retrieve the real group identifier.
 *
 * The GID event defines the real group identifier which can be retreived
 * with this function. The value represents the real group identifier at
 * the time the event occurred. Most often, this value does not change
 * unless the process wants to drop all privileges forever.
 *
 * For all other events, this value is set to snapdev::NO_GID.
 *
 * \return The new real group identifier.
 */
gid_t process_changed_event::get_rgid() const
{
    return f_rgid;
}


/** \brief Set the real group identifier.
 *
 * This function is used to set the real group identifier.
 *
 * \param[in] gid  The new real group identifier.
 */
void process_changed_event::set_rgid(gid_t gid)
{
    f_rgid = gid;
}


/** \brief Retrieve the effective group identifier.
 *
 * The UID event defines the effective group identifier which can be retreived
 * with this function. The value represents the effective group identifier at
 * the time the event occurred. Most often, this value changes when a process
 * requires more privileges and then back to its normal group. For example,
 * the snapdev::as_root class does that to change to root (or some other group)
 * and back on destruction.
 *
 * For all other events, this value is set to snapdev::NO_GID.
 *
 * \return The new effective group identifier.
 */
gid_t process_changed_event::get_egid() const
{
    return f_egid;
}


/** \brief Set the effective group identifier.
 *
 * This function is used to set the effective group identifier.
 *
 * \param[in] gid  The new effective group identifier.
 */
void process_changed_event::set_egid(gid_t gid)
{
    f_egid = gid;
}


/** \brief Get the command name.
 *
 * The COMM event includes a command name, when a process changes its
 * `/proc/<pid>/comm` file with a new name.
 *
 * The value found in that file is returned by this function as a string.
 *
 * \return The new command name.
 */
std::string const & process_changed_event::get_command() const
{
    return f_command;
}


/** \brief Set the command name as found in the COMM event.
 *
 * This function saves the new command name of the specified process. This
 * happens when one overwrites the name in the `/proc/<pid>/comm` file.
 * This name is limited to 15 characters.
 *
 * \param[in] command  The new command name.
 */
void process_changed_event::set_command(std::string const & command)
{
    f_command = command;
}


/** \brief Exit code of the process or acknoledgement.
 *
 * This value represents the exit code when the event is
 * process_event_t::PROCESS_EVENT_EXIT. This is exactly what is placed
 * in the exit(3) function or the return statement of the main() function.
 * Note that the value is 32 bits although it is likely only holding 8 bits.
 *
 * This value is also used for the acknowledgement sent to us when you change
 * the enabled flag. This comes in as the
 * process_event_t::PROCESS_EVENT_NONE. In this case, this represents a
 * possible error in your message or 0 if the message was accepted.
 *
 * \return The exit code or acknowledgement error.
 */
std::int32_t process_changed_event::get_exit_code() const
{
    return f_exit_code;
}


/** \brief Save the exit code from the EXIT event.
 *
 * This function is used when we receive an EXIT even or a NONE event.
 *
 * In case of the EXIT, this represents the EXIT code as specified with
 * the exit(3) or the return statement in the main() function.
 *
 * In case of the NONE event, this is used to save the error code (an
 * errno) acknowledging success or failure of the last send() call.
 * For example, the function may return EINVAL if the `cn_mcast` value
 * is not a valid number (`PROC_CN_MCAST_LISTEN` or `PROC_CN_MCAST_IGNORE`).
 *
 * \param[in] code  The exit code or errno.
 */
void process_changed_event::set_exit_code(std::int32_t code)
{
    f_exit_code = code;
}


/** \brief Retrieve the exit signal from the EXIT event.
 *
 * This value is set on the process_event_t::PROCESS_EVENT_EXIT event.
 * In all other cases, it is set to -1.
 *
 * If the process exited normally, the signal value is set to -1.
 *
 * \return The exit signal as found in the EXIT event.
 */
std::int32_t process_changed_event::get_exit_signal() const
{
    return f_exit_signal;
}


/** \brief Save the exit signal on an EXIT event.
 *
 * This function saves the exit signal found in the EXIT event.
 *
 * \param[in] signal  The signal found in the EXIT event.
 */
void process_changed_event::set_exit_signal(std::int32_t signal)
{
    f_exit_signal = signal;
}







/** \brief Create a listener of process changes.
 *
 * This constructor creates a socket to listen to process changes.
 *
 * \todo
 * TBD: Offer means to set or not set the SOCK_CLOEXEC flag. At the moment,
 * I'm thinking that this should be a service and other systems can register
 * to listen for changes they are interested (like one registers with the
 * communicator daemon or fluid settings services). This means this class
 * can stay this way.
 */
process_changed::process_changed()
{
    // TODO: look at whether we do want to allow SOCK_CLOEXEC to not be
    //       specified (keep in mind that this requires root to open so
    //       we probably want to not have it too loose...)
    //
    f_socket.reset(socket(PF_NETLINK, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, NETLINK_CONNECTOR));
    if(f_socket == nullptr)
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << "socket() failed to create a datagram NETLINK socket connector to listen to process events (errno: "
            << std::to_string(e)
            << " -- "
            << strerror(e)
            << ")"
            << SNAP_LOG_SEND;
        throw runtime_error("could not create socket for process events");
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    sockaddr_nl nl =
    {
        .nl_family = AF_NETLINK,
        .nl_pad = 0,
        .nl_pid = static_cast<std::uint32_t>(getpid()),
        .nl_groups = CN_IDX_PROC,
    };
#pragma GCC diagnostic pop

    int const r(bind(
              f_socket.get()
            , reinterpret_cast<struct sockaddr *>(&nl)
            , sizeof(nl)));
    if(r != 0)
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << "bind() failed to bind the datagram NETLINK socket for process events (errno: "
            << std::to_string(e)
            << " -- "
            << strerror(e)
            << ")"
            << SNAP_LOG_SEND;
        throw runtime_error("could not bind socket for process events");
    }

    listen_for_events();
}


/** \brief Make sure to disable the connection before closing it.
 *
 * This function disables the connection and then closes the socket. If an
 * error occurred reading data, the socket may already be closed.
 *
 * \bug
 * According to this post:
 * https://lore.kernel.org/lkml/20230329182543.1161480-4-anjali.k.kulkarni@oracle.com/
 * there was a bug in older versions where the disabling of one connection
 * can disable all connections. However, the effect I've seen is that
 * disabling (i.e. `set_enabled(false)`) had no effects at all.
 */
process_changed::~process_changed()
{
    if(f_socket != nullptr)
    {
        try
        {
            set_enable(false);
        }
        catch(runtime_error const &)
        {
            // set_enable() failed
        }
    }
}


/** \brief Enable or disable this connection.
 *
 * This function will enable (\p enabled is true) or disable (\p enabled is
 * false) the connection.
 *
 * \warning
 * In older kernels, the disabling will properly stop the events in our
 * system (our communicator class ignores disabled connections), however,
 * the kernel continues to send messages through your socket. To avoid
 * this issue, you want to consider deleting the connect altogether. See
 * the listen_for_events() function for additional details.
 *
 * \param[in] enabled  Whether to enable (true) or disable (false) this
 * connection.
 */
void process_changed::set_enable(bool enabled)
{
    if(enabled != is_enabled())
    {
        connection::set_enable(enabled);
        listen_for_events();
    }
}


/** \brief Send a message to listen for process changed events.
 *
 * This internal function makes sure that the socket is ready to wait
 * for process changes.
 *
 * If the connection is currently disabled, then the function asks the
 * kernel to stop sending data.
 *
 * \bug
 * Older kernels (up until 2023) ignore that flag and continue sending
 * data through the socket. You should not just disable this connection
 * on such kernels. Instead, you want to delete the connection and
 * re-establish it later when you need it again.
 */
void process_changed::listen_for_events()
{
    // with newer versions of g++ the following fails with an error telling
    // us that a struct can end with data[0], but nothing can follow that
    // field -- so instead we have to "manually create the struct" we
    // we do below
    //
    //struct __attribute__((aligned(NLMSG_ALIGNTO))) multicast_message {
    //    nlmsghdr f_nl_hdr;
    //    struct __attribute__((__packed__)) {
    //        cn_msg f_cn_msg;
    //        enum proc_cn_mcast_op f_cn_mcast;
    //    } f_nl_msg;
    //};
    constexpr std::size_t EVENT_MESSAGE_SIZE = sizeof(nlmsghdr) + sizeof(cn_msg) + sizeof(enum proc_cn_mcast_op);

    //multicast_message msg = {};
    //msg.f_nl_hdr.nlmsg_len = sizeof(multicast_message);
    //msg.f_nl_hdr.nlmsg_pid = getpid();
    //msg.f_nl_hdr.nlmsg_type = NLMSG_DONE;

    //msg.f_nl_msg.f_cn_msg.id.idx = CN_IDX_PROC;
    //msg.f_nl_msg.f_cn_msg.id.val = CN_VAL_PROC;
    //msg.f_nl_msg.f_cn_msg.len = sizeof(enum proc_cn_mcast_op);

    //msg.f_nl_msg.f_cn_mcast = is_enabled()
    //                            ? PROC_CN_MCAST_LISTEN
    //                            : PROC_CN_MCAST_IGNORE;

    std::vector<std::uint8_t> msg(EVENT_MESSAGE_SIZE);

    nlmsghdr * nl_hdr(reinterpret_cast<nlmsghdr *>(msg.data()));
    cn_msg * cnmsg(reinterpret_cast<cn_msg *>(nl_hdr + 1));
    enum proc_cn_mcast_op * proc_op(reinterpret_cast<enum proc_cn_mcast_op *>(cnmsg + 1));

    nl_hdr->nlmsg_len = EVENT_MESSAGE_SIZE;
    nl_hdr->nlmsg_pid = getpid();
    nl_hdr->nlmsg_type = NLMSG_DONE;

    cnmsg->id.idx = CN_IDX_PROC;
    cnmsg->id.val = CN_VAL_PROC;
    cnmsg->len = sizeof(enum proc_cn_mcast_op);

    *proc_op = is_enabled()
                ? PROC_CN_MCAST_LISTEN
                : PROC_CN_MCAST_IGNORE;

    int const r(send(f_socket.get(), msg.data(), msg.size(), 0));
    if(r < 0)
    {
        f_socket.reset();
        int const e(errno);
        SNAP_LOG_ERROR
            << "send() failed to enable/disable datagram NETLINK socket for process events (errno: "
            << std::to_string(e)
            << " -- "
            << strerror(e)
            << ")"
            << SNAP_LOG_SEND;
        throw runtime_error("could not bind socket for process events");
    }
}


bool process_changed::is_reader() const
{
    return true;
}


int process_changed::get_socket() const
{
    return f_socket.get();
}


void process_changed::process_read()
{
    static bool g_received_unknown_event = false;

    // with newer versions of g++ the following fails with an error telling
    // us that a struct can end with data[0], but nothing can follow that
    // field -- so instead we have to "manually create the struct" we
    // we do below
    //
    //struct __attribute__((aligned(NLMSG_ALIGNTO))) event_message {
    //    nlmsghdr f_nl_hdr;
    //    struct __attribute__((__packed__)) {
    //        cn_msg f_cn_msg;
    //        proc_event f_proc_ev;
    //    } f_nl_msg;
    //};
    constexpr std::size_t EVENT_MESSAGE_SIZE = sizeof(nlmsghdr) + sizeof(cn_msg) + sizeof(proc_event);

    std::int64_t const date_limit(get_current_date() + get_processing_time_limit());
    int count(0);
    std::vector<std::uint8_t> msg(EVENT_MESSAGE_SIZE);
    do
    {
        int const r(recv(f_socket.get(), msg.data(), msg.size(), 0));
        if(r > 0)
        {
            nlmsghdr const * nl_hdr(reinterpret_cast<nlmsghdr const *>(msg.data()));
            cn_msg const * cnmsg(reinterpret_cast<cn_msg const *>(nl_hdr + 1));
            proc_event const * proc_ev(reinterpret_cast<proc_event const *>(cnmsg + 1));

            process_changed_event event;
            event.set_cpu(proc_ev->cpu);
            event.set_timestamp(proc_ev->timestamp_ns);

// in newer versions of Linux the PROC_EVENT_... are not defined inside the
// proc_event structure
//
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,8,1)
#define proc_event_namespace
#else
#define proc_event_namespace proc_event::
#endif
            switch(proc_ev->what)
            {
            case proc_event_namespace PROC_EVENT_NONE:
                // report the error as an exit code (same size, it fits)
                event.set_event(process_event_t::PROCESS_EVENT_NONE);
                event.set_exit_code(proc_ev->event_data.ack.err);
                break;

            case proc_event_namespace PROC_EVENT_FORK:
                event.set_event(process_event_t::PROCESS_EVENT_FORK);
                event.set_pid(proc_ev->event_data.fork.child_pid);
                event.set_tgid(proc_ev->event_data.fork.child_tgid);
                event.set_parent_pid(proc_ev->event_data.fork.parent_pid);
                event.set_parent_tgid(proc_ev->event_data.fork.parent_tgid);
                break;

            case proc_event_namespace PROC_EVENT_EXEC:
                event.set_event(process_event_t::PROCESS_EVENT_EXEC);
                event.set_pid(proc_ev->event_data.id.process_pid);
                event.set_tgid(proc_ev->event_data.id.process_tgid);
                break;

            case proc_event_namespace PROC_EVENT_UID:
                event.set_event(process_event_t::PROCESS_EVENT_UID);
                event.set_pid(proc_ev->event_data.id.process_pid);
                event.set_tgid(proc_ev->event_data.id.process_tgid);
                event.set_ruid(proc_ev->event_data.id.r.ruid);
                event.set_euid(proc_ev->event_data.id.e.euid);
                break;

            case proc_event_namespace PROC_EVENT_GID:
                event.set_event(process_event_t::PROCESS_EVENT_GID);
                event.set_pid(proc_ev->event_data.id.process_pid);
                event.set_tgid(proc_ev->event_data.id.process_tgid);
                event.set_rgid(proc_ev->event_data.id.r.rgid);
                event.set_egid(proc_ev->event_data.id.e.egid);
                break;

            case proc_event_namespace PROC_EVENT_SID:
                event.set_event(process_event_t::PROCESS_EVENT_SESSION);
                event.set_pid(proc_ev->event_data.sid.process_pid);
                event.set_tgid(proc_ev->event_data.sid.process_tgid);
                break;

            case proc_event_namespace PROC_EVENT_PTRACE:
                event.set_event(process_event_t::PROCESS_EVENT_PTRACE);
                event.set_pid(proc_ev->event_data.ptrace.process_pid);
                event.set_tgid(proc_ev->event_data.ptrace.process_tgid);
                event.set_parent_pid(proc_ev->event_data.ptrace.tracer_pid);
                event.set_parent_tgid(proc_ev->event_data.ptrace.tracer_tgid);
                break;

            case proc_event_namespace PROC_EVENT_COMM:
                event.set_event(process_event_t::PROCESS_EVENT_COMMAND);
                event.set_pid(proc_ev->event_data.comm.process_pid);
                event.set_tgid(proc_ev->event_data.comm.process_tgid);

                // as far as I know the "comm" field can be a max. of 15
                // characters, but just in case, the following assumes
                // it is not null terminated
                //
                event.set_command(std::string(
                          proc_ev->event_data.comm.comm
                        , strnlen(proc_ev->event_data.comm.comm
                                , sizeof(proc_ev->event_data.comm.comm))));
                break;

            case proc_event_namespace PROC_EVENT_COREDUMP:
                event.set_event(process_event_t::PROCESS_EVENT_COREDUMP);
                event.set_pid(proc_ev->event_data.coredump.process_pid);
                event.set_tgid(proc_ev->event_data.coredump.process_tgid);
                break;

            case proc_event_namespace PROC_EVENT_EXIT:
                event.set_event(process_event_t::PROCESS_EVENT_EXIT);
                event.set_pid(proc_ev->event_data.exit.process_pid);
                event.set_tgid(proc_ev->event_data.exit.process_tgid);
                event.set_exit_code(proc_ev->event_data.exit.exit_code);
                event.set_exit_signal(proc_ev->event_data.exit.exit_signal);
                break;

            default:
                event.set_event(process_event_t::PROCESS_EVENT_UNKNOWN);
                event.set_exit_code(proc_ev->what);
                if(!g_received_unknown_event)
                {
                    g_received_unknown_event = true;
                    SNAP_LOG_WARNING
                        << "process_changed::process_read() received unknown proc_event: "
                        << static_cast<int>(proc_ev->what)
                        << SNAP_LOG_SEND;
                }
                break;

            }
            process_event(event);
        }
        else if(r == 0 || errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // no more data available at this time
            break;
        }
        else //if(r < 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "an error occurred while reading from NETLINK process socket (errno: "
                << e
                << " -- "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }
        ++count;
    }
    while(count < get_event_limit() && get_current_date() < date_limit);

    // process next level too
    //
    connection::process_read();
}


/** \fn process_changed::process_event()
 * \brief New callback used to process one event.
 *
 * Each time the kernel sends us a process event, we call this function.
 * Your implementation of the process_changed class must include an
 * override of this function.
 *
 * \param[in] event  The event that just happened.
 */


} // namespace ed
// vim: ts=4 sw=4 et
