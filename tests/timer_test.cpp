// Copyright (c) 2013-2025  Made to Order Software Corp.  All Rights Reserved
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

// eventdispatcher
//
#include    <eventdispatcher/timer.h>
#include    <eventdispatcher/communicator.h>


// last include
//
#include    <snapdev/poison.h>




class timer_test
    : public ed::timer
{
public:
                        timer_test(std::int64_t timeout_us);
                        timer_test(timer_test const &) = delete;
    timer_test const &  operator = (timer_test const &) = delete;

    // process_timeout implementation
    //
    virtual void        process_timeout() override;

private:
};


timer_test::timer_test(std::int64_t timeout_us)
    : timer(timeout_us)
{
}


void timer_test::process_timeout()
{
    std::cout << "--- process timeout\n";
    snapdev::timespec_ex const now(snapdev::now());
    std::cerr << now.to_string() << "\n";
}


int main(int argc, char * argv[])
{
    char const * v(nullptr);
    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "--help") == 0
        || strcmp(argv[i], "-h") == 0)
        {
            std::cout << "Usage: timer-test [-h|--help] [-i <value>|--interval[=| ]<value>]\n";
            std::cout << "where an interval is defined as a number of micro seconds.\n";
            return 1;
        }
        else if(strncmp(argv[i], "--interval", 10) == 0)
        {
            if(v != nullptr)
            {
                std::cerr << "error: interval is already defined.\n";
                return 1;
            }
            if(argv[i][10] == '=')
            {
                // the value is just after
                //
                v = argv[i] + 11;
            }
            else if(i + 1 >= argc)
            {
                std::cerr << "error: value missing after --interval.\n";
                return 1;
            }
            else
            {
                ++i;
                v = argv[i];
            }
        }
        else if(strcmp(argv[i], "-i") == 0)
        {
            if(v != nullptr)
            {
                std::cerr << "error: interval is already defined.\n";
                return 1;
            }
            else if(i + 1 >= argc)
            {
                std::cerr << "error: value missing after -i.\n";
                return 1;
            }
            else
            {
                ++i;
                v = argv[i];
            }
        }
        else
        {
            std::cerr << "error: unknown command line option \""
                << argv[i]
                << "\".\n";
            return 1;
        }
    }

    std::int64_t time_us(0);
    if(v != nullptr)
    {
        for(; *v != '\0'; ++v)
        {
            if(*v < '0' || *v > '9')
            {
                std::cerr << "error: interval must be a positive decimal value.\n";
                return 1;
            }
            time_us *= 10;
            time_us += *v - '0';
        }
    }
    else
    {
        time_us = 1'000'000;
    }

    timer_test::pointer_t timer(std::make_shared<timer_test>(time_us));

    ed::communicator::pointer_t communicator(ed::communicator::instance());
    communicator->add_connection(timer);

    communicator->run();

    return 0;
}

// vim: ts=4 sw=4 et
