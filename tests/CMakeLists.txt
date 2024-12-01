# Copyright (c) 2013-2024  Made to Order Software Corp.  All Rights Reserved
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

##
## eventdispatcher unittest
##
find_package(SnapCatch2)
if(SnapCatch2_FOUND)
    project(unittest)

    add_executable(${PROJECT_NAME}
        catch_main.cpp

        catch_dispatcher.cpp
        catch_file_changed.cpp
        catch_message.cpp
        catch_process.cpp
        catch_process_info.cpp
        catch_signal_handler.cpp
        catch_timer.cpp
        catch_unix_dgram.cpp
        catch_unix_stream.cpp
        catch_version.cpp

        catch_reporter_executor.cpp
        catch_reporter_expression.cpp
        catch_reporter_instruction.cpp
        catch_reporter_lexer.cpp
        catch_reporter_parser.cpp
        catch_reporter_statement.cpp
        catch_reporter_token.cpp
    )

    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${CMAKE_SOURCE_DIR}/cppprocess
            ${CMAKE_BINARY_DIR}/cppprocess
            ${CMAKE_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}
            ${SNAPCATCH2_INCLUDE_DIRS}
            ${LIBEXCEPT_INCLUDE_DIRS}
            ${LIBUTF8_INCLUDE_DIRS}
    )

    target_link_libraries(${PROJECT_NAME}
        eventdispatcher
        reporter
        cppprocess
        ${SNAPCATCH2_LIBRARIES}
    )




    project(check_signal_and_eint)

    add_executable(${PROJECT_NAME}
        check_signal_and_eint.cpp
    )

    target_include_directories(${PROJECT_NAME}
        PUBLIC
            ${CMAKE_SOURCE_DIR}/cppprocess
            ${CMAKE_BINARY_DIR}/cppprocess
            ${CMAKE_BINARY_DIR}
            ${PROJECT_SOURCE_DIR}
            ${SNAPCATCH2_INCLUDE_DIRS}
            ${LIBEXCEPT_INCLUDE_DIRS}
            ${LIBUTF8_INCLUDE_DIRS}
    )

    target_link_libraries(${PROJECT_NAME}
        eventdispatcher
        #reporter
        cppprocess
        #${SNAPCATCH2_LIBRARIES}
    )

else(SnapCatch2_FOUND)

    message("snapcatch2 not found... no tests will be built.")

endif(SnapCatch2_FOUND)

if(SnapCatch2_FOUND)

    find_package(SnapTestRunner)
    AddUnitTestsTarget(
        PROJECT_NAME
            rununittests
    )

endif(SnapCatch2_FOUND)


##
## Verify expected compile time errors
##
set(CHECK_INCLUDES
    ${CMAKE_SOURCE_DIR}
    ${LIBEXCEPT_INCLUDE_DIRS}
)
list(TRANSFORM CHECK_INCLUDES PREPEND "-I")

try_compile(
    COMPILED_DISPATCHER_WITHOUT_COMMAND
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/tests/check_dispatcher_without_command.cpp

    COMPILE_DEFINITIONS
        ${CMAKE_REQUIRED_DEFINITIONS}
        ${CHECK_INCLUDES}
    CMAKE_FLAGS
        -DCOMPILE_DEFINITIONS:STRING=${CMAKE_REQUIRED_FLAGS}
    CXX_STANDARD
        23
    CXX_EXTENSIONS
        TRUE
    OUTPUT_VARIABLE
        OUTPUT)

if(${COMPILED_DISPATCHER_WITHOUT_COMMAND} OR NOT "${OUTPUT}" MATCHES "no matching function for call to.*find_match_value.*ed::Callback")
    message(FATAL_ERROR "ed::define_match() worked with without an ed::Callback().")
endif()


##
## Process Listener
##
project(process-listener)

add_executable(${PROJECT_NAME}
    process_listener.cpp
)

target_link_libraries(${PROJECT_NAME}
    eventdispatcher
)

# vim: ts=4 sw=4 et
