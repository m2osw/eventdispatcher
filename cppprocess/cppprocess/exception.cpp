// Copyright (c) 2013-2025  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

/** \file
 * \brief Documentation of the exception.h file.
 *
 * The exception.h file defines classes 100% inline so we document them here.
 */

#error "Documentation only file, do not compile."

namespace cppprocess
{



// TODO: update to proper exceptions
/** \class cppprocess_logic_error
 * \brief The library detected an unexpected combination of events.
 *
 * This exception is raised when the library checks that things are in
 * order and detects that they aren't.
 */

/** \class cppprocess_out_of_range
 * \brief A value is out of range and can't be used as is.
 *
 * This exception is raised when a value is either too small or too large.
 */

/** \class cppprocess_exception
 * \brief To catch any thread exception, catch this base thread exception.
 *
 * This is the base thread exception for all the thread exceptions.
 * You may catch this exception to catch any of the thread exceptions.
 *
 * \warning
 * Some exceptions use the cppprocess_logic_error and the cppprocess_out_of_range
 * errors which are both not derived from the cppprocess_exception. In many
 * cases, these other two exceptions should not be processed like a runtime
 * error which the cppprocess_exception represents.
 */

/** \class cppprocess_already_exists
 * \brief An object already exists and can't be created again.
 *
 * Some object have to be unique within your entire process or at least within
 * a given collection. This error is generated whenever the library detects
 * that two of the same object are created in a row.
 */

/** \class cppprocess_in_use_error
 * \brief One thread runner can be attached to one thread.
 *
 * This exception is raised if a thread notices that a runner being
 * attached to it is already attached to another thread. This is just
 * not possible. One thread runner can only be running in one thread
 * not two or three (it has to stop and be removed from another thread
 * first otherwise.)
 */

/** \class cppprocess_invalid_error
 * \brief An invalid parameter or value was detected.
 *
 * This exception is raised when a parameter or a variable member or some
 * other value is out of range or generally not valid for its purpose.
 */

/** \class cppprocess_mutex_failed_error
 * \brief A mutex failed.
 *
 * In most cases, a mutex will fail if the input buffer is not considered
 * valid. (i.e. it was not initialized and it does not look like a mutex.)
 */

/** \class cppprocess_name_mismatch
 * \brief Two references to the same object used different names.
 *
 * This error occurs whenever an object detects two different names to
 * reference it or one of its children.
 */

/** \class cppprocess_not_locked_error
 * \brief A mutex cannot be unlocked if not locked.
 *
 * Each time we lock a mutex, we increase a counter. Each time we unlock a
 * mutex we decrease a counter. If you try to unlock when the counter is
 * zero, you have a lock/unlock discrepancy. This exception is raised
 * when such is discovered.
 */

/** \class cppprocess_not_locked_once_error
 * \brief When calling wait() the mutex should be locked once.
 *
 * When calling the wait() instruction, the mutex has to be locked once.
 *
 * At this time this is commented out as it caused problems. We probably
 * need to test that it is at least locked once and not exactly once.
 */

/** \class cppprocess_not_started
 * \brief Tried to start a thread and it failed.
 *
 * When using the thread_safe object which is created on the FIFO, one
 * guarantee is that the thread actually starts. If the threads cannot
 * be started, this exception is raised.
 */

/** \class cppprocess_system_error
 * \brief We called a system function and it failed.
 *
 * This exception is raised if a system function call fails.
 */




} // namespace cppprocess
// vim: ts=4 sw=4 et
