// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

/** \file
 * \brief Class used to gather certificate information.
 *
 * Class used to load a certificate either from a website using its domain
 * name or from a file such as `certificate.pem`.
 *
 * The class then offers varying functions to extract data from the
 * certificate such as the expiration date.
 */

// self
//
//#include    <eventdispatcher/tcp_bio_options.h>
//#include    <eventdispatcher/utils.h>


// libaddr
//
#include    <libaddr/addr.h>


// snapdev
//
#include    <snapdev/timespec_ex.h>


// C++
//
#include    <memory>



// definitions from openssl/x509.h
struct x509_st;
struct X509_name_st;


namespace ed
{



typedef int cert_param_t;


constexpr cert_param_t const    BASE_PARAM_COMMON_NAME = 0;
constexpr cert_param_t const    BASE_PARAM_COUNTRY_NAME = 1;
constexpr cert_param_t const    BASE_PARAM_LOCALITY_NAME = 2;
constexpr cert_param_t const    BASE_PARAM_STATE_OR_PROVINCE_NAME = 3;
constexpr cert_param_t const    BASE_PARAM_ORGANIZATION_NAME = 4;
constexpr cert_param_t const    BASE_PARAM_ORGANIZATIONAL_UNIT = 5;
constexpr cert_param_t const    BASE_PARAM_EMAIL_ADDRESS = 6;
constexpr cert_param_t const    BASE_PARAM_count = 7;


constexpr cert_param_t const    CERT_PARAM_ISSUER_BASE = 0;
constexpr cert_param_t const    CERT_PARAM_ISSUER_COMMON_NAME            = CERT_PARAM_ISSUER_BASE + BASE_PARAM_COMMON_NAME;
constexpr cert_param_t const    CERT_PARAM_ISSUER_COUNTRY_NAME           = CERT_PARAM_ISSUER_BASE + BASE_PARAM_COUNTRY_NAME;
constexpr cert_param_t const    CERT_PARAM_ISSUER_LOCALITY_NAME          = CERT_PARAM_ISSUER_BASE + BASE_PARAM_LOCALITY_NAME;
constexpr cert_param_t const    CERT_PARAM_ISSUER_STATE_OR_PROVINCE_NAME = CERT_PARAM_ISSUER_BASE + BASE_PARAM_STATE_OR_PROVINCE_NAME;
constexpr cert_param_t const    CERT_PARAM_ISSUER_ORGANIZATION_NAME      = CERT_PARAM_ISSUER_BASE + BASE_PARAM_ORGANIZATION_NAME;
constexpr cert_param_t const    CERT_PARAM_ISSUER_ORGANIZATIONAL_UNIT    = CERT_PARAM_ISSUER_BASE + BASE_PARAM_ORGANIZATIONAL_UNIT;
constexpr cert_param_t const    CERT_PARAM_ISSUER_EMAIL_ADDRESS          = CERT_PARAM_ISSUER_BASE + BASE_PARAM_EMAIL_ADDRESS;

constexpr cert_param_t const    CERT_PARAM_SUBJECT_BASE = CERT_PARAM_ISSUER_BASE + BASE_PARAM_count;
constexpr cert_param_t const    CERT_PARAM_SUBJECT_COMMON_NAME            = CERT_PARAM_SUBJECT_BASE + BASE_PARAM_COMMON_NAME;
constexpr cert_param_t const    CERT_PARAM_SUBJECT_COUNTRY_NAME           = CERT_PARAM_SUBJECT_BASE + BASE_PARAM_COUNTRY_NAME;
constexpr cert_param_t const    CERT_PARAM_SUBJECT_LOCALITY_NAME          = CERT_PARAM_SUBJECT_BASE + BASE_PARAM_LOCALITY_NAME;
constexpr cert_param_t const    CERT_PARAM_SUBJECT_STATE_OR_PROVINCE_NAME = CERT_PARAM_SUBJECT_BASE + BASE_PARAM_STATE_OR_PROVINCE_NAME;
constexpr cert_param_t const    CERT_PARAM_SUBJECT_ORGANIZATION_NAME      = CERT_PARAM_SUBJECT_BASE + BASE_PARAM_ORGANIZATION_NAME;
constexpr cert_param_t const    CERT_PARAM_SUBJECT_ORGANIZATIONAL_UNIT    = CERT_PARAM_SUBJECT_BASE + BASE_PARAM_ORGANIZATIONAL_UNIT;
constexpr cert_param_t const    CERT_PARAM_SUBJECT_EMAIL_ADDRESS          = CERT_PARAM_SUBJECT_BASE + BASE_PARAM_EMAIL_ADDRESS;




// Create/manage certificates details:
// https://help.ubuntu.com/lts/serverguide/certificates-and-security.html
class certificate
{
public:
    typedef std::shared_ptr<certificate>    pointer_t;

                            certificate();
                            certificate(certificate const & src) = delete;
                            ~certificate();

    certificate &           operator = (certificate const & rhs) = delete;

    void                    clear();
    bool                    empty() const;
    void                    set_timeout(int seconds);

    bool                    load_from_file(std::string const & filename);
    bool                    load_from_domain(std::string const & domain);

    snapdev::timespec_ex    get_not_before() const;
    snapdev::timespec_ex    get_not_after() const;

    std::size_t             get_cert_param_size(cert_param_t name) const;
    std::string             get_cert_param(cert_param_t name, int idx) const;

    std::string             get_issuer_common_name() const;
    std::string             get_issuer_country_name() const;
    std::string             get_issuer_locality_name() const;
    std::string             get_issuer_state_or_province_name() const;
    std::string             get_issuer_organization_name() const;
    std::string             get_issuer_organizational_unit() const;
    std::string             get_issuer_email_address() const;

    std::string             get_subject_common_name() const;
    std::string             get_subject_country_name() const;
    std::string             get_subject_locality_name() const;
    std::string             get_subject_state_or_province_name() const;
    std::string             get_subject_organization_name() const;
    std::string             get_subject_organizational_unit() const;
    std::string             get_subject_email_address() const;

private:
    typedef std::map<cert_param_t, std::vector<std::string>>    cert_parameters_t;

    void                            get_names() const;
    void                            get_each_name(X509_name_st * name, cert_param_t base) const;
    void                            get_additional_common_names(int nid, cert_param_t param) const;

    std::uint32_t                   f_timeout = 5; // used by load_from_domain()
    x509_st *                       f_certificate = nullptr; // the X509 structure

    mutable bool                    f_defined_not_before = false;
    mutable bool                    f_defined_not_after = false;
    mutable bool                    f_defined_names = false;

    mutable snapdev::timespec_ex    f_not_before = snapdev::timespec_ex();
    mutable snapdev::timespec_ex    f_not_after = snapdev::timespec_ex();

    mutable cert_parameters_t       f_cert_parameters = cert_parameters_t();
    //mutable std::string             f_common_name = std::string();                  // NID_commonName (13)
    //mutable std::string             f_country_name = std::string();                 // NID_countryName (14)
    //mutable std::string             f_locality_name = std::string();                // NID_localityName (15)
    //mutable std::string             f_state_or_province_name = std::string();       // NID_stateOrProvinceName (16)
    //mutable std::string             f_organization_name = std::string();            // NID_organizationName (17)
    //mutable std::string             f_organizational_unit = std::string();          // NID_organizationalUnitName (18)
    //mutable std::string             f_email_address = std::string();                // NID_pkcs9_emailAddress (48)
};



} // namespace ed
// vim: ts=4 sw=4 et
