// Copyright (c) 2012-2023  Made to Order Software Corp.  All Rights Reserved
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
#include    <linux/cn_proc.h>
#include    <linux/netlink.h>
#include    <sys/socket.h>

// #include <signal.h>
// #include <errno.h>
// #include <stdbool.h>
// #include <unistd.h>
// #include <string.h>
// #include <stdlib.h>
// #include <stdio.h>
// #include <fcntl.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{
namespace
{



constexpr char const * const g_process_event_to_string[] =
{
    /* PROCESS_EVENT_NONE       */ "NONE",
    /* PROCESS_EVENT_UNKNOWN    */ "UNKNOWN",
    /* PROCESS_EVENT_FORK       */ "FORK",
    /* PROCESS_EVENT_EXEC       */ "EXEC",
    /* PROCESS_EVENT_UID        */ "UID",
    /* PROCESS_EVENT_GID        */ "GID",
    /* PROCESS_EVENT_SESSION    */ "SESSION",
    /* PROCESS_EVENT_PTRACE     */ "PTRACE",
    /* PROCESS_EVENT_COMMAND    */ "COMMAND",
    /* PROCESS_EVENT_COREDUMP   */ "COREDUMP",
    /* PROCESS_EVENT_EXIT       */ "EXIT",
};



} // no name namespace



char const * process_event_to_string(process_event_t event)
{
    if(static_cast<std::size_t>(event) >= std::size(g_process_event_to_string))
    {
        throw runtime_error(
                  "event number ("
                + std::to_string(static_cast<int>(event))
                + ") out of range.");
    }

    return g_process_event_to_string[static_cast<std::size_t>(event)];
}




process_event_t process_changed_event::get_event() const
{
    return f_event;
}


void process_changed_event::set_event(process_event_t event)
{
    f_event = event;
}


std::uint32_t process_changed_event::get_cpu() const
{
    return f_cpu;
}


void process_changed_event::set_cpu(std::uint32_t cpu)
{
    f_cpu = cpu;
}


std::uint64_t process_changed_event::get_timestamp() const
{
    return f_timestamp;
}


void process_changed_event::set_timestamp(std::uint64_t timestamp)
{
    f_timestamp = timestamp;
}


pid_t process_changed_event::get_pid() const
{
    return f_pid;
}


void process_changed_event::set_pid(pid_t pid)
{
    f_pid = pid;
}


pid_t process_changed_event::get_tgid() const
{
    return f_tgid;
}


void process_changed_event::set_tgid(pid_t tgid)
{
    f_tgid = tgid;
}


pid_t process_changed_event::get_parent_pid() const
{
    return f_parent_pid;
}


void process_changed_event::set_parent_pid(pid_t pid)
{
    f_parent_pid = pid;
}


pid_t process_changed_event::get_parent_tgid() const
{
    return f_parent_tgid;
}


void process_changed_event::set_parent_tgid(pid_t tgid)
{
    f_parent_tgid = tgid;
}


uid_t process_changed_event::get_ruid() const
{
    return f_ruid;
}


void process_changed_event::set_ruid(uid_t uid)
{
    f_ruid = uid;
}


uid_t process_changed_event::get_euid() const
{
    return f_euid;
}


void process_changed_event::set_euid(uid_t uid)
{
    f_euid = uid;
}


gid_t process_changed_event::get_rgid() const
{
    return f_rgid;
}


void process_changed_event::set_rgid(gid_t gid)
{
    f_rgid = gid;
}


gid_t process_changed_event::get_egid() const
{
    return f_egid;
}


void process_changed_event::set_egid(gid_t gid)
{
    f_egid = gid;
}


std::string const & process_changed_event::get_command() const
{
    return f_command;
}


void process_changed_event::set_command(std::string const & command)
{
    f_command = command;
}


std::int32_t process_changed_event::get_exit_code() const
{
    return f_exit_code;
}


void process_changed_event::set_exit_code(std::int32_t code)
{
    f_exit_code = code;
}


std::int32_t process_changed_event::get_exit_signal() const
{
    return f_exit_signal;
}


void process_changed_event::set_exit_signal(std::int32_t signal)
{
    f_exit_signal = signal;
}







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


void process_changed::set_enable(bool enabled)
{
    if(enabled != is_enabled())
    {
        connection::set_enable(enabled);
        listen_for_events();
    }
}


void process_changed::listen_for_events()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    struct __attribute__((aligned(NLMSG_ALIGNTO))) multicast_message {
        nlmsghdr f_nl_hdr;
        struct __attribute__((__packed__)) {
            cn_msg f_cn_msg;
            enum proc_cn_mcast_op f_cn_mcast;
        } f_nl_msg;
    };
#pragma GCC diagnostic pop

    multicast_message msg = {};
    msg.f_nl_hdr.nlmsg_len = sizeof(multicast_message);
    msg.f_nl_hdr.nlmsg_pid = getpid();
    msg.f_nl_hdr.nlmsg_type = NLMSG_DONE;

    msg.f_nl_msg.f_cn_msg.id.idx = CN_IDX_PROC;
    msg.f_nl_msg.f_cn_msg.id.val = CN_VAL_PROC;
    msg.f_nl_msg.f_cn_msg.len = sizeof(enum proc_cn_mcast_op);

    msg.f_nl_msg.f_cn_mcast = is_enabled()
                                ? PROC_CN_MCAST_LISTEN
                                : PROC_CN_MCAST_IGNORE;

    int const r(send(f_socket.get(), &msg, sizeof(msg), 0));
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    struct __attribute__((aligned(NLMSG_ALIGNTO))) event_message {
        nlmsghdr f_nl_hdr;
        struct __attribute__((__packed__)) {
            cn_msg f_cn_msg;
            proc_event f_proc_ev;
        } f_nl_msg;
    };
#pragma GCC diagnostic pop

    std::int64_t const date_limit(get_current_date() + get_processing_time_limit());
    int count(0);
    do
    {
        event_message msg;
        int const r(recv(f_socket.get(), &msg, sizeof(msg), 0));
        if(r > 0)
        {
            process_changed_event event;
            event.set_cpu(msg.f_nl_msg.f_proc_ev.cpu);
            event.set_timestamp(msg.f_nl_msg.f_proc_ev.timestamp_ns);
            switch(msg.f_nl_msg.f_proc_ev.what)
            {
            case proc_event::PROC_EVENT_NONE:
                // report the error as an exit code (same size, it fits)
                event.set_exit_code(msg.f_nl_msg.f_proc_ev.event_data.ack.err);
                break;

            case proc_event::PROC_EVENT_FORK:
                event.set_event(process_event_t::PROCESS_EVENT_FORK);
                event.set_pid(msg.f_nl_msg.f_proc_ev.event_data.fork.child_pid);
                event.set_tgid(msg.f_nl_msg.f_proc_ev.event_data.fork.child_tgid);
                event.set_parent_pid(msg.f_nl_msg.f_proc_ev.event_data.fork.parent_pid);
                event.set_parent_tgid(msg.f_nl_msg.f_proc_ev.event_data.fork.parent_tgid);
                break;

            case proc_event::PROC_EVENT_EXEC:
                event.set_event(process_event_t::PROCESS_EVENT_FORK);
                event.set_pid(msg.f_nl_msg.f_proc_ev.event_data.id.process_pid);
                event.set_tgid(msg.f_nl_msg.f_proc_ev.event_data.id.process_tgid);
                break;

            case proc_event::PROC_EVENT_UID:
                event.set_event(process_event_t::PROCESS_EVENT_UID);
                event.set_pid(msg.f_nl_msg.f_proc_ev.event_data.id.process_pid);
                event.set_tgid(msg.f_nl_msg.f_proc_ev.event_data.id.process_tgid);
                event.set_ruid(msg.f_nl_msg.f_proc_ev.event_data.id.r.ruid);
                event.set_euid(msg.f_nl_msg.f_proc_ev.event_data.id.e.euid);
                break;

            case proc_event::PROC_EVENT_GID:
                event.set_event(process_event_t::PROCESS_EVENT_GID);
                event.set_pid(msg.f_nl_msg.f_proc_ev.event_data.id.process_pid);
                event.set_tgid(msg.f_nl_msg.f_proc_ev.event_data.id.process_tgid);
                event.set_rgid(msg.f_nl_msg.f_proc_ev.event_data.id.r.rgid);
                event.set_egid(msg.f_nl_msg.f_proc_ev.event_data.id.e.egid);
                break;

            case proc_event::PROC_EVENT_SID:
                event.set_event(process_event_t::PROCESS_EVENT_SESSION);
                event.set_pid(msg.f_nl_msg.f_proc_ev.event_data.sid.process_pid);
                event.set_tgid(msg.f_nl_msg.f_proc_ev.event_data.sid.process_tgid);
                break;

            case proc_event::PROC_EVENT_PTRACE:
                event.set_event(process_event_t::PROCESS_EVENT_PTRACE);
                event.set_pid(msg.f_nl_msg.f_proc_ev.event_data.ptrace.process_pid);
                event.set_tgid(msg.f_nl_msg.f_proc_ev.event_data.ptrace.process_tgid);
                event.set_parent_pid(msg.f_nl_msg.f_proc_ev.event_data.ptrace.tracer_pid);
                event.set_parent_tgid(msg.f_nl_msg.f_proc_ev.event_data.ptrace.tracer_tgid);
                break;

            case proc_event::PROC_EVENT_COMM:
                event.set_event(process_event_t::PROCESS_EVENT_COMMAND);
                event.set_pid(msg.f_nl_msg.f_proc_ev.event_data.comm.process_pid);
                event.set_tgid(msg.f_nl_msg.f_proc_ev.event_data.comm.process_tgid);

                // as far as I know the "comm" field can be a max. of 15
                // characters, but just in case, the following assumes
                // it is not null terminated
                //
                event.set_command(std::string(
                          msg.f_nl_msg.f_proc_ev.event_data.comm.comm
                        , strnlen(msg.f_nl_msg.f_proc_ev.event_data.comm.comm
                                , sizeof(msg.f_nl_msg.f_proc_ev.event_data.comm.comm))));
                break;

            case proc_event::PROC_EVENT_COREDUMP:
                event.set_event(process_event_t::PROCESS_EVENT_COREDUMP);
                event.set_pid(msg.f_nl_msg.f_proc_ev.event_data.coredump.process_pid);
                event.set_tgid(msg.f_nl_msg.f_proc_ev.event_data.coredump.process_tgid);
                break;

            case proc_event::PROC_EVENT_EXIT:
                event.set_event(process_event_t::PROCESS_EVENT_EXIT);
                event.set_pid(msg.f_nl_msg.f_proc_ev.event_data.exit.process_pid);
                event.set_tgid(msg.f_nl_msg.f_proc_ev.event_data.exit.process_tgid);
                event.set_exit_code(msg.f_nl_msg.f_proc_ev.event_data.exit.exit_code);
                event.set_exit_signal(msg.f_nl_msg.f_proc_ev.event_data.exit.exit_signal);
                break;

            default:
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

//             default:
//                 printf("unhandled proc event\n");
//                 break;
//         }
//     }
// 
//     return 0;
// }
}



// 
// /*
//  * connect to netlink
//  * returns netlink socket, or -1 on error
//  */
// static int nl_connect()
// {
//     int rc;
//     int nl_sock;
//     struct sockaddr_nl sa_nl;
// 
//     nl_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
//     if (nl_sock == -1) {
//         perror("socket");
//         return -1;
//     }
// 
//     sa_nl.nl_family = AF_NETLINK;
//     sa_nl.nl_groups = CN_IDX_PROC;
//     sa_nl.nl_pid = getpid();
// 
//     rc = bind(nl_sock, (struct sockaddr *)&sa_nl, sizeof(sa_nl));
//     if (rc == -1) {
//         perror("bind");
//         close(nl_sock);
//         return -1;
//     }
// 
//     return nl_sock;
// }
// 
// /*
//  * subscribe on proc events (process notifications)
//  */
// static int set_proc_ev_listen(int nl_sock, bool enable)
// {
//     int rc;
//     struct __attribute__ ((aligned(NLMSG_ALIGNTO))) {
//         struct nlmsghdr nl_hdr;
//         struct __attribute__ ((__packed__)) {
//             struct cn_msg cn_msg;
//             enum proc_cn_mcast_op cn_mcast;
//         };
//     } nlcn_msg;
// 
//     memset(&nlcn_msg, 0, sizeof(nlcn_msg));
//     nlcn_msg.nl_hdr.nlmsg_len = sizeof(nlcn_msg);
//     nlcn_msg.nl_hdr.nlmsg_pid = getpid();
//     nlcn_msg.nl_hdr.nlmsg_type = NLMSG_DONE;
// 
//     nlcn_msg.cn_msg.id.idx = CN_IDX_PROC;
//     nlcn_msg.cn_msg.id.val = CN_VAL_PROC;
//     nlcn_msg.cn_msg.len = sizeof(enum proc_cn_mcast_op);
// 
//     nlcn_msg.cn_mcast = enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE;
// 
//     rc = send(nl_sock, &nlcn_msg, sizeof(nlcn_msg), 0);
//     if (rc == -1) {
//         perror("netlink send");
//         return -1;
//     }
// 
//     return 0;
// }
// 
// /*
//  * handle a single process event
//  */
// static volatile bool need_exit = false;
// static int handle_proc_ev(int nl_sock)
// {
//     int rc;
//     struct __attribute__ ((aligned(NLMSG_ALIGNTO))) {
//         struct nlmsghdr nl_hdr;
//         struct __attribute__ ((__packed__)) {
//             struct cn_msg cn_msg;
//             struct proc_event proc_ev;
//         };
//     } nlcn_msg;
// 
//     while (!need_exit) {
//         rc = recv(nl_sock, &nlcn_msg, sizeof(nlcn_msg), 0);
//         if (rc == 0) {
//             /* shutdown? */
//             return 0;
//         } else if (rc == -1) {
//             if (errno == EINTR) continue;
//             perror("netlink recv");
//             return -1;
//         }
//         switch (nlcn_msg.proc_ev.what) {
//             case PROC_EVENT_NONE:
//                 printf("set mcast listen ok\n");
//                 break;
//             case PROC_EVENT_FORK:
//                 {
//                 char proc[256];
//                 int fd;
//                 sprintf(proc, "/proc/%d/cmdline", nlcn_msg.proc_ev.event_data.fork.child_pid);
//                 fd = open(proc, O_RDONLY);
//                 char cmd[1024];
//                 if(fd == -1)
//                 {
//                     strcpy(cmd, "[no cmdline]");
//                 }
//                 else
//                 {
//                     ssize_t r = read(fd, cmd, sizeof(cmd));
//                     if(r < 0)
//                     {
//                         strcpy(cmd, "[cmdline not accessible]");
//                     }
//                     else
//                     {
//                         if((size_t)r > sizeof(cmd) - 1)
//                         {
//                             r = (ssize_t)(sizeof(cmd) - 1);
//                         }
//                         cmd[r] = '\0';
//                     }
//                 }
//                 printf("fork: parent tid=%d pid=%d -> child tid=%d pid=%d -> %s\n",
//                         nlcn_msg.proc_ev.event_data.fork.parent_pid,
//                         nlcn_msg.proc_ev.event_data.fork.parent_tgid,
//                         nlcn_msg.proc_ev.event_data.fork.child_pid,
//                         nlcn_msg.proc_ev.event_data.fork.child_tgid,
//                         cmd);
//                 }
//                 break;
//             case PROC_EVENT_EXEC:
//                 printf("exec: tid=%d pid=%d\n",
//                         nlcn_msg.proc_ev.event_data.exec.process_pid,
//                         nlcn_msg.proc_ev.event_data.exec.process_tgid);
//                 break;
//             case PROC_EVENT_UID:
//                 printf("uid change: tid=%d pid=%d from %d to %d\n",
//                         nlcn_msg.proc_ev.event_data.id.process_pid,
//                         nlcn_msg.proc_ev.event_data.id.process_tgid,
//                         nlcn_msg.proc_ev.event_data.id.r.ruid,
//                         nlcn_msg.proc_ev.event_data.id.e.euid);
//                 break;
//             case PROC_EVENT_GID:
//                 printf("gid change: tid=%d pid=%d from %d to %d\n",
//                         nlcn_msg.proc_ev.event_data.id.process_pid,
//                         nlcn_msg.proc_ev.event_data.id.process_tgid,
//                         nlcn_msg.proc_ev.event_data.id.r.rgid,
//                         nlcn_msg.proc_ev.event_data.id.e.egid);
//                 break;
//             case PROC_EVENT_EXIT:
//                 printf("exit: tid=%d pid=%d exit_code=%d\n",
//                         nlcn_msg.proc_ev.event_data.exit.process_pid,
//                         nlcn_msg.proc_ev.event_data.exit.process_tgid,
//                         nlcn_msg.proc_ev.event_data.exit.exit_code);
//                 break;
//             default:
//                 printf("unhandled proc event\n");
//                 break;
//         }
//     }
// 
//     return 0;
// }
// 
// static void on_sigint(int unused)
// {
//     (void)unused;
//     need_exit = true;
// }
// 
// int main(int argc, const char *argv[])
// {
//     (void)argc;
//     (void)argv;
// 
//     int nl_sock;
//     int rc = EXIT_SUCCESS;
// 
//     signal(SIGINT, &on_sigint);
//     siginterrupt(SIGINT, true);
// 
//     nl_sock = nl_connect();
//     if (nl_sock == -1)
//         exit(EXIT_FAILURE);
// 
//     rc = set_proc_ev_listen(nl_sock, true);
//     if (rc == -1) {
//         rc = EXIT_FAILURE;
//         goto out;
//     }
// 
//     rc = handle_proc_ev(nl_sock);
//     if (rc == -1) {
//         rc = EXIT_FAILURE;
//         goto out;
//     }
// 
//     set_proc_ev_listen(nl_sock, false);
// 
// out:
//     close(nl_sock);
//     exit(rc);
// }
// 
// 
// 
// // sample output
// // 
// // $ sudo ./pmon
// // set mcast listen ok
// // fork: parent tid=2202 pid=2202 -> child tid=17595 pid=17595
// // exec: tid=17595 pid=17595
// // exec: tid=17595 pid=17595
// // exec: tid=17595 pid=17595
// // fork: parent tid=17595 pid=17595 -> child tid=17596 pid=17596
// // exec: tid=17596 pid=17596
// // exit: tid=17596 pid=17596 exit_code=0
// // fork: parent tid=17595 pid=17595 -> child tid=17597 pid=17597
// // exec: tid=17597 pid=17597
// // exit: tid=17597 pid=17597 exit_code=0
// // exit: tid=17595 pid=17595 exit_code=0
// // fork: parent tid=2202 pid=2202 -> child tid=17598 pid=17598
// // exec: tid=17598 pid=17598
// // exec: tid=17598 pid=17598
// // exec: tid=17598 pid=17598
// // fork: parent tid=17598 pid=17598 -> child tid=17599 pid=17599
// // exec: tid=17599 pid=17599
// // exit: tid=17599 pid=17599 exit_code=0
// // fork: parent tid=17598 pid=17598 -> child tid=17600 pid=17600
// // exec: tid=17600 pid=17600
// // exit: tid=17600 pid=17600 exit_code=0
// // exit: tid=17598 pid=17598 exit_code=0


} // namespace ed
// vim: ts=4 sw=4 et
