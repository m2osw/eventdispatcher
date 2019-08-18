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

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */

// make sure we use OpenSSL with multi-thread support
// (TODO: move to .cpp once we have the impl!)
#define OPENSSL_THREAD_DEFINES

// self
//
#include    "eventdispatcher/tcp_private.h"

#include    "eventdispatcher/exception.h"


// cppthread lib
//
#include    <cppthread/guard.h>
#include    <cppthread/mutex.h>
#include    <cppthread/thread.h>


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/not_used.h>


// OpenSSL lib
//
#include    <openssl/bio.h>
#include    <openssl/err.h>
#include    <openssl/ssl.h>


// last include
//
#include    <snapdev/poison.h>




#ifndef OPENSSL_THREADS
#error "OPENSSL_THREADS is not defined. Event Dispatcher requires OpenSSL to support multi-threading."
#endif

namespace ed
{


namespace detail
{



///** \brief Data handled by each lock.
// *
// * This function holds the data handled on a per lock basis.
// * Even if your daemon is not using multiple threads, this
// * is likely to kick in.
// */
//class crypto_lock_t
//{
//public:
//    typedef std::vector<crypto_lock_t>  vector_t;
//
//                        crypto_lock_t()
//                        {
//                            pthread_mutex_init(&f_mutex, nullptr);
//                        }
//
//                        ~crypto_lock_t()
//                        {
//                            pthread_mutex_destroy(&f_mutex);
//                        }
//
//    void                lock()
//                        {
//                            pthread_mutex_lock(&f_mutex);
//                        }
//
//    void                unlock()
//                        {
//                            pthread_mutex_unlock(&f_mutex);
//                        }
//
//private:
//    pthread_mutex_t     f_mutex = pthread_mutex_t();
//};


/** \brief The vector of locks.
 *
 * This function is initialized by the crypto_thread_setup().
 *
 * It is defined as a pointer in case someone was to try to access this
 * pointer before entering main().
 */
//crypto_lock_t::vector_t *   g_locks = nullptr;
cppthread::mutex::direct_vector_t  *   g_locks = nullptr;


/** \brief Retrieve the system thread identifier.
 *
 * This function is used by the OpenSSL library to attach an internal thread
 * identifier (\p tid) to a system thread identifier.
 *
 * \param[in] tid  The crypto internal thread identifier.
 */
void pthreads_thread_id(CRYPTO_THREADID * tid)
{
    // on 19.04 the macro does not use tid
    //
    snap::NOTUSED(tid);

    CRYPTO_THREADID_set_numeric(tid, cppthread::gettid());
}


/** \brief Handle locks and unlocks.
 *
 * This function is a callback used to lock and unlock mutexes as required.
 *
 * \param[in] mode  Whether lock or unlock in read or write mode.
 * \param[in] type  The "type" of lock (i.e. the index).
 * \param[in] file  The filename of the source asking for a lock/unlock.
 * \param[in] line  The line number in file where the call was made.
 */
void pthreads_locking_callback(int mode, int type, char const * file, int line)
{
    snap::NOTUSED(file);
    snap::NOTUSED(line);

    if(g_locks == nullptr)
    {
        throw event_dispatcher_initialization_missing("g_locks was not initialized");
    }

/*
# ifdef undef
    BIO_printf(bio_err, "thread=%4d mode=%s lock=%s %s:%d\n",
               CRYPTO_thread_id(),
               (mode & CRYPTO_LOCK) ? "l" : "u",
               (type & CRYPTO_READ) ? "r" : "w", file, line);
# endif
    if (CRYPTO_LOCK_SSL_CERT == type)
            BIO_printf(bio_err,"(t,m,f,l) %ld %d %s %d\n",
                       CRYPTO_thread_id(),
                       mode,file,line);
*/

    // Note: at this point we ignore READ | WRITE because we do not have
    //       such a concept with a simple mutex; we could take those in
    //       account with a semaphore though.
    //
    if((mode & CRYPTO_LOCK) != 0)
    {
        (*g_locks)[type].lock();
    }
    else
    {
        (*g_locks)[type].unlock();
    }
}


/** \brief This function is called once on initialization.
 *
 * This function is called when the bio_initialize() function. It is
 * expected that the bio_initialize() function is called once by the
 * main thread before any other thread has a chance to do so.
 */
void crypto_thread_setup()
{
    cppthread::guard g(*cppthread::g_system_mutex);

    if(g_locks != nullptr)
    {
        throw event_dispatcher_initialization_error(
                "crypto_thread_setup() called for the second time."
                " This usually means two threads are initializing"
                " the BIO environment simultaneously.");
    }

    g_locks = new cppthread::mutex::direct_vector_t(CRYPTO_num_locks());

    CRYPTO_THREADID_set_callback(pthreads_thread_id);
    CRYPTO_set_locking_callback(pthreads_locking_callback);
}


/** \brief This function cleans up the thread setup.
 *
 * This function could be called to clean up the setup created to support
 * multiple threads running with the OpenSSL library.
 *
 * \note
 * At this time this function never gets called. So we have a small leak
 * but that's only on a quit.
 */
void thread_cleanup()
{
    CRYPTO_set_locking_callback(nullptr);

    delete g_locks;
    g_locks = nullptr;
}


/** \brief This function cleans up the error state of a thread.
 *
 * Whenever the OpenSSL system runs in a thread, it may create a
 * state to save various information, especially its error queue.
 *
 * \sa cleanup_on_thread_exit()
 */
void per_thread_cleanup()
{
#if __cplusplus < 201700
    // this function is not necessary in newer versions of OpenSSL
    //
    ERR_remove_thread_state(nullptr);
#endif
}







/** \brief Whether the bio_initialize() function was already called.
 *
 * This flag is used to know whether the bio_initialize() function was
 * already called. Only the bio_initialize() function is expected to
 * make use of this flag. Other functions should simply call the
 * bio_initialize() function (future versions may include addition
 * flags or use various bits in an integer instead.)
 */
bool g_bio_initialized = false;


/** \brief Initialize the BIO library.
 *
 * This function is called by the BIO implementations to initialize the
 * BIO library as required. It can be called any number of times. The
 * initialization will happen only once.
 */
void bio_initialize()
{
    cppthread::guard g(*cppthread::g_system_mutex);

    // already initialized?
    //
    if(g_bio_initialized)
    {
        return;
    }
    g_bio_initialized = true;

    // Make sure the SSL library gets initialized
    //
    SSL_library_init();

    // TBD: should we call the load string functions only when we
    //      are about to generate the first error?
    //
    ERR_load_crypto_strings();
    ERR_load_SSL_strings();
    SSL_load_error_strings();

    // TODO: define a way to only define safe algorithms?
    //       (it looks like we can force TLSv1.2 below at least)
    //
    OpenSSL_add_all_algorithms();

    // TBD: need a PRNG seeding before creating a new SSL context?

    // then initialize the library so it works in a multithreaded
    // environment
    //
    crypto_thread_setup();
}


/** \brief Clean up the BIO environment.
 *
 * This function cleans up the BIO environment.
 *
 * \note
 * This function is here mainly for documentation rather than to get called.
 * Whenever you exit a process that uses the BIO calls it will leak
 * a few things. To make the process really spanking clean, you want
 * to call this function before exit(3). You have to make sure that
 * you call this function only after every single BIO object was
 * closed and none must be opened after this call.
 */
void bio_cleanup()
{
#if __cplusplus < 201700
    // this function is not necessary in newer versions of OpenSSL
    //
    ERR_remove_state(0);
#endif

    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
}


/** \brief Get all the error messages and output them in our logs.
 *
 * This function reads all existing errors from the OpenSSL library
 * and send them to our logs.
 *
 * \param[in] sni  Whether SNI is ON (true) or OFF (false).
 */
int bio_log_errors()
{
    // allow for up to 5 errors in one go, but we have a HUGE problem
    // at this time as in some cases the same error is repeated forever
    //
    for(int i(0);; ++i)
    {
        char const * filename(nullptr);
        int line(0);
        char const * data(nullptr);
        int flags(0);
        unsigned long bio_errno(ERR_get_error_line_data(&filename, &line, &data, &flags));
        if(bio_errno == 0)
        {
            // no more errors
            //
            return i;
        }

        // get corresponding messages too
        //
        // Note: current OpenSSL documentation on Ubuntu says errmsg[]
        //       should be at least 120 characters BUT the code actually
        //       use a limit of 256...
        //
        char errmsg[256];
        ERR_error_string_n(bio_errno, errmsg, sizeof(errmsg) / sizeof(errmsg[0]));
        // WARNING: the ERR_error_string() function is NOT multi-thread safe

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        int const lib_num(ERR_GET_LIB(bio_errno));
        int const func_num(ERR_GET_FUNC(bio_errno));
#pragma GCC diagnostic pop
        char const * lib_name(ERR_lib_error_string(lib_num));
        char const * func_name(ERR_func_error_string(func_num));
        int const reason_num(ERR_GET_REASON(bio_errno));
        char const * reason(ERR_reason_error_string(reason_num));

        if(lib_name == nullptr)
        {
            lib_name = "<no libname>";
        }
        if(func_name == nullptr)
        {
            func_name = "<no funcname>";
        }
        if(reason == nullptr)
        {
            reason = "<no reason>";
        }

        // the format used by the OpenSSL library is as follow:
        //
        //     [pid]:error:[error code]:[library name]:[function name]:[reason string]:[file name]:[line]:[optional text message]
        //
        // we do not duplicate the [pid] and "error" but include all the
        // other fields
        //
        SNAP_LOG_ERROR
            << "OpenSSL: ["
            << bio_errno // should be shown in hex...
            << "/"
            << lib_num
            << "|"
            << func_num
            << "|"
            << reason_num
            << "]:["
            << lib_name
            << "]:["
            << func_name
            << "]:["
            << reason
            << "]:["
            << filename
            << "]:["
            << line
            << "]:["
            << ((flags & ERR_TXT_STRING) != 0 && data != nullptr ? data : "(no details)")
            << "]"
            << SNAP_LOG_SEND;
    }
}


/** \brief Free a BIO object.
 *
 * This deleter is used to make sure that the BIO object gets freed
 * whenever the object holding it gets destroyed.
 *
 * Note that deleting a BIO connection calls shutdown() and close()
 * on the socket. In other words, it hangs up.
 *
 * \param[in] bio  The BIO object to be freed.
 */
void bio_deleter(BIO * bio)
{
    // IMPORTANT NOTE:
    //
    //   The BIO_free_all() calls shutdown() on the socket. This is not
    //   acceptable in a normal Unix application that makes use of fork().
    //   So... instead we ask the BIO interface to not close the socket,
    //   and instead we close it ourselves. This means the shutdown()
    //   never gets called.
    //
    BIO_set_close(bio, BIO_NOCLOSE);

    int c;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    BIO_get_fd(bio, &c);
#pragma GCC diagnostic pop
    if(c != -1)
    {
        close(c);
    }

    BIO_free_all(bio);
}


/** \brief Free an SSL_CTX object.
 *
 * This deleter is used to make sure that the SSL_CTX object gets
 * freed whenever the object holding it gets destroyed.
 */
void ssl_ctx_deleter(SSL_CTX * ssl_ctx)
{
    SSL_CTX_free(ssl_ctx);
}


}
// namespace detail














} // namespace ed
// vim: ts=4 sw=4 et
