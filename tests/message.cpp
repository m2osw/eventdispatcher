// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// self
//
#include    "main.h"


// snaplogger lib
//
#include    <eventdispatcher/message.h>


// C lib
//
#include    <unistd.h>



CATCH_TEST_CASE("message", "[message]")
{
    CATCH_START_SECTION("Simple message fields")
    {
        ed::message msg;

        // SENT FROM SERVER
        //
        CATCH_REQUIRE(msg.get_sent_from_server().empty());
        msg.set_sent_from_server("remote");
        CATCH_REQUIRE(msg.get_sent_from_server() == "remote");

        // SENT FROM SERVICE
        //
        CATCH_REQUIRE(msg.get_sent_from_service().empty());
        msg.set_sent_from_service("firewall");
        CATCH_REQUIRE(msg.get_sent_from_service() == "firewall");

        // SERVER
        //
        CATCH_REQUIRE(msg.get_server().empty());
        msg.set_server("jungle");
        CATCH_REQUIRE(msg.get_server() == "jungle");

        // SERVICE
        //
        CATCH_REQUIRE(msg.get_service().empty());
        msg.set_service("watchdog");
        CATCH_REQUIRE(msg.get_service() == "watchdog");

        // COMMAND
        //
        CATCH_REQUIRE(msg.get_command().empty());
        msg.set_command("CONNECT");
        CATCH_REQUIRE(msg.get_command() == "CONNECT");

        // MESSAGE VERSION
        //
        CATCH_REQUIRE(msg.get_message_version() == ed::MESSAGE_VERSION);
        msg.add_version_parameter();
        CATCH_REQUIRE(msg.check_version_parameter());

        // PARAMETER (race)
        //
        CATCH_REQUIRE_FALSE(msg.has_parameter("race"));
        msg.add_parameter("race", "true");
        CATCH_REQUIRE(msg.has_parameter("race"));
        CATCH_REQUIRE(msg.get_parameter("race") == "true");

        // PARAMETER (speed)
        //
        CATCH_REQUIRE_FALSE(msg.has_parameter("speed"));
        msg.add_parameter("speed", 1078);
        CATCH_REQUIRE(msg.has_parameter("speed"));
        CATCH_REQUIRE(msg.get_parameter("speed") == "1078");
        CATCH_REQUIRE(msg.get_integer_parameter("speed") == 1078);

        // PARAMETER (height)
        //
        CATCH_REQUIRE_FALSE(msg.has_parameter("height"));
        msg.add_parameter("height", 27U);
        CATCH_REQUIRE(msg.has_parameter("height"));
        CATCH_REQUIRE(msg.get_parameter("height") == "27");
        CATCH_REQUIRE(msg.get_integer_parameter("height") == 27);

        // PARAMETER (huge)
        //
        CATCH_REQUIRE_FALSE(msg.has_parameter("huge"));
        msg.add_parameter("huge", 7428447997487423361LL);
        CATCH_REQUIRE(msg.has_parameter("huge"));
        CATCH_REQUIRE(msg.get_parameter("huge") == "7428447997487423361");
        CATCH_REQUIRE(msg.get_integer_parameter("huge") == 7428447997487423361LL);

        // PARAMETER (huge #2)
        //
        CATCH_REQUIRE_FALSE(msg.has_parameter("huge2"));
        msg.add_parameter("huge2", 7428447997487423961ULL);
        CATCH_REQUIRE(msg.has_parameter("huge2"));
        CATCH_REQUIRE(msg.get_parameter("huge2") == "7428447997487423961");
        CATCH_REQUIRE(msg.get_integer_parameter("huge2") == 7428447997487423961ULL);

        // PARAMETER (a64bit)
        //
        std::int64_t const a64bit = 7428447907487423361LL;
        CATCH_REQUIRE_FALSE(msg.has_parameter("a64bit"));
        msg.add_parameter("a64bit", a64bit);
        CATCH_REQUIRE(msg.has_parameter("a64bit"));
        CATCH_REQUIRE(msg.get_parameter("a64bit") == "7428447907487423361");
        CATCH_REQUIRE(msg.get_integer_parameter("a64bit") == a64bit);

        // PARAMETER (u64bit)
        //
        std::uint64_t const u64bit = 428447907487423361UL;
        CATCH_REQUIRE_FALSE(msg.has_parameter("u64bit"));
        msg.add_parameter("u64bit", u64bit);
        CATCH_REQUIRE(msg.has_parameter("u64bit"));
        CATCH_REQUIRE(msg.get_parameter("u64bit") == "428447907487423361");
        CATCH_REQUIRE(msg.get_integer_parameter("u64bit") == u64bit);

        for(auto p : msg.get_all_parameters())
        {
            if(p.first == "huge")
            {
                CATCH_REQUIRE(p.second == "7428447997487423361");
            }
            else if(p.first == "u64bit")
            {
                CATCH_REQUIRE(p.second == "428447907487423361");
            }
        }

        // REPLY TO
        //
        ed::message msg2;
        msg2.reply_to(msg);

        CATCH_REQUIRE(msg2.get_sent_from_server().empty());
        CATCH_REQUIRE(msg2.get_sent_from_service().empty());
        CATCH_REQUIRE(msg2.get_server() == "remote");
        CATCH_REQUIRE(msg2.get_service() == "firewall");
        CATCH_REQUIRE(msg2.get_command().empty());
        CATCH_REQUIRE(msg2.get_message_version() == ed::MESSAGE_VERSION);
        CATCH_REQUIRE_FALSE(msg2.has_parameter("race"));
        CATCH_REQUIRE_FALSE(msg2.has_parameter("speed"));
        CATCH_REQUIRE_FALSE(msg2.has_parameter("height"));
        CATCH_REQUIRE_FALSE(msg2.has_parameter("huge"));
        CATCH_REQUIRE_FALSE(msg2.has_parameter("huge2"));
        CATCH_REQUIRE_FALSE(msg2.has_parameter("a64bit"));
        CATCH_REQUIRE_FALSE(msg2.has_parameter("u64bit"));
        CATCH_REQUIRE(msg2.get_all_parameters().empty());

//for(auto p : msg2.get_all_parameters())
//{
//std::cerr << "--- " << p.first << "=" << p.second << "\n";
//}

        // make sure the original wasn't modified
        //
        CATCH_REQUIRE(msg.get_sent_from_server() == "remote");
        CATCH_REQUIRE(msg.get_sent_from_service() == "firewall");
        CATCH_REQUIRE(msg.get_server() == "jungle");
        CATCH_REQUIRE(msg.get_service() == "watchdog");
    }
    CATCH_END_SECTION()

    // TODO:
    //bool                    from_message(std::string const & msg);
    //std::string             to_message() const;
    //void                    reply_to(message const & msg);
    //parameters_t const &    get_all_parameters() const;
}


// vim: ts=4 sw=4 et
