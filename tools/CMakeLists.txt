# Copyright (c) 2012-2020  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/eventdispatcher
# contact@m2osw.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

##
## ed-signal
##
project(ed-signal)

add_executable(${PROJECT_NAME}
    ed_signal.cpp
)

include_directories(
    ${ADVGETOPT_INCLUDE_DIRS}
    #${LIBEXCEPT_INCLUDE_DIRS}
    #${LIBUFTF8_INCLUDE_DIRS}
    #${SNAPLOGGER_INCLUDE_DIRS}
    #${OPENSSL_INCLUDE_DIR}
    #${LIBADDR_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    eventdispatcher
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)


# vim: ts=4 sw=4 et