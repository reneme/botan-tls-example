# botan-tls-example

This is a minimal example how to use Botan's TLS stream with Boost beast to perform a HTTPS request

## Credits

The code in https_client.cpp is an adaption of the [example code of Boost beast](https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/sync-ssl/http_client_sync_ssl.cpp) by @vinniefalco.

## What is this?

This is merely an example application to showcase how to use Botan's TLS engine on top of Boost ASIO to perform a HTTPS request via Boost beast. It merely replaces ASIO's TLS stream (that is based on OpenSSL) with [the ASIO stream shipped with Botan](https://botan.randombit.net/handbook/api_ref/tls.html#tls-stream).

## How to build

The easiest way to build this is to use the [Conan package manager](https://conan.io). Assuming it is already installed on your system, please run this:

```bash
# use Conan the build the 3rd party dependencies
conan install . --build missing

# compile the example code (with your favourite compiler)
clang++ @conanbuildinfo.args -std=c++11 -o https_client https_client.cpp

# run the example (arguments: <host> <port number> <HTTP path>)
./https_client www.randombit.net 443 /
```
