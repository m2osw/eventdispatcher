// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Implementation of the permanent TCP connection.
 *
 * This class is an extension of the TCP client message connection used
 * to handle a TCP connection which supports messages and will automatically
 * try to reconnect if the connection drops.
 *
 * The class will also try to connect with the next address if more than one
 * is available.
 */


// self
//
#include    "eventdispatcher/tcp_client_permanent_message_connection.h"

#include    "eventdispatcher/communicator.h"
#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/tcp_server_client_message_connection.h"
#include    "eventdispatcher/thread_done_signal.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_used.h>


// cppthread
//
#include    <cppthread/exception.h>
#include    <cppthread/guard.h>
#include    <cppthread/runner.h>
#include    <cppthread/thread.h>


// C++
//
#include    <cstring>


// C
//
#include    <sys/socket.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



namespace detail
{


/** \brief Internal implementation of the tcp_client_permanent_message_connection class.
 *
 * This class is used to handle a thread that will process a connection for
 * us. This allows us to connect in any amount of time required by the
 * Unix system to obtain the connection with the remote server.
 *
 * \todo
 * Having threads at the time we do a fork() is not safe. We may
 * want to reconsider offering this functionality here. Because at
 * this time we would have no control of when the thread is created
 * and thus a way to make sure that no such thread is running when
 * we call fork().
 */
class tcp_client_permanent_message_connection_impl
{
public:
    class messenger
        : public tcp_server_client_message_connection
    {
    public:
        typedef std::shared_ptr<messenger>      pointer_t;

        messenger(tcp_client_permanent_message_connection * parent, tcp_bio_client::pointer_t client)
            : tcp_server_client_message_connection(client)
            , f_parent(parent)
        {
            set_name("tcp_client_permanent_message_connection_impl::messenger");
        }

        messenger(messenger const & rhs) = delete;
        messenger & operator = (messenger const & rhs) = delete;

        // connection implementation
        virtual void process_empty_buffer()
        {
            tcp_server_client_message_connection::process_empty_buffer();
            f_parent->process_empty_buffer();
        }

        // connection implementation
        virtual void process_error()
        {
            tcp_server_client_message_connection::process_error();
            f_parent->process_error();
        }

        // connection implementation
        virtual void process_hup()
        {
            tcp_server_client_message_connection::process_hup();
            f_parent->process_hup();
        }

        // connection implementation
        virtual void process_invalid()
        {
            tcp_server_client_message_connection::process_invalid();
            f_parent->process_invalid();
        }

        // tcp_server_client_message_connection implementation
        virtual void process_message(message & msg)
        {
            // We call the dispatcher from our parent since the child
            // (this messenger) is not given a dispatcher
            //
            f_parent->dispatch_message(msg);
        }

    private:
        tcp_client_permanent_message_connection *  f_parent = nullptr;
    };

    class thread_signal_handler
        : public thread_done_signal
    {
    public:
        typedef std::shared_ptr<thread_signal_handler>   pointer_t;

        thread_signal_handler(tcp_client_permanent_message_connection_impl * parent_impl)
            : f_parent_impl(parent_impl)
        {
            set_name("tcp_client_permanent_message_connection_impl::thread_signal_handler");
        }

        thread_signal_handler(thread_signal_handler const &) = delete;
        thread_signal_handler & operator = (thread_signal_handler const &) = delete;

        /** \brief This signal was emitted.
         *
         * This function gets called whenever the thread is just about to
         * quit. Calling f_thread.is_running() may still return true when
         * you get in the 'thread_done()' callback. However, an
         * f_thread.stop() will return very quickly.
         */
        virtual void process_read()
        {
            thread_done_signal::process_read();

            f_parent_impl->thread_done();
        }

    private:
        tcp_client_permanent_message_connection_impl *  f_parent_impl = nullptr;
    };

    class runner
        : public cppthread::runner
    {
    public:
        runner(
                      tcp_client_permanent_message_connection_impl * parent_impl
                    , addr::addr::vector_t const & addresses
                    , mode_t mode)
            : cppthread::runner("background tcp_client_permanent_message_connection for asynchronous connections")
            , f_parent_impl(parent_impl)
            , f_addresses(addresses)
            , f_mode(mode)
        {
        }

        runner(runner const &) = delete;
        runner & operator = (runner const &) = delete;


        /** \brief This is the actual function run by the thread.
         *
         * This function calls the connect() function and then
         * tells the main thread we are done.
         */
        virtual void run()
        {
            connect();

            // tell the main thread that we are done
            //
            f_parent_impl->trigger_thread_done();
        }


        /** \brief This function attempts to connect.
         *
         * This function attempts a connection to the specified address
         * and port with the specified mode (i.e. plain or encrypted.)
         *
         * The function may take a long time to succeed connecting with
         * the server. The main thread will be awaken whenever this
         * thread dies.
         *
         * If an error occurs, then the f_socket variable member will
         * be set to -1. Otherwise it represents the socket that we
         * just connected with.
         */
        void connect()
        {
            char const * error_name(nullptr);
            try
            {
                // create a socket using the bio_client class,
                // but then just create a duplicate that we will
                // use in a server-client TCP object (because
                // we cannot directly create the right type of
                // object otherwise...)
                //
                f_tcp_connection = std::make_shared<tcp_bio_client>(f_addresses[f_index], f_mode);
                return;
            }
            catch(failed_connecting const & e)
            {
                error_name = "ed::failed_connecting";
                f_last_error = e.what();
            }
            catch(initialization_error const & e)
            {
                error_name = "ed::initialization_error";
                f_last_error = e.what();
            }
            catch(runtime_error const & e)
            {
                error_name = "ed::runtime_error";
                f_last_error = e.what();
            }
            catch(std::exception const & e)
            {
                error_name = "std::exception";
                f_last_error = e.what();
            }
            catch(...)
            {
                error_name = "a non-standard exception";
                f_last_error = "Unknown exception";
            }
            f_tcp_connection.reset();

            // on an error, we want to try the next address
            //
            ++f_index;
            if(f_index >= f_addresses.size())
            {
                f_index = 0;
            }

            // connection failed... we will have to try again later
            //
            SNAP_LOG_ERROR
                << "connection to "
                << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT)
                << f_addresses
                << " failed with: "
                << f_last_error
                << " ("
                << error_name
                << ")."
                << SNAP_LOG_SEND;
        }


        /** \brief Retrieve the address to connect to.
         *
         * This function returns the address we connected to or, if not
         * connected, the one that we will attempt to connect to next.
         *
         * \note
         * Since the variable is constant, it is likely to never change.
         * However, the c_str() function may change the buffer pointer.
         * Hence, to be 100% safe, you cannot call this function until
         * you make sure that the thread is fully stopped.
         *
         * \return The destination address.
         */
        addr::addr const & get_address() const
        {
            return f_addresses[f_index];
        }


        /** \brief Retrieve the vector of addresses to connect to.
         *
         * This function returns the addresses passed in on creation.
         *
         * \note
         * Since the variable is constant, it is likely to never change.
         * However, the c_str() function may change the buffer pointer.
         * Hence, to be 100% safe, you cannot call this function until
         * you make sure that the thread is fully stopped.
         *
         * \return The destination addresses.
         */
        addr::addr::vector_t const & get_addresses() const
        {
            return f_addresses;
        }


        /** \brief Retrieve the client allocated and connected by the thread.
         *
         * This function returns the TCP connection object resulting from
         * connection attempts of the background thread.
         *
         * If the pointer is null, then you may get the corresponding
         * error message using the get_last_error() function.
         *
         * You can get the client TCP connection pointer once. After that
         * you always get a null pointer.
         *
         * \note
         * This function is guarded so the pointer and the object it
         * points to will be valid in another thread that retrieves it.
         *
         * \return The connection pointer.
         */
        tcp_bio_client::pointer_t release_client()
        {
            cppthread::guard g(f_mutex);
            tcp_bio_client::pointer_t tcp_connection;
            tcp_connection.swap(f_tcp_connection);
            return tcp_connection;
        }


        /** \brief Retrieve the last error message that happened.
         *
         * This function returns the last error message that was captured
         * when trying to connect to the socket. The message is the
         * e.what() message from the exception we captured.
         *
         * The message does not get cleared so the function can be called
         * any number of times. To know whether an error was generated
         * on the last attempt, make sure to first get the get_socket()
         * and if it returns -1, then this message is significant,
         * otherwise it is from a previous error.
         *
         * \warning
         * Remember that if the background thread was used the error will
         * NOT be available in the main thread until a full memory barrier
         * was executed. For that reason we make sure that the thread
         * was stopped when we detect an error.
         *
         * \return The last error message.
         */
        std::string const & get_last_error() const
        {
            return f_last_error;
        }


        /** \brief Close the connection.
         *
         * This function closes the connection. Since the f_tcp_connection
         * holds the socket to the remote server, we have get this function
         * called in order to completely disconnect.
         *
         * \note
         * This function does not clear the f_last_error parameter so it
         * can be read later.
         */
        void close()
        {
            f_tcp_connection.reset();
        }


    private:
        tcp_client_permanent_message_connection_impl *  f_parent_impl = nullptr;
        std::size_t                                     f_index = 0;
        addr::addr::vector_t const                      f_addresses;
        mode_t const                                    f_mode;
        tcp_bio_client::pointer_t                       f_tcp_connection = tcp_bio_client::pointer_t();
        std::string                                     f_last_error = std::string();
    };


    /** \brief Initialize a permanent message connection implementation object.
     *
     * This object manages the thread used to asynchronically connect to
     * the specified address and port.
     *
     * This class and its sub-classes may end up executing callbacks
     * of the tcp_client_permanent_message_connection object.
     * However, in all cases these are never run from the thread.
     *
     * \param[in] parent  A pointer to the owner of this
     * tcp_client_permanent_message_connection_impl object.
     * \param[in] addresses  The addresses we are to connect to.
     * \param[in] mode  The mode used to connect.
     */
    tcp_client_permanent_message_connection_impl(
                  tcp_client_permanent_message_connection * parent
                , addr::addr::vector_t const & addresses
                , mode_t mode)
        : f_parent(parent)
        , f_thread_runner(this, addresses, mode)
        , f_thread("background connection handler thread", &f_thread_runner)
    {
    }


    tcp_client_permanent_message_connection_impl(tcp_client_permanent_message_connection_impl const & rhs) = delete;
    tcp_client_permanent_message_connection_impl & operator = (tcp_client_permanent_message_connection_impl const & rhs) = delete;

    /** \brief Destroy the permanent message connection.
     *
     * This function makes sure that the messenger was lost.
     */
    ~tcp_client_permanent_message_connection_impl()
    {
        // to make sure we can lose the messenger, first we want to be sure
        // that we do not have a thread running
        //
        try
        {
            f_thread.stop();
        }
        catch(cppthread::mutex_failed_error const &)
        {
        }
        catch(cppthread::invalid_error const &)
        {
        }

        // in this case we may still have an instance of the f_thread_done
        // which linger around, we want it out
        //
        // Note: the call is safe even if the f_thread_done is null
        //
        communicator::instance()->remove_connection(f_thread_done);

        // although the f_messenger variable gets reset automatically in
        // the destructor, it would not get removed from the
        // communicator instance if we were not doing it explicitly
        //
        disconnect();
    }


    /** \brief Direct connect to the messenger.
     *
     * In this case we try to connect without the thread. This allows
     * us to avoid the thread problems, but we are blocked until the
     * OS decides to time out or the connection worked.
     */
    void connect()
    {
        if(f_done)
        {
            SNAP_LOG_ERROR
                << "Permanent connection marked done. Cannot attempt to reconnect."
                << SNAP_LOG_SEND;
            return;
        }

        // call the thread connect() function from the main thread
        //
        f_thread_runner.connect();

        // simulate receiving the thread_done() signal
        //
        thread_done();
    }


    /** \brief Check whether the permanent connection is currently connected.
     *
     * This function returns true if the messenger exists, which means that
     * the connection is up.
     *
     * \return true if the connection is up.
     */
    bool is_connected()
    {
        return f_messenger != nullptr;
    }


    /** \brief Try to start the thread runner.
     *
     * This function tries to start the thread runner in order to initiate
     * a connection in the background. If the thread could not be started,
     * then the function returns false.
     *
     * If the thread started, then the function returns true. This does
     * not mean that the connection was obtained. This is known once
     * the process_connected() function is called.
     *
     * \return true if the thread was successfully started.
     */
    bool background_connect()
    {
        if(f_done)
        {
            SNAP_LOG_ERROR
                << "Permanent connection marked done. Cannot attempt to reconnect."
                << SNAP_LOG_SEND;
            return false;
        }

        if(f_thread.is_running())
        {
            SNAP_LOG_ERROR
                << "A background connection attempt is already in progress. Further requests are ignored."
                << SNAP_LOG_SEND;
            return false;
        }

        // create the f_thread_done only when required
        //
        if(f_thread_done == nullptr)
        {
            f_thread_done = std::make_shared<thread_signal_handler>(this);
        }

        communicator::instance()->add_connection(f_thread_done);

        if(!f_thread.start())
        {
            SNAP_LOG_ERROR
                << "The thread used to run the background connection process did not start."
                << SNAP_LOG_SEND;
            return false;
        }

        return true;
    }


    /** \brief Tell the main thread that the background thread is done.
     *
     * This function is called by the thread so the thread_done()
     * function of the thread done object gets called. Only the
     * thread should call this function.
     *
     * As a result the thread_done() function of this class will be
     * called by the main thread.
     */
    void trigger_thread_done()
    {
        f_thread_done->thread_done();
    }


    /** \brief Signal that the background thread is done.
     *
     * This callback is called whenever the background thread sends
     * a signal to us. This is used to avoid calling end user functions
     * that would certainly cause a lot of problem if called from the
     * thread.
     *
     * The function calls the process_connection_failed() if the
     * connection did not happen.
     *
     * The function calls the process_connected() if the connection
     * did happen.
     *
     * \note
     * This is used only if the user requested that the connection
     * happen in the background (i.e. use_thread was set to true
     * in the tcp_client_permanent_message_connection object
     * constructor.)
     */
    void thread_done()
    {
        // if we used the thread we have to remove the signal used
        // to know that the thread was done
        //
        communicator::instance()->remove_connection(f_thread_done);

        // we will access the f_last_error member of the thread runner
        // which may not be available to the main thread yet, calling
        // stop forces a memory barrier so we are all good.
        //
        // calling stop() has no effect if we did not use the thread,
        // however, not calling stop() when we did use the thread
        // causes all sorts of other problems (especially, the thread
        // never gets joined)
        //
        f_thread.stop();

        tcp_bio_client::pointer_t client(f_thread_runner.release_client());
        if(f_done)
        {
            // already marked done, ignore the result and lose the
            // connection immediately
            //
            //f_thread_running.close(); -- not necessary, 'client' is the connection
            return;
        }

        if(client == nullptr)
        {
            SNAP_LOG_ERROR
                << "connection to "
                << f_thread_runner.get_address().to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT)
                << " failed with: "
                << f_thread_runner.get_last_error()
                << SNAP_LOG_SEND;

            // signal that an error occurred
            //
            f_parent->process_connection_failed(f_thread_runner.get_last_error());
        }
        else
        {
            f_messenger = std::make_shared<messenger>(f_parent, client);

            // add the messenger to the communicator
            //
            communicator::instance()->add_connection(f_messenger);

            // if some messages were cached, process them immediately
            //
            while(!f_message_cache.empty())
            {
                f_messenger->send_message(f_message_cache[0]);
                f_message_cache.erase(f_message_cache.begin());
            }

            // let the client know we are now connected
            //
            f_parent->process_connected();
        }
    }

    /** \brief Send a message to the connection.
     *
     * This implementation function actually sends the message to the
     * connection, assuming that the connection exists. Otherwise, it
     * may cache the message (if cache is true.)
     *
     * Note that the message does not get cached if mark_done() was
     * called earlier since we are trying to close the whole connection.
     *
     * \param[in] msg  The message to send.
     * \param[in] cache  Whether to cache the message if the connection is
     *                   currently down.
     *
     * \return true if the message was forwarded, false if the message
     *         was ignored or cached.
     */
    bool send_message(message & msg, bool cache)
    {
        if(f_messenger != nullptr)
        {
            return f_messenger->send_message(msg);
        }

        if(cache && !f_done)
        {
            f_message_cache.push_back(msg);
        }

        return false;
    }


    /** \brief Forget about the messenger connection.
     *
     * This function is used to fully disconnect from the messenger.
     *
     * If there is a messenger, this means:
     *
     * \li Removing the messenger from the communicator instance.
     * \li Closing the connection in the thread object.
     *
     * In most cases, it is called when an error occur, also it happens
     * that we call it explicitly through the disconnect() function
     * of the permanent connection class.
     *
     * \note
     * This is safe, even though it is called from the messenger itself
     * because it will not get deleted yet. This is because the run()
     * loop has a copy in its own temporary copy of the vector of
     * connections.
     */
    void disconnect()
    {
        if(f_messenger != nullptr)
        {
            communicator::instance()->remove_connection(f_messenger);
            f_messenger.reset();

            // just the messenger does not close the TCP connection because
            // we may have another in the thread runner
            //
            f_thread_runner.close();
        }
    }


    /** \brief Return the address of the remote computer.
     *
     * This function retrieve a copy of the socket address of the remote
     * computer (also called the peer).
     *
     * If the connection is not currently valid, then the returned address
     * is the default address (all zeroes). You can determine such by
     * calling the addr::is_default() function.
     *
     * \return The address of the remote computer.
     */
    addr::addr get_client_address() const
    {
        if(f_messenger != nullptr)
        {
            return f_messenger->get_client_address();
        }
        return addr::addr();
    }


    /** \brief Mark the messenger as done.
     *
     * This function is used to mark the messenger as done. This means it
     * will get removed from the communicator instance as soon as it
     * is done with its current write buffer if there is one.
     *
     * You may also want to call the disconnection() function to actually
     * reset the pointer along the way.
     */
    void mark_done()
    {
        f_done = true;

        // once done we don't attempt to reconnect so we can as well
        // get rid of our existing cache immediately to save some
        // memory
        //
        f_message_cache.clear();

        if(f_messenger != nullptr)
        {
            f_messenger->mark_done();
        }
    }


private:
    tcp_client_permanent_message_connection *   f_parent = nullptr;
    thread_signal_handler::pointer_t            f_thread_done = thread_signal_handler::pointer_t();
    runner                                      f_thread_runner;
    cppthread::thread                           f_thread;
    messenger::pointer_t                        f_messenger = messenger::pointer_t();
    message::vector_t                           f_message_cache = message::vector_t();
    bool                                        f_done = false;
};



}
// namespace detail



/** \brief Initializes this TCP client message connection.
 *
 * This implementation creates what we call a permanent connection.
 * Such a connection may fail once in a while. In such circumstances,
 * the class automatically requests for a reconnection (see various
 * parameters below in that regard). However, this causes one issue:
 * by default, the connection just never ends. When you are about
 * ready to close the connection, you must call the mark_done()
 * function first. This will tell the various error functions to
 * drop this connection instead of restarting it after a small pause.
 *
 * This constructor makes sure to initialize the timer and saves
 * the address, port, mode, pause, and use_thread parameters. This
 * constructor makes use of a single address.
 *
 * The timer is first set to trigger immediately. This means the TCP
 * connection will be attempted as soon as possible (the next time
 * the run() loop is entered, it will time out immediately.) You
 * are free to call set_timeout_date() with a date in the future if
 * you prefer that the connect be attempted a little later.
 *
 * The \p pause parameter is used if the connection is lost and this
 * timer is used again to attempt a new connection. It will be reused
 * as long as the connection fails (as a delay). It has to be at least
 * 10 microseconds, although really you should not use less than 1
 * second (1000000). You may set the pause parameter to 0 in which case
 * you are responsible to set the delay (by default there will be no
 * delay and thus the timer will never time out.)
 *
 * To start with a delay, instead of trying to connect immediately,
 * you may pass a negative pause parameter. So for example to get the
 * first attempt 5 seconds after you created this object, you use
 * -5000000LL as the pause parameter.
 *
 * The \p use_thread parameter determines whether the connection should
 * be attempted in a thread (asynchronously) or immediately (which means
 * the timeout callback may block for a while.) If the connection is to
 * a local server with an IP address specified as numbers (i.e. 127.0.0.1),
 * the thread is probably not required. For connections to a remote
 * computer, though, it certainly is important.
 *
 * \param[in] address  The address and port to connect to.
 * \param[in] mode  The mode to use to open the connection.
 * \param[in] pause  The amount of time to wait before attempting a new
 *                   connection after a failure, in microseconds, or 0.
 * \param[in] use_thread  Whether a thread is used to connect to the
 *                        server.
 * \param[in] service_name  The name of your daemon service. Only use once
 *                          on your permanent connection to snapcommunicator.
 */
tcp_client_permanent_message_connection::tcp_client_permanent_message_connection(
            addr::addr const & address
          , mode_t mode
          , std::int64_t const pause
          , bool const use_thread
          , std::string const & service_name)
    : timer(pause < 0 ? -pause : 0)
    , connection_with_send_message(service_name)
    , f_impl(std::make_shared<detail::tcp_client_permanent_message_connection_impl>(this, addr::addr::vector_t{address}, mode))
    , f_pause(llabs(pause))
    , f_use_thread(use_thread)
{
}


/** \brief Initializes this TCP client message connection.
 *
 * The difference between this constructor and the other one is the
 * array of addresses instead of using just one address, this constructor
 * supports multiple. Each address is used to try to connect to the
 * server on the other side. The use of another address happens
 * if the connection fails the connection, the SNI, the handshakes, etc.
 * If the connection comes up and is severed later, then the same address
 * is used to attempt the first reconnect.
 *
 * You may pass a vector with a single address in which case the system
 * behaves the same way as the other constructor.
 *
 * \param[in] addresses  The addresses and ports to connect to.
 * \param[in] mode  The mode to use to open the connection.
 * \param[in] pause  The amount of time to wait before attempting a new
 *                   connection after a failure, in microseconds, or 0.
 * \param[in] use_thread  Whether a thread is used to connect to the
 *                        server.
 * \param[in] service_name  The name of your daemon service. Only use once
 *                          on your permanent connection to snapcommunicator.
 */
tcp_client_permanent_message_connection::tcp_client_permanent_message_connection(
            addr::addr::vector_t const & addresses
          , mode_t mode
          , std::int64_t const pause
          , bool const use_thread
          , std::string const & service_name)
    : timer(pause < 0 ? -pause : 0)
    , connection_with_send_message(service_name)
    , f_impl(std::make_shared<detail::tcp_client_permanent_message_connection_impl>(this, addresses, mode))
    , f_pause(llabs(pause))
    , f_use_thread(use_thread)
{
}


/** \brief Initializes this TCP client message connection.
 *
 * The difference between this constructor and the others is the array of
 * address ranges instead of using just one address or a vector of addresses.
 *
 * This constructor simply transforms the address ranges in a vector of
 * addresses and passes that down to our implementation which handles the
 * connection attempts.
 *
 * \param[in] address_ranges  The address ranges and ports to connect to.
 * \param[in] mode  The mode to use to open the connection.
 * \param[in] pause  The amount of time to wait before attempting a new
 *                   connection after a failure, in microseconds, or 0.
 * \param[in] use_thread  Whether a thread is used to connect to the
 *                        server.
 * \param[in] service_name  The name of your daemon service. Only use once
 *                          on your permanent connection to snapcommunicator.
 */
tcp_client_permanent_message_connection::tcp_client_permanent_message_connection(
            addr::addr_range::vector_t const & address_ranges
          , mode_t mode
          , std::int64_t const pause
          , bool const use_thread
          , std::string const & service_name)
    : timer(pause < 0 ? -pause : 0)
    , connection_with_send_message(service_name)
    , f_impl(std::make_shared<detail::tcp_client_permanent_message_connection_impl>(
                      this
                    , addr::addr_range::to_addresses(address_ranges)
                    , mode))
    , f_pause(llabs(pause))
    , f_use_thread(use_thread)
{
}


/** \brief Destroy instance.
 *
 * This function cleans up everything in the permanent message object.
 */
tcp_client_permanent_message_connection::~tcp_client_permanent_message_connection()
{
    // Does nothing
}


/** \brief Attempt to send a message to this connection.
 *
 * If the connection is currently enabled, the message is sent immediately.
 * Otherwise, it may be cached if the \p cache parameter is set to true.
 * A cached message is forwarded as soon as a new successful connection
 * happens, which can be a problem if messages need to happen in a very
 * specific order (For example, after a reconnection to snapcommunicator
 * you first need to REGISTER or CONNECT...)
 *
 * \param[in] msg  The message to send to the connected server.
 * \param[in] cache  Whether the message should be cached.
 *
 * \return true if the message was sent, false if it was not sent, although
 *         if cache was true, it was cached
 */
bool tcp_client_permanent_message_connection::send_message(
          message & msg
        , bool cache)
{
    return f_impl->send_message(msg, cache);
}


/** \brief Check whether the connection is up.
 *
 * This function returns true if the connection is considered to be up.
 * This means sending messages will work quickly instead of being
 * cached up until an actual TCP/IP connection gets established.
 *
 * Note that the connection may have hanged up since, and the system
 * may not have yet detected the fact (i.e. the connection is going
 * to receive the process_hup() call after the event in which you are
 * working.)
 *
 * \return true if connected
 */
bool tcp_client_permanent_message_connection::is_connected() const
{
    return f_impl->is_connected();
}


/** \brief Disconnect the messenger now.
 *
 * This function kills the current connection.
 *
 * There are a few cases where two daemons communicate between each others
 * and at some point one of them wants to exit and needs to disconnect. This
 * function can be used in that one situation assuming that you have an
 * acknowledgement from the other daemon.
 *
 * Say you have daemon A and B. B wants to quit and before doing so sends
 * a form of "I'm quitting" message to A. In that situation, B is not closing
 * the messenger connection, A is responsible for that (i.e. A acknowledges
 * receipt of the "I'm quitting" message from B by closing the connection.)
 *
 * B also wants to call the mark_done() function to make sure that it
 * does not reconnected a split second later and instead the permanent
 * connection gets removed from the communicator list of connections.
 */
void tcp_client_permanent_message_connection::disconnect()
{
    f_impl->disconnect();
}


/** \brief Overload so we do not have to use namespace everywhere.
 *
 * This function overloads the connection::mark_done() function so
 * we can call it without the need to use timer::mark_done()
 * everywhere.
 */
void tcp_client_permanent_message_connection::mark_done()
{
    timer::mark_done();
}


/** \brief Mark connection as done.
 *
 * This function allows you to mark the permanent connection and the
 * messenger as done.
 *
 * Note that calling this function with false is the same as calling the
 * base class mark_done() function.
 *
 * If the \p message parameter is set to true, we suggest you also call
 * the disconnect() function. That way the messenger will truly get
 * removed from everywhere quickly.
 *
 * \param[in] messenger  If true, also mark the messenger as done.
 */
void tcp_client_permanent_message_connection::mark_done(bool messenger)
{
    timer::mark_done();
    if(messenger)
    {
        f_impl->mark_done();
    }
}


/** \brief Retrieve a copy of the client's address.
 *
 * This function makes a copy of the address of this client connection.
 *
 * \return Return the client's address.
 *
 * \sa get_addr()
 */
addr::addr tcp_client_permanent_message_connection::get_client_address() const
{
    return f_impl->get_client_address();
}


/** \brief Internal timeout callback implementation.
 *
 * This callback implements the guts of this class: it attempts to connect
 * to the specified address and port, optionally after creating a thread
 * so the attempt can happen asynchronously.
 *
 * When the connection fails, the timer is used to try again pause
 * microseconds later (pause as specified in the constructor).
 *
 * When a connection succeeds, the timer is disabled until you detect
 * an error while using the connection and re-enable the timer.
 *
 * \warning
 * This function changes the timeout delay to the pause amount
 * as defined with the constructor. If you want to change that
 * amount, you can do so an any point after this function call
 * using the set_timeout_delay() function. If the pause parameter
 * was set to -1, then the timeout never gets changed.
 * However, you should not use a permanent message timer as your
 * own or you will interfere with the internal use of the timer.
 */
void tcp_client_permanent_message_connection::process_timeout()
{
    // got a spurious call when already marked done
    //
    if(is_done())
    {
        return;
    }

    // change the timeout delay although we will not use it immediately
    // if we start the thread or attempt an immediate connection, but
    // that way the user can change it by calling set_timeout_delay()
    // at any time after the first process_timeout() call
    //
    if(f_pause > 0)
    {
        set_timeout_delay(f_pause);
        f_pause = 0;
    }

    if(f_use_thread)
    {
        // in this case we create a thread, run it and know whether the
        // connection succeeded only when the thread tells us it did
        //
        // TODO: the background_connect() may return false in two situations:
        //       1) when the thread is already running and then the behavior
        //          we have below is INCORRECT
        //       2) when the thread cannot be started (i.e. could not
        //          allocate the stack?) in which case the if() below
        //          is the correct behavior
        //
        if(f_impl->background_connect())
        {
            // we started the thread successfully, so block the timer
            //
            set_enable(false);
        }
    }
    else
    {
        // the success is noted when we receive a call to
        // process_connected(); there we do set_enable(false)
        // so the timer stops
        //
        f_impl->connect();
    }
}


/** \brief Process an error.
 *
 * When an error occurs, we restart the timer so we can attempt to reconnect
 * to that server.
 *
 * If you overload this function, make sure to either call this
 * implementation or enable the timer yourselves.
 *
 * \warning
 * This function does not call the timer::process_error() function
 * which means that this connection is not automatically removed from
 * the communicator object on failures.
 */
void tcp_client_permanent_message_connection::process_error()
{
    if(is_done())
    {
        timer::process_error();
    }
    else
    {
        f_impl->disconnect();
        set_enable(true);
    }
}


/** \brief Process a hang up.
 *
 * When a hang up occurs, we restart the timer so we can attempt to reconnect
 * to that server.
 *
 * If you overload this function, make sure to either call this
 * implementation or enable the timer yourselves.
 *
 * \warning
 * This function does not call the timer::process_hup() function
 * which means that this connection is not automatically removed from
 * the communicator object on failures.
 */
void tcp_client_permanent_message_connection::process_hup()
{
    if(is_done())
    {
        timer::process_hup();
    }
    else
    {
        f_impl->disconnect();
        set_enable(true);
    }
}


/** \brief Process an invalid signal.
 *
 * When an invalid signal occurs, we restart the timer so we can attempt
 * to reconnect to that server.
 *
 * If you overload this function, make sure to either call this
 * implementation or enable the timer yourselves.
 *
 * \warning
 * This function does not call the timer::process_invalid() function
 * which means that this connection is not automatically removed from
 * the communicator object on failures.
 */
void tcp_client_permanent_message_connection::process_invalid()
{
    if(is_done())
    {
        timer::process_invalid();
    }
    else
    {
        f_impl->disconnect();
        set_enable(true);
    }
}


/** \brief Make sure that the messenger connection gets removed.
 *
 * This function makes sure that the messenger sub-connection also gets
 * removed from the communicator. Otherwise it would lock the system
 * since connections are saved in the communicator object as shared
 * pointers.
 */
void tcp_client_permanent_message_connection::connection_removed()
{
    f_impl->disconnect();
}


/** \brief Process a connection failed callback.
 *
 * When a connection attempt fails, we restart the timer so we can
 * attempt to reconnect to that server.
 *
 * If you overload this function, make sure to either call this
 * implementation or enable the timer yourselves.
 *
 * \param[in] error_message  The error message that triggered this callback.
 */
void tcp_client_permanent_message_connection::process_connection_failed(std::string const & error_message)
{
    snapdev::NOT_USED(error_message);
    set_enable(true);
}


/** \brief The connection is ready.
 *
 * This callback gets called whenever the connection succeeded and is
 * ready to be used.
 *
 * You should implement this virtual function if you have to initiate
 * the communication. For example, the snapserver has to send a
 * REGISTER to the communicator system and thus implements this
 * function.
 *
 * The default implementation makes sure that the timer gets turned off
 * so we do not try to reconnect every minute or so.
 */
void tcp_client_permanent_message_connection::process_connected()
{
    set_enable(false);
}



} // namespace ed
// vim: ts=4 sw=4 et
