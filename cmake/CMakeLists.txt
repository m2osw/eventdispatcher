# Copyright (c) 2013-2025  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/eventdispatcher
# contact@m2osw.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

project(eventdispatcher_cmake)

install(
    FILES
        EventDispatcherConfig.cmake

    DESTINATION
        share/cmake/EventDispatcher
)

install(
    FILES
        EventDispatcherQtConfig.cmake

    DESTINATION
        share/cmake/EventDispatcherQt
)

install(
    FILES
        ReporterConfig.cmake

    DESTINATION
        share/cmake/Reporter
)

install(
    FILES
        CppProcessConfig.cmake

    DESTINATION
        share/cmake/CppProcess
)

install(
    FILES
        SnapLoggerNetworkConfig.cmake

    DESTINATION
        share/cmake/SnapLoggerNetwork
)

install(
    FILES
        VerifyMessageDefinitionsConfig.cmake

    DESTINATION
        share/cmake/VerifyMessageDefinitions
)

# vim: ts=4 sw=4 et nocindent
