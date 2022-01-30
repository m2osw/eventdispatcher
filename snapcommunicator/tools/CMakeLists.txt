# Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


###############################################################################
## Snap send signals
##
project(snapsignal)

add_executable(${PROJECT_NAME}
    snapsignal.cpp
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


###############################################################################
## A tool used mainly to help debug sending/receiving messages between services
##
project(snapmessage)

add_executable(${PROJECT_NAME}
    snapmessage.cpp
)

target_link_libraries(${PROJECT_NAME}
    eventdispatcher
    readline
    ${LIBTLD_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)


#################################################################################
## Snap send SIGINT to stop a daemon
## (TODO: make all our daemon work with the default systemctl stop command)
##
project(snapstop)

add_executable(${PROJECT_NAME}
    snapstop.cpp
)

target_link_libraries(${PROJECT_NAME}
    eventdispatcher
    ${ADVGETOPT_LIBRARIES}
    ${LIBTLD_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)


#################################################################################
## Get snapcommunicator cluster status (UP/DOWN and/or COMPLETE/INCOMPLETE)
## TODO: put that status in out Fluid Settings
##
project(snapclusterstatus)

add_executable(${PROJECT_NAME}
    snapclusterstatus.cpp
)

target_link_libraries(${PROJECT_NAME}
    eventdispatcher
    ${LIBTLD_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)


# vim: ts=4 sw=4 et
