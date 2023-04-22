// Copyright (c) 2012-2023  Made to Order Software Corp.  All Rights Reserved
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
#include    "eventdispatcher_qt/qt_connection.h"


// eventdispatcher
//
#include    "eventdispatcher/exception.h"


// Qt
//
#include    <QX11Info>
#include    <QEventLoop>
#include    <QApplication>


// C
//
#include    <xcb/xcb.h>


// X11
//
#include    <X11/Xlib.h>


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief Implementation of the Event Dispatcher connection to support Qt.
 *
 * In order to run an application with both, eventdispatcher and Qt,
 * you need to use this connection to handle the Qt (X-Windows) events.
 *
 * This connection retrieves the Qt file descriptor that it can then use
 * along the `poll()` function as used by the communicator::run()
 * function.
 *
 * Also you can only create one such connection.
 *
 * \warning
 * We use a 100ms timer to act on the Qt events. If you try to use timers
 * with a greater precision, it will never work properly for you. We suggest
 * you look at using a thread for your eventdispatcher loop in such a
 * situation (i.e. if you're using OpenGL and expect realtime updates,
 * this class is definitely not a good solution).
 */

namespace ed
{
namespace
{


/** \brief A global variable to check unicity.
 *
 * We use this variable to make sure that you don't create two of
 * the ed::qt_connection since that would wreak havoc your application
 * anyway. It will throw implementation_error if
 * it happens.
 */
bool g_qt_communicator_created = false;


} // no name namespace



/** \class qt_connection
 * \brief Handle the Qt connection along the communicator.
 *
 * This class is used to handle the Qt connection along your other
 * connection objects. You can only create one of them. Attempt
 * to create a second one and it will throw an exception.
 *
 * The idea is pretty simple, you create the qt_connection and
 * add it as a connection to the communicator. Then call the
 * communicator::run() function instead of the Qt application
 * run() function. The messages will be executed by the
 * qt_connection instead.
 *
 * \warning
 * The class uses a timer with a 100ms increment. This is used to
 * make sure that all the events get executed. Without that, the
 * event loops require mouse movements or some other such X11
 * event to work and it's not good... One day we may find a fix
 * for this issue. In the meantime, if you can use threads, I
 * suggest you place your eventdispatcher loop in a thread and
 * call the app.exec() as usual on your main (GUI) thread.
 */



/////////////////////////////////////
// Snap Communicator Qt Connection //
/////////////////////////////////////



/** \brief Initializes the connection.
 *
 * This function initializes the Qt connection object.
 *
 * It gives it the name "qt". Since only one such object should exist
 * you should not have a problem with the name.
 *
 * \warning
 * The constructor and destructor of this connection make use of a
 * global flag without the use of a mutex. Since it is expected to
 * only be used by the GUI thread, we do not see much of an
 * inconvenience, but here we state that it can't be used by more
 * than one thread. In any event, you can't create more than one
 * Qt connection.
 *
 * \bug
 * The current implementation uses a 100ms timer which checks for
 * additional messages on a constant basis. This means your application
 * will not be sleeping when no events happen. If your application
 * can use threads, we suggest that you place the eventdispatcher loop
 * in a separate threads and do not use the qt_connection at all.
 * Then in your main thread, call the standard app.exec() function.
 * Obviously this means you now need mutexes to communicate between
 * the eventdispatcher callbacks and your GUI, but that should still
 * be preferable to having a timer that wakes up all the time for naught.
 */
qt_connection::qt_connection()
{
    if(g_qt_communicator_created)
    {
        throw implementation_error("you cannot create more than one qt_connection, make sure to delete the previous one before creating a new one (if you used a shared pointer, make sure to reset() first.)");
    }

    g_qt_communicator_created = true;

    set_name("qt");

    if(QX11Info::isPlatformX11())
    {
        Display * d(QX11Info::display());
        if(d != nullptr)
        {
            f_fd = XConnectionNumber(d);
        }
        else
        {
            xcb_connection_t * c(QX11Info::connection());
            if(c != nullptr)
            {
                f_fd = xcb_get_file_descriptor(c);
            }
        }
    }

    if(f_fd == -1)
    {
        throw no_connection_found("qt_connection was not able to find a file descriptor to poll() on");
    }

    // Qt has many internal functionality which doesn't get awaken by
    // the X11 socket so we have always be checking for messages...
    //
    set_timeout_delay(100'000);
}


/** \brief Proceed with the cleanup of the qt_connection.
 *
 * This function cleans up a qt_connection object.
 *
 * After this call, you can create a new qt_connection again.
 */
qt_connection::~qt_connection()
{
    g_qt_communicator_created = false;
}


/** \brief Retrieve the X11 socket.
 *
 * This function returns the X11 socket. It may return -1 although by
 * default if we cannot determine the socket we fail with an exception.
 *
 * \return The file descriptor of the Qt connection.
 */
int qt_connection::get_socket() const
{
    return f_fd;
}


/** \brief The X11 pipe is only a reader for us.
 *
 * The X11 pipe is a read/write pipe, but we don't handle the write,
 * only the read. So the connection is only viewed as a reader here.
 *
 * The X11 protocol is such that we won't have a read and/or write
 * problem that will block us so we'll be fine.
 *
 * \return Return true so we listen for X11 incoming messages.
 */
bool qt_connection::is_reader() const
{
    return true;
}


/** \brief The timer kicked in.
 *
 * The X11 socket is not used by all the Qt messages (to the contrary, most
 * events don't use any of the OS windowing system mechanism). So at this
 * point we have to use a timer to constantly check for more messages. This
 * is not ideal, though.
 *
 * If your application is able to make use of threads, you may want to run
 * the eventdispatcher loop in a thread and not make use of the qt_connection
 * at all. Then use the normal app.exec() function from Qt. This will make
 * the loops much cleaner (i.e. no timer wasting time every 100ms). This
 * solution is good if do not want to deal with multiple thread and your
 * application is not expected to be running 24/7.
 *
 * \note
 * If your application is to display things that need constant updating such
 * as statistics, then having a timer like so can be to your advantage.
 * Instead of using a Qt timer, your functions can just request an update
 * of your widget viewport and it will refresh as expected.
 *
 * \note
 * The main reason for not using threads is because your application uses
 * fork() without exec() (i.e. a fork() + exec() is safe, a fork() + work
 * means that this sub-process state in invalid because only one task gets
 * forked and all the other threads are _dangling_.)
 */
void qt_connection::process_timeout()
{
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
}


/** \brief At least one X11 event was received.
 *
 * This function is called whenever X11 sent a message to your
 * application. It calls the necessary Qt functions to process it.
 */
void qt_connection::process_read()
{
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
}





} // namespace ed
// vim: ts=4 sw=4 et
