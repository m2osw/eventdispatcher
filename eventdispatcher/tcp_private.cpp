// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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


// cppthread
//
#include    <cppthread/guard.h>
#include    <cppthread/mutex.h>
#include    <cppthread/thread.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_used.h>


// OpenSSL
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



/** \brief The vector of locks.
 *
 * This function is initialized by the crypto_thread_setup().
 *
 * It is defined as a pointer in case someone was to try to access this
 * pointer before entering main().
 */
cppthread::mutex::direct_vector_t  *   g_locks = nullptr;


/** \brief Retrieve the system thread identifier.
 *
 * This function is used by the OpenSSL library to attach an internal thread
 * identifier (\p tid) to a system thread identifier.
 *
 * \note
 * Since Ubuntu 19.04, the \p tid parameter is ignored.
 *
 * \param[in] tid  The crypto internal thread identifier.
 */
void pthreads_thread_id(CRYPTO_THREADID * tid)
{
    // since 19.04 the macro does not use tid
    //
    snapdev::NOT_USED(tid);

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
    snapdev::NOT_USED(file, line);

    if(g_locks == nullptr)
    {
        throw initialization_missing("g_locks was not initialized");
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
        throw initialization_error(
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
#if OPENSSL_VERSION_NUMBER < 0x1010000fL
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
#if OPENSSL_VERSION_NUMBER < 0x30000020L
    // this function is not necessary in newer versions of OpenSSL
    //
    ERR_load_SSL_strings();
#endif
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
 * In our coverage tests, we verify that memory is not leaking. You have
 * to make sure this function gets called before exit(3) happens in that
 * specific situation. In general, we do so using the RAII class which
 * in the main() function of the test.
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
#if OPENSSL_VERSION_NUMBER < 0x1000000fL
    // this function is not necessary in newer versions of OpenSSL
    //
    ERR_remove_state(0);
#endif

    CONF_modules_unload(1);
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
}


/** \brief Get all the error messages and output them in our logs.
 *
 * This function reads all existing errors from the OpenSSL library
 * and send them to our logs.
 *
 * \return The number of errors that the function found.
 */
int bio_log_errors()
{
    // allow for up to 5 errors in one go, but we have a HUGE problem
    // at this time as in some cases the same error is repeated forever
    //
    for(int count(0);; ++count)
    {
        char const * filename(nullptr);
        int line(0);
        char const * data(nullptr);
        int flags(0);
        char const * func_name(nullptr);
#if OPENSSL_VERSION_NUMBER < 0x30000020L
        unsigned long bio_errno(ERR_get_error_line_data(&filename, &line, &data, &flags));
#else
        unsigned long bio_errno(ERR_get_error_all(&filename, &line, &func_name, &data, &flags));
#endif
        if(bio_errno == 0)
        {
            // no more errors
            //
            return count;
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
#if OPENSSL_VERSION_NUMBER < 0x30000020L
        int const func_num(ERR_GET_FUNC(bio_errno));
#endif
#pragma GCC diagnostic pop
        char const * lib_name(ERR_lib_error_string(lib_num));
#if OPENSSL_VERSION_NUMBER < 0x30000020L
        func_name = ERR_func_error_string(func_num);
#endif
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


/** \brief Delete a GENERAL_NAMES object.
 *
 * This function deletes a GENERAL_NAMES object. It is useful with a
 * shared pointer in order to get rid of a GENERAL_NAMES object at
 * any point in time (i.e. when done with it or when an exception
 * occurs).
 *
 * \note
 * Apparently the function safely accepts NULL as input.
 *
 * \param[in] general_names  The GENERAL_NAMES to delete.
 */
void general_names_deleter(GENERAL_NAMES * general_names)
{
    GENERAL_NAMES_free(general_names);
}


/** \brief Free a BIO object.
 *
 * This deleter is used to make sure that the BIO object gets freed
 * whenever the object holding it gets destroyed.
 *
 * Note that by default deleting a BIO connection calls shutdown()
 * and close() on the socket. In other words, it hangs up. To prevent
 * that issue, we re-implement the shutdown() function.
 *
 * If you created a child via a fork() with the intend of using the
 * socket further, then this wouldn't work properly without that
 * redefinition.
 *
 * \note
 * In older versions of the bio_deleter(), I would close the file
 * descriptor before calling the BIO_free_all() function, This
 * failed by leaking really badly by not releasing many of the
 * resources used by the BIO interface.
 *
 * \param[in] bio  The BIO object to be freed.
 */
void bio_deleter(BIO * bio)
{
    BIO_free_all(bio);
}


/** \brief Free an SSL_CTX object.
 *
 * This deleter is used to make sure that the SSL_CTX object gets
 * freed whenever the object holding it gets destroyed.
 *
 * \param[in] ssl_ctx  The SSL context to delete.
 */
void ssl_ctx_deleter(SSL_CTX * ssl_ctx)
{
    SSL_CTX_free(ssl_ctx);
}



} // namespace detail
} // namespace ed



/** \brief Prevent the shutdown(2) function from being called.
 *
 * This re-implementation of the shutdown() function is useful in processes
 * that create a BIO based object and then share it with a child process
 * via a fork() call. The result is two folds:
 *
 * 1. The parent does not actually shutdown the socket so the child can
 *    use it as expected
 * 2. The child cannot shutdown the socket either since at that point
 *    we do not know whether to call the C library function or not.
 *
 * For this to work, make sure the eventdispatcher library is loaded before
 * the openssl library. You can see the list and order by looking at the
 * final executable with:
 *
 * \code
 * objdump -x executable | less
 * \endcode
 *
 * The libssl and libcrypto are expected to appear after the
 * libeventdispatcher. Here is an example from the unittest of this
 * project:
 *
 * \code
 * [...snip...]
 * Dynamic Section:
 *   NEEDED               libasan.so.6
 *   NEEDED               libreporter.so.1
 *   NEEDED               libcppprocess.so.1
 *   NEEDED               libeventdispatcher.so.1
 *   NEEDED               libadvgetopt.so.2
 *   NEEDED               libcppthread.so.1
 *   NEEDED               libaddr.so.1
 *   NEEDED               libutf8.so.1
 *   NEEDED               libssl.so.3
 *   NEEDED               libcrypto.so.3
 *   NEEDED               libsnaplogger.so.1
 * [...snip...]
 * \endcode
 *
 * \todo
 * For now, this works as expected (I think). If we need to fix the issue
 * (i.e. need to properly shutdown(2) in the child processes), then we
 * should look into either not using the BIO layer, go directly to the
 * SSL layer (which apparently does not call shutdown(2), which means
 * it would work as it does with this hack) or save the parent PID and
 * check in the function below whether we are hitting the parent or
 * not. If not, allow the call to the C library function. Note that the
 * parent may not always fork(), so there is the possibility that
 * this won't work 100% as expected either way.
 *
 * \return This implementation always return 0.
 */
extern "C" int shutdown(int, int)
{
//SNAP_LOG_WARNING
//<< "---------------------------- shutdown() intercepted!!! -----------------------------------"
//<< SNAP_LOG_SEND;

    // do nothing
    //
    return 0;
}



// vim: ts=4 sw=4 et
