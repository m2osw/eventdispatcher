# - Find Reporter
#
# REPORTER_FOUND        - System has Reporter Catch2 extension library
# REPORTER_INCLUDE_DIRS - The Reporter include directories
# REPORTER_LIBRARIES    - The libraries needed to use Reporter
# REPORTER_DEFINITIONS  - Compiler switches required for using Reporter
#
# License:
#
# Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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

find_path(
    REPORTER_INCLUDE_DIR
        eventdispatcher/reporter/executor.h

    PATHS
        ENV REPORTER_INCLUDE_DIR
)

find_library(
    REPORTER_LIBRARY
        reporter

    PATHS
        ${REPORTER_LIBRARY_DIR}
        ENV REPORTER_LIBRARY
)

mark_as_advanced(
    REPORTER_INCLUDE_DIR
    REPORTER_LIBRARY
)

set(REPORTER_INCLUDE_DIRS ${REPORTER_INCLUDE_DIR})
set(REPORTER_LIBRARIES    ${REPORTER_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Reporter
    REQUIRED_VARS
        REPORTER_INCLUDE_DIR
        REPORTER_LIBRARY
)

# vim: ts=4 sw=4 et
