// Copyright (c) 2013-2024  Made to Order Software Corp.  All Rights Reserved
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

/** \brief Test to verify that signalfd() prevents EINT errors.
 *
 * When running a software under Unix, you can send it signals. As a result,
 * functions may return an error with errno set to EINT. In really large
 * software, it is really difficult to make sure that all such interrupts
 * are properly handled. Therefore, we tend not to use them. Also with
 * a GRPC type of system (eventdistpatcher), it is not extermely useful
 * (i.e. we can just send a message asking the process to quit, etc.).
 *
 * This test was written to show that sending a signal that was first
 * setup to be captured by the signalfd() and properly masked does not
 * resultin an EINT error in poll().
 *
 * First make sure the test was compiled, them run it with one of the
 * signals to be sent:
 *
 * \code
 * ../../BUILD/Debug/contrib/eventdispatcher/tests/check_signal_and_eint --usr1
 * \endcode
 *
 * Note that you can then find that process and further send signals to it
 * using the kill command.
 *
 * \code
 * $ ps -ef | grep [c]heck_signal_and_eint
 * alexis    204574   11718  0 12:56 pts/4    00:00:00 ../../BUILD/Debug/contrib/eventdispatcher/tests/check_signal_and_eint --usr1
 * $ kill -USR2 204574
 * \endcode
 *
 * This way you can test all three of the captured signals and see that
 * the poll() function never returns with an EINT error.
 *
 * To terminate, send the process a QUIT or TERM signal. With your keyboard,
 * use Ctrl-\ for a QUIT. With the `kill` command use the `-TERM` 
 */

// C++
//
#include    <cstring>
#include    <iostream>
#include    <thread>


// C
//
#include    <poll.h>
#include    <signal.h>
#include    <sys/signalfd.h>


void usage()
{
    std::cout << "Usage: check_signal_and_eint <opts>\n";
    std::cout << "Where <opts> is one or more of:\n";
    std::cout << "  -h | --help     print out this help screen\n";
    std::cout << "  --int           send a SIGINT\n";
    std::cout << "  --usr1          send a SIGUSR1\n";
    std::cout << "  --usr2          send a SIGUSR2\n";
}


int capture_signal(int sig)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);
    if(sigprocmask(SIG_BLOCK, &set, nullptr) != 0)
    {
        std::cerr << "sigprocmask() failed to block signals.";
        return -1;
    }

    int s = signalfd(-1, &set, SFD_NONBLOCK | SFD_CLOEXEC);
    if(s == -1)
    {
        std::cerr << "signalfd() failed with signal " << sig << "\n";
        return -1;
    }

    return s;
}


void emit_signal(int sig)
{
    sleep(1);

    kill(getpid(), sig);
}


int main(int argc, char * argv[])
{
    int sig = SIGTERM;

    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "-h") == 0
        || strcmp(argv[i], "--help") == 0)
        {
            usage();
            return 1;
        }
        else if(strcmp(argv[i], "--int") == 0)
        {
            sig = SIGINT;
        }
        else if(strcmp(argv[i], "--usr1") == 0)
        {
            sig = SIGUSR1;
        }
        else if(strcmp(argv[i], "--usr2") == 0)
        {
            sig = SIGUSR2;
        }
        else
        {
            std::cerr << "error: unknown command line option \""
                << argv[i]
                << "\".\n";
            return 1;
        }
    }

    int s_int(capture_signal(SIGINT));
    int s_usr1(capture_signal(SIGUSR1));
    int s_usr2(capture_signal(SIGUSR2));

    for(int count(1);; ++count)
    {
        struct pollfd fds[3] =
        {
            {
                .fd = s_int,
                .events = POLLIN,
                .revents = 0,
            },
            {
                .fd = s_usr1,
                .events = POLLIN,
                .revents = 0,
            },
            {
                .fd = s_usr2,
                .events = POLLIN,
                .revents = 0,
            },
        };

        std::thread t(emit_signal, sig);

        std::cout << "--- " << count << ". poll for " << sig << "\n";
        int r = poll(fds, 3, 5000);
        int e(errno);

        t.join();

        std::cerr << "poll() returned with " << r << "\n";
        if(r < 0)
        {
            std::cerr << "  errno = " << e
                << ", "
                << strerror(e)
                << "\n";
        }
        else
        {
            if(fds[0].revents != 0)
            {
                signalfd_siginfo info;
                r = read(s_int, &info, sizeof(info));
                std::cout << "--- got SIGINT\n";
            }
            if(fds[1].revents != 0)
            {
                signalfd_siginfo info;
                r = read(s_usr1, &info, sizeof(info));
                std::cout << "--- got SIGUSR1\n";
            }
            if(fds[2].revents != 0)
            {
                signalfd_siginfo info;
                r = read(s_usr2, &info, sizeof(info));
                std::cout << "--- got SIGUSR2\n";
            }
        }
    }

    return 0;
}

// vim: ts=4 sw=4 et
