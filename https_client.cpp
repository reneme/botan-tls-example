//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP SSL client, synchronous
//
//------------------------------------------------------------------------------

#include <boost/asio/error.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

#include <botan/asio_stream.h>
#include <botan/auto_rng.h>
#include <botan/credentials_manager.h>
#include <botan/certstor_system.h>
#include <botan/tls_client.h>

using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

namespace http = boost::beast::http;  // from <boost/beast/http.hpp>

namespace {

// Simple override of a Botan::Credentials_Manager that returns a handle to the
// operating system's trusted root certificate chain. Botan provides an
// abstraction for accessing those on Linux, Windows and macOS.
class CredentialsManager : public Botan::Credentials_Manager
{
  public:
    std::vector<Botan::Certificate_Store*>
    trusted_certificate_authorities(const std::string&,
                                    const std::string&) override {
        return { &_systemCertStore };
    }

  private:
    Botan::System_Certificate_Store _systemCertStore;
};

// Basically uses Botan's default TLS policy but disables the hard requirement
// on a certificate revocation info during the TLS handshake.
class TlsPolicy : public Botan::TLS::Policy {
  public:
    bool require_cert_revocation_info() const override { return false; }
};

}

// Performs an HTTP GET and prints the response
int main(int argc, char **argv)
{
    try {
        // Check command line arguments.
        if (argc != 4 && argc != 5) {
            std::cerr << "Usage: http-client-sync-ssl <host> <port> <target> "
                         "[<HTTP version: 1.0 or 1.1(default)>]\n"
                      << "Example:\n"
                      << "    http-client-sync-ssl www.example.com 443 /\n"
                      << "    http-client-sync-ssl www.example.com 443 / 1.0\n";
            return EXIT_FAILURE;
        }
        auto const host   = argv[1];
        auto const port   = argv[2];
        auto const target = argv[3];
        int version       = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

        boost::asio::io_context ioc;
        tcp::resolver           resolver{ioc};
        tcp::socket             sock{ioc};

        Botan::AutoSeeded_RNG                 rng;
        Botan::TLS::Session_Manager_In_Memory sessionManager(rng);
        Botan::TLS::Server_Information        serverInformation(host, port);
        CredentialsManager                    credentialsManager;
        TlsPolicy                             policy;

        Botan::TLS::Context ctx(credentialsManager,
                                rng,
                                sessionManager,
                                policy,
                                serverInformation);

        Botan::TLS::Stream<tcp::socket &> stream{ctx, sock};

        // Look up the domain name
        auto const results = resolver.resolve(host, port);
        // Make the connection on the IP address we get from a lookup
        boost::asio::connect(stream.next_layer(), results);

        std::cout << "shaking hands" << std::endl;
        stream.handshake(Botan::TLS::Connection_Side::CLIENT);

        std::cout << "now we're talking" << std::endl;
        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(stream, req);

        // // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer buffer;

        // // Declare a container to hold the response
        http::response<http::string_body> res;

        // Receive the HTTP response
        boost::system::error_code ec;
        std::size_t parsed_bytes = http::read(stream, buffer, res, ec);

        std::cout << "Parse: " << parsed_bytes << " bytes" << std::endl;

        if (ec == boost::asio::error::eof) {
            // Rationale:
            // http:
            // //stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec.assign(0, ec.category());
        }

        std::cout << res << std::endl;

        // Gracefully close the stream
        ec = boost::system::error_code();
        stream.shutdown(ec);
        if (ec == boost::asio::error::eof) {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec.assign(0, ec.category());
        }
        if (ec) {
            throw boost::system::system_error{ec};
        }

    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
