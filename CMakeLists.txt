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

cmake_minimum_required(VERSION 3.10.2)

project(eventdispatcher_project)

find_package(AdvGetOpt        REQUIRED)
find_package(AtomicNames      REQUIRED)
find_package(CppThread        REQUIRED)
find_package(LibAddr          REQUIRED)
find_package(LibExcept        REQUIRED)
find_package(LibUtf8          REQUIRED)
find_package(OpenSSL          REQUIRED)
find_package(ServerPlugins    REQUIRED)
find_package(SnapCMakeModules REQUIRED)
find_package(SnapDev          REQUIRED)
find_package(SnapLogger       REQUIRED)

SnapGetVersion(EVENTDISPATCHER ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(eventdispatcher   )
add_subdirectory(eventdispatcher_qt)
add_subdirectory(doc               )
add_subdirectory(cmake             )
add_subdirectory(scripts           )
add_subdirectory(tools             )
add_subdirectory(examples          )
add_subdirectory(snaplogger        )    # Extension to support a snaplogger server (remote logs)
add_subdirectory(cppprocess        )    # Extension to support a cppprocess implementation

add_subdirectory(tests             )

# vim: ts=4 sw=4 et
