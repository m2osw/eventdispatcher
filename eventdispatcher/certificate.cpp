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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Implementation of the certificate class.
 *
 * This file is the implementation of the certificate class. It is used
 * to load an X509 certificate and query various fields (names, not
 * before/after dates).
 *
 * The class allows loading the certificate either from a local .pem file
 * or from an HTTPS server from a domain name.
 */

// make sure we use OpenSSL with multi-thread support
//
#define OPENSSL_THREAD_DEFINES

// self
//
#include    "eventdispatcher/certificate.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/tcp_private.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// OpenSSL
//
//#include    <openssl/bio.h>
//#include    <openssl/err.h>
//#include    <openssl/ssl.h>


// C
//
//#include    <netdb.h>
//#include    <arpa/inet.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \class certificate
 * \brief Create an object that can hold an X509 certificate.
 *
 * This class loads an X509 certificate from either a live website or
 * a .pem file and then offers functions to gather the values from the
 * certificate fields.
 */



/** \brief Make sure the OpenSSL library is initialized.
 *
 * The constructor ensures that the OpenSSL library is fully initialized
 * before we start using it in the other functions.
 */
certificate::certificate()
    : f_impl(std::make_shared<detail::tcp_bio_client_impl>())
{
    detail::bio_initialize();
}


certificate::~certificate()
{
    clear();
}


/** \brief Clear the current certificate.
 *
 * This function clears the current certificate. This means the functions
 * such as get_not_after() will return an error.
 */
void certificate::clear()
{
    if(!empty())
    {
        f_impl->f_ssl_ctx.reset();
        f_impl->f_bio.reset();

        X509_free(f_certificate);
        f_certificate = nullptr;

        f_defined_not_before = false;
        f_defined_not_after = false;
        f_defined_names = false;

        f_cert_parameters.clear();
    }
}


/** \brief Check whether the certificate is defined.
 *
 * This function checks whether a certificate is defined in this object.
 * If so, it returns false. If no certificate was loaded or it was cleared,
 * then the function returns true.
 *
 * \return true if there is no certificate currently defined.
 */
bool certificate::empty() const
{
    return f_certificate == nullptr;
}


/** \brief Set the timeout for the load_from_domain() function.
 *
 * When attempting to load the certificate of a domain, it may take
 * too long to receive a reply. This timeout is used to cancel the
 * process and return instead of waiting forever.
 *
 * \param[in] seconds  The number of seconds to wait before we time out.
 */
void certificate::set_timeout(int seconds)
{
    f_timeout = seconds;
}


/** \brief Load an X509 certificate from a file on disk.
 *
 * This function loads a file representing an X509 certificate.
 *
 * The function always clears the existing certificate. So on an error,
 * any old certificate is not available anymore.
 *
 * \todo
 * Add support for a password.
 *
 * \param[in] filename  The name of the file to load.
 *
 * \return true if the file could be loaded properly.
 */
bool certificate::load_from_file(std::string const & filename)
{
    clear();
    FILE * f(fopen(filename.c_str(), "rb"));
    if(f == nullptr)
    {
        return false;
    }
    X509 * cert = PEM_read_X509(f, &f_certificate, nullptr, nullptr);
    if(cert == nullptr)
    {
        f_certificate = nullptr;
        return false;
    }
    return !empty();
}


/** \brief Load an X509 certificate from a live domain.
 *
 * This function connects to an existing website using the specified
 * \p domain name. The domain must exist, be up, and have a valid TLS
 * certificate.
 *
 * If any step fails, then the function returns false.
 *
 * \param[in] domain  The domain name.
 *
 * \return true if the load worked as expected.
 */
bool certificate::load_from_domain(std::string const & domain)
{
    clear();

    // I tested with the X509_load_http() and it has been failing
    //
    // I'm not too sure what this is about (i.e. is it an actual GET /path
    // and not a connection to a website with a certificate?)
    //
    //f_certificate = X509_load_http(domain.c_str(), nullptr, nullptr, f_timeout);

    std::shared_ptr<SSL_CTX> ssl_ctx; // use a reset(), see SNAP-507
    ssl_ctx.reset(SSL_CTX_new(SSLv23_client_method()), detail::ssl_ctx_deleter);
    if(ssl_ctx == nullptr)
    {
        detail::bio_log_errors();
        return false;
    }

    std::shared_ptr<BIO> bio;
    bio.reset(BIO_new_ssl_connect(ssl_ctx.get()), detail::bio_deleter);
    if(bio == nullptr)
    {
        detail::bio_log_errors();
        return false;
    }

    SSL * ssl(nullptr);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    BIO_get_ssl(bio.get(), &ssl);
#pragma GCC diagnostic pop
    if(ssl == nullptr)
    {
        // TBD: does this mean we would have a plain connection?
        detail::bio_log_errors();
        return false;
    }
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    // not only old style cast (but it's C, so expected)
    // but they want a non-constant pointer!?
    //
    SSL_set_tlsext_host_name(ssl, const_cast<char *>(domain.c_str()));
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    BIO_set_conn_hostname(bio.get(), const_cast<char *>(domain.c_str()));
    BIO_set_conn_port(bio.get(), const_cast<char *>(std::to_string(443).c_str())); // TODO: make that user definable
#pragma GCC diagnostic pop

    if(BIO_do_connect(bio.get()) <= 0)
    {
        detail::bio_log_errors();
        return false;
    }

    if(BIO_do_handshake(bio.get()) != 1)
    {
        detail::bio_log_errors();
        return false;
    }

    f_certificate = SSL_get_peer_certificate(ssl);
    if(f_certificate == nullptr)
    {
        detail::bio_log_errors();
        return false;
    }

    // since the certificate is part of the BIO, we need to keep it all
    // around until we're done
    //
    f_impl->f_ssl_ctx.swap(ssl_ctx);
    f_impl->f_bio.swap(bio);

    return !empty();
}


/** \brief Get the "not before" date of the certificate.
 *
 * This function returns the date when the certificate became valid.
 *
 * The function returns a null date if there is no valid certificate loaded
 * or if the gather of the date failed.
 *
 * \return the date when the certificate became valid.
 */
snapdev::timespec_ex certificate::get_not_before() const
{
    if(!empty()
    && !f_defined_not_before)
    {
        f_defined_not_before = true;

        ASN1_TIME * date(X509_get_notBefore(f_certificate));
        struct tm t;
        int const r(ASN1_TIME_to_tm(date,  &t));
        if(r == 1)
        {
            f_not_before = snapdev::timespec_ex(t);
        }
    }

    return f_not_before;
}


/** \brief Get the "not after" date of the certificate.
 *
 * This function returns the date after which the certificate becomes invalid.
 * In most cases, this date is in the future. If not, then the certificate
 * is not valid anymore.
 *
 * The function returns a null date if there is no valid certificate loaded
 * or if the gather of the date failed.
 *
 * \return the date when the certificate becomes invalid.
 */
snapdev::timespec_ex certificate::get_not_after() const
{
    if(!empty()
    && !f_defined_not_after)
    {
        f_defined_not_after = true;

        ASN1_TIME * date(X509_get_notAfter(f_certificate));
        struct tm t;
        int const r(ASN1_TIME_to_tm(date,  &t));
        if(r == 1)
        {
            f_not_after = snapdev::timespec_ex(t);
        }
    }

    return f_not_after;
}


std::size_t certificate::get_cert_param_size(cert_param_t name) const
{
    // make sure we gathered the "names"
    //
    get_names();

    auto const it(f_cert_parameters.find(name));
    if(it == f_cert_parameters.end())
    {
        return 0;
    }

    return it->second.size();
}


std::string certificate::get_cert_param(cert_param_t name, int idx) const
{
    // make sure we gathered the "names"
    //
    get_names();

    auto const it(f_cert_parameters.find(name));
    if(it == f_cert_parameters.end())
    {
        return std::string();
    }

    if(static_cast<std::size_t>(idx) >= it->second.size())
    {
        return std::string();
    }

    return it->second.at(idx);
}


std::string certificate::get_issuer_common_name() const
{
    return get_cert_param(CERT_PARAM_ISSUER_COMMON_NAME, 0);
}


std::string certificate::get_issuer_country_name() const
{
    return get_cert_param(CERT_PARAM_ISSUER_COUNTRY_NAME, 0);
}


std::string certificate::get_issuer_locality_name() const
{
    return get_cert_param(CERT_PARAM_ISSUER_LOCALITY_NAME, 0);
}


std::string certificate::get_issuer_state_or_province_name() const
{
    return get_cert_param(CERT_PARAM_ISSUER_STATE_OR_PROVINCE_NAME, 0);
}


std::string certificate::get_issuer_organization_name() const
{
    return get_cert_param(CERT_PARAM_ISSUER_ORGANIZATION_NAME, 0);
}


std::string certificate::get_issuer_organizational_unit() const
{
    return get_cert_param(CERT_PARAM_ISSUER_ORGANIZATIONAL_UNIT, 0);
}


std::string certificate::get_issuer_email_address() const
{
    return get_cert_param(CERT_PARAM_ISSUER_EMAIL_ADDRESS, 0);
}


std::string certificate::get_subject_common_name() const
{
    return get_cert_param(CERT_PARAM_SUBJECT_COMMON_NAME, 0);
}


std::string certificate::get_subject_country_name() const
{
    return get_cert_param(CERT_PARAM_SUBJECT_COUNTRY_NAME, 0);
}


std::string certificate::get_subject_locality_name() const
{
    return get_cert_param(CERT_PARAM_SUBJECT_LOCALITY_NAME, 0);
}


std::string certificate::get_subject_state_or_province_name() const
{
    return get_cert_param(CERT_PARAM_SUBJECT_STATE_OR_PROVINCE_NAME, 0);
}


std::string certificate::get_subject_organization_name() const
{
    return get_cert_param(CERT_PARAM_SUBJECT_ORGANIZATION_NAME, 0);
}


std::string certificate::get_subject_organizational_unit() const
{
    return get_cert_param(CERT_PARAM_SUBJECT_ORGANIZATIONAL_UNIT, 0);
}


std::string certificate::get_subject_email_address() const
{
    return get_cert_param(CERT_PARAM_SUBJECT_EMAIL_ADDRESS, 0);
}


void certificate::get_names() const
{
    if(f_defined_names)
    {
        return;
    }
    f_defined_names = true;

    get_each_name(X509_get_issuer_name(f_certificate), CERT_PARAM_ISSUER_BASE);
    get_each_name(X509_get_subject_name(f_certificate), CERT_PARAM_SUBJECT_BASE);

    // OpenSSL v1.1.1 we find a list of extensions in `man 3SSL X509_get_ext_d2i`
    //
    // we are interested by the NID_subject_alt_name and NID_issuer_alt_name
    // extensions at the moment
    //
    get_additional_common_names(NID_issuer_alt_name, CERT_PARAM_ISSUER_COMMON_NAME);
    get_additional_common_names(NID_subject_alt_name, CERT_PARAM_SUBJECT_COMMON_NAME);
}


void certificate::get_each_name(X509_NAME * name, cert_param_t base) const
{
#if 0
char const * base_name = base == CERT_PARAM_ISSUER_BASE ? "issuer" : "subject";
for(int nid_pos(13); nid_pos <= 19; nid_pos++)
for(int pos(-1);;)
{
int const nid(nid_pos == 19 ? 48 : nid_pos);
pos = X509_NAME_get_index_by_NID(name, nid, pos);
if(pos < 0) break;
X509_NAME_ENTRY * entry = X509_NAME_get_entry(name, pos);
if(entry == nullptr)
{
SNAP_LOG_WARNING << "index -- " << base_name << " nid " << nid << " -- no entry at position #" << pos << SNAP_LOG_SEND;
continue;
}
ASN1_STRING * string(X509_NAME_ENTRY_get_data(entry));
if(string == nullptr)
{
SNAP_LOG_WARNING << "index -- " << base_name << " nid " << nid << " -- no string at position #" << pos << SNAP_LOG_SEND;
continue;
}
unsigned char * out(nullptr);
int const len(ASN1_STRING_to_UTF8(&out, string));
std::string utf8(reinterpret_cast<char const *>(out), len);
OPENSSL_free(out);
SNAP_LOG_WARNING << "index -- " << base_name << " nid " << nid << " pos = " << pos << " -> [" << utf8 << "]" << SNAP_LOG_SEND;
}
#endif

    int const max(X509_NAME_entry_count(name));
//SNAP_LOG_WARNING << "checking out " << max << " entries in certificate name." << SNAP_LOG_SEND;
    for(int loc(0); loc < max; ++loc)
    {
        X509_NAME_ENTRY * entry(X509_NAME_get_entry(name, loc));
        if(entry == nullptr)
        {
//SNAP_LOG_WARNING << "no entry at position #" << loc << SNAP_LOG_SEND;
            continue;
        }
        ASN1_OBJECT * object(X509_NAME_ENTRY_get_object(entry));
        if(object == nullptr)
        {
//SNAP_LOG_WARNING << "no object at position #" << loc << SNAP_LOG_SEND;
            continue;
        }
//SNAP_LOG_WARNING << "object to NID: " << OBJ_obj2nid(object) << SNAP_LOG_SEND;
        ASN1_STRING * string(X509_NAME_ENTRY_get_data(entry));
        if(string == nullptr)
        {
            SNAP_LOG_WARNING << "no string at position #" << loc << SNAP_LOG_SEND;
            continue;
        }
        unsigned char * out(nullptr);
        int const len(ASN1_STRING_to_UTF8(&out, string));
        std::string utf8(reinterpret_cast<char const *>(out), len);
        OPENSSL_free(out);

        switch(OBJ_obj2nid(object))
        {
        case NID_commonName: // 13
            f_cert_parameters[base + BASE_PARAM_COMMON_NAME].push_back(utf8);
            break;

        case NID_countryName: // 14
            f_cert_parameters[base + BASE_PARAM_COUNTRY_NAME].push_back(utf8);
            break;

        case NID_localityName: // 15
            f_cert_parameters[base + BASE_PARAM_LOCALITY_NAME].push_back(utf8);
            break;

        case NID_stateOrProvinceName: // 16
            f_cert_parameters[base + BASE_PARAM_STATE_OR_PROVINCE_NAME].push_back(utf8);
            break;

        case NID_organizationName: // 17
            f_cert_parameters[base + BASE_PARAM_ORGANIZATION_NAME].push_back(utf8);
            break;

        case NID_organizationalUnitName: // 18
            f_cert_parameters[base + BASE_PARAM_ORGANIZATIONAL_UNIT].push_back(utf8);
            break;

        case NID_pkcs9_emailAddress: // 48
            f_cert_parameters[base + BASE_PARAM_EMAIL_ADDRESS].push_back(utf8);
            break;

        default:
            SNAP_LOG_TODO
                << "found unrecognized string NID "
                << OBJ_obj2nid(object)
                << " \""
                << utf8
                << "\"; skipping."
                << SNAP_LOG_SEND;
            break;

        }
    }
}


void certificate::get_additional_common_names(int nid, cert_param_t param) const
{
    int pos(-1);
    for(;;)
    {
        int crit(0);
        std::shared_ptr<GENERAL_NAMES> general_names;
        general_names.reset(static_cast<GENERAL_NAMES *>(X509_get_ext_d2i(f_certificate, nid, &crit, &pos)), detail::general_names_deleter);
        if(general_names == nullptr
        || crit < 0)
        {
            return;
        }
        int const max(sk_GENERAL_NAME_num(general_names.get()));
        for(int i(0); i < max; ++i)
        {
            GENERAL_NAME * gen(sk_GENERAL_NAME_value(general_names.get(), i));
            int type(-1);
            ASN1_IA5STRING * dns(static_cast<ASN1_IA5STRING *>(GENERAL_NAME_get0_value(gen, &type)));
            if(dns != nullptr
            && type == GEN_DNS)
            {
                unsigned char * out(nullptr);
                int const len(ASN1_STRING_to_UTF8(&out, dns));
                std::string utf8(reinterpret_cast<char const *>(out), len);
                OPENSSL_free(out);
//SNAP_LOG_WARNING << "--- got name: [" << utf8 << "]" << SNAP_LOG_SEND;

                // the CN in the certificate is likely repeated, so make
                // sure we don't duplicate the name
                //
                auto const it(std::find(f_cert_parameters[param].begin(), f_cert_parameters[param].end(), utf8));
                if(it == f_cert_parameters[param].end())
                {
                    f_cert_parameters[param].push_back(utf8);
                }
            }
        }
    }
    snapdev::NOT_REACHED();
}




} // namespace ed
// vim: ts=4 sw=4 et
