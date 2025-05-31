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

// self
//
#include    "catch_main.h"


// eventdispatcher
//
#include    <eventdispatcher/certificate.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C
//
//#include    <unistd.h>



CATCH_TEST_CASE("certificate", "[certificate]")
{
    CATCH_START_SECTION("certificate: Load PEM file")
    {
        std::string const dir(SNAP_CATCH2_NAMESPACE::g_source_dir());
        std::string const cert_dir(dir + "/tests/certificate");
        std::string const cert_filename(cert_dir + "/snakeoil.pem");
        ed::certificate cert;
        CATCH_REQUIRE(cert.empty());
        CATCH_REQUIRE(cert.load_from_file(cert_filename));
        CATCH_REQUIRE_FALSE(cert.empty());
        snapdev::timespec_ex date(cert.get_not_before());
//SNAP_LOG_WARNING << "--- BEFORE " << date << " [" << date.to_string("%Y/%m/%d %H:%M:%S.%N") << "]" << SNAP_LOG_SEND;
        CATCH_REQUIRE(date.tv_sec == 1738371918);
        CATCH_REQUIRE(date.to_string("%Y/%m/%d %H:%M:%S.%N") == "2025/02/01 01:05:18.000000000");
        date = cert.get_not_after();
//SNAP_LOG_WARNING << "--- AFTER " << date << " [" << date.to_string("%Y/%m/%d %H:%M:%S.%N") << "]" << SNAP_LOG_SEND;
        CATCH_REQUIRE(date.tv_sec == 1769907918);
        CATCH_REQUIRE(date.to_string("%Y/%m/%d %H:%M:%S.%N") == "2026/02/01 01:05:18.000000000");
        CATCH_REQUIRE(cert.get_issuer_common_name() == "example.net");
        CATCH_REQUIRE(cert.get_issuer_country_name() == "US");
        CATCH_REQUIRE(cert.get_issuer_locality_name() == "Los Angeles");
        CATCH_REQUIRE(cert.get_issuer_state_or_province_name() == "California");
        CATCH_REQUIRE(cert.get_issuer_organization_name() == "Made to Order Software Corporation");
        CATCH_REQUIRE(cert.get_issuer_organizational_unit() == "Software Development");
        CATCH_REQUIRE(cert.get_issuer_email_address() == "contact@example.net");
        CATCH_REQUIRE(cert.get_subject_common_name() == "example.net");
        CATCH_REQUIRE(cert.get_subject_country_name() == "US");
        CATCH_REQUIRE(cert.get_subject_locality_name() == "Los Angeles");
        CATCH_REQUIRE(cert.get_subject_state_or_province_name() == "California");
        CATCH_REQUIRE(cert.get_subject_organization_name() == "Made to Order Software Corporation");
        CATCH_REQUIRE(cert.get_subject_organizational_unit() == "Software Development");
        CATCH_REQUIRE(cert.get_subject_email_address() == "contact@example.net");

        CATCH_REQUIRE(cert.get_cert_param_size(ed::CERT_PARAM_SUBJECT_COMMON_NAME) == 1);
        CATCH_REQUIRE(cert.get_cert_param(ed::CERT_PARAM_SUBJECT_COMMON_NAME, 0) == "example.net");

        CATCH_REQUIRE(cert.get_cert_param_size(ed::CERT_PARAM_ISSUER_COMMON_NAME) == 1);
        CATCH_REQUIRE(cert.get_cert_param(ed::CERT_PARAM_ISSUER_COMMON_NAME, 0) == "example.net");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("certificate: Load PEM from domain")
    {
        ed::certificate cert;
        CATCH_REQUIRE(cert.empty());
        CATCH_REQUIRE(cert.load_from_domain("www.m2osw.com"));
        CATCH_REQUIRE_FALSE(cert.empty());

// the dates change all the time so we do not verify them here
// I kept the code so one can look at said dates
//
//snapdev::timespec_ex date(cert.get_not_before());
//SNAP_LOG_WARNING << "--- BEFORE " << date << " [" << date.to_string("%Y/%m/%d %H:%M:%S.%N") << "]" << SNAP_LOG_SEND;
//date = cert.get_not_after();
//SNAP_LOG_WARNING << "--- AFTER " << date << " [" << date.to_string("%Y/%m/%d %H:%M:%S.%N") << "]" << SNAP_LOG_SEND;

//SNAP_LOG_WARNING
//    << "issuer: CN:" << cert.get_issuer_common_name()
//    << "/C:" << cert.get_issuer_country_name()
//    << "/L:" << cert.get_issuer_locality_name()
//    << "/S:" << cert.get_issuer_state_or_province_name()
//    << "/O:" << cert.get_issuer_organization_name()
//    << "/U:" << cert.get_issuer_organizational_unit()
//    << "/E:" << cert.get_issuer_email_address()
//    << SNAP_LOG_SEND;
//
//SNAP_LOG_WARNING
//    << "subject: CN:" << cert.get_subject_common_name()
//    << "/C:" << cert.get_subject_country_name()
//    << "/L:" << cert.get_subject_locality_name()
//    << "/S:" << cert.get_subject_state_or_province_name()
//    << "/O:" << cert.get_subject_organization_name()
//    << "/U:" << cert.get_subject_organizational_unit()
//    << "/E:" << cert.get_subject_email_address()
//    << SNAP_LOG_SEND;

        CATCH_REQUIRE(cert.get_issuer_common_name() == "R11");
        CATCH_REQUIRE(cert.get_issuer_country_name() == "US");
        CATCH_REQUIRE(cert.get_issuer_locality_name() == "");
        CATCH_REQUIRE(cert.get_issuer_state_or_province_name() == "");
        CATCH_REQUIRE(cert.get_issuer_organization_name() == "Let's Encrypt");
        CATCH_REQUIRE(cert.get_issuer_organizational_unit() == "");
        CATCH_REQUIRE(cert.get_issuer_email_address() == "");

        CATCH_REQUIRE(cert.get_cert_param_size(ed::CERT_PARAM_ISSUER_COMMON_NAME) == 1);

        CATCH_REQUIRE(cert.get_subject_common_name() == "*.m20sw.com");
        CATCH_REQUIRE(cert.get_subject_country_name() == "");
        CATCH_REQUIRE(cert.get_subject_locality_name() == "");
        CATCH_REQUIRE(cert.get_subject_state_or_province_name() == "");
        CATCH_REQUIRE(cert.get_subject_organization_name() == "");
        CATCH_REQUIRE(cert.get_subject_organizational_unit() == "");
        CATCH_REQUIRE(cert.get_subject_email_address() == "");

        // at the moment, we have multiple names in our certificate
        // so we can test that too
        //
        std::size_t max(cert.get_cert_param_size(ed::CERT_PARAM_SUBJECT_COMMON_NAME));
        CATCH_REQUIRE(max == 8);
        CATCH_REQUIRE(cert.get_cert_param(ed::CERT_PARAM_SUBJECT_COMMON_NAME, 0) == "*.m20sw.com");
        CATCH_REQUIRE(cert.get_cert_param(ed::CERT_PARAM_SUBJECT_COMMON_NAME, 1) == "*.m2o.software");
        CATCH_REQUIRE(cert.get_cert_param(ed::CERT_PARAM_SUBJECT_COMMON_NAME, 2) == "*.m2osw.com");
        CATCH_REQUIRE(cert.get_cert_param(ed::CERT_PARAM_SUBJECT_COMMON_NAME, 3) == "*.madetoorder.software");
        CATCH_REQUIRE(cert.get_cert_param(ed::CERT_PARAM_SUBJECT_COMMON_NAME, 4) == "m20sw.com");
        CATCH_REQUIRE(cert.get_cert_param(ed::CERT_PARAM_SUBJECT_COMMON_NAME, 5) == "m2o.software");
        CATCH_REQUIRE(cert.get_cert_param(ed::CERT_PARAM_SUBJECT_COMMON_NAME, 6) == "m2osw.com");
        CATCH_REQUIRE(cert.get_cert_param(ed::CERT_PARAM_SUBJECT_COMMON_NAME, 7) == "madetoorder.software");
        CATCH_REQUIRE(cert.get_cert_param(ed::CERT_PARAM_SUBJECT_COMMON_NAME, 8) == std::string());
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("certificate_error", "[certificate][error]")
{
    CATCH_START_SECTION("certificate_error: Try loading invalid file")
    {
        std::string const dir(SNAP_CATCH2_NAMESPACE::get_tmp_dir("certificates"));
        std::string const filename(dir + "/invalid.pem");
        {
            std::ofstream cert(filename);
            cert << "This is not a certificate." << std::endl;
        }
        ed::certificate cert;
        CATCH_REQUIRE(cert.empty());
        CATCH_REQUIRE_FALSE(cert.load_from_file(filename));
        CATCH_REQUIRE(cert.empty());
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
