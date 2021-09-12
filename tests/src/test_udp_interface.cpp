#include "test.h"

TEST_CASE("IPv4 Socket Open/Close", "[udp]")
{
    clarinet_udp_socket* socket = nullptr;
    const clarinet_endpoint any = { clarinet_addr_ipv4_any, 0 };
    const clarinet_endpoint loopback = {clarinet_addr_ipv4_loopback, 0};
    
    SECTION("Open NULL socket")
    {
        int errcode = clarinet_udp_open(nullptr, &any, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        REQUIRE(socket == nullptr);
    }
    
    SECTION("Open NULL endpoint")
    {
        int errcode = clarinet_udp_open(&socket, nullptr, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        REQUIRE(socket == nullptr);
    }
    
    SECTION("Open NULL settings")
    {        
        int errcode = clarinet_udp_open(&socket, &any, nullptr, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        REQUIRE(socket == nullptr);
    }


    SECTION("Open same socket twice")
    {
        int errcode = clarinet_udp_open(&socket, &loopback, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket != nullptr);

        clarinet_udp_socket* original = socket;

        errcode = clarinet_udp_open(&socket, &loopback, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        REQUIRE(socket == original);

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);
    }
    
    SECTION("Close NULL socket")
    {
        int errcode = clarinet_udp_close(nullptr);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        REQUIRE(socket == nullptr);
    }
    
    SECTION("Close same socket twice")
    {        
        int errcode = clarinet_udp_open(&socket, &any, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        
        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        REQUIRE(socket == nullptr);
    }
    
    SECTION("Open 0.0.0.0:0")
    {
        int errcode = clarinet_udp_open(&socket, &any, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket != nullptr);

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);
    }

    SECTION("Open 127.0.0.1:0 twice")
    {
        int errcode = clarinet_udp_open(&socket, &loopback, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket != nullptr);

        clarinet_endpoint first;
        errcode = clarinet_udp_get_endpoint(socket, &first);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(clarinet_addr_is_equal(&loopback.addr, &first.addr));
        REQUIRE(loopback.port != first.port);

        clarinet_udp_socket* other = nullptr;
        errcode = clarinet_udp_open(&other, &loopback, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(other != nullptr);

        clarinet_endpoint second;
        errcode = clarinet_udp_get_endpoint(other, &second);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(clarinet_addr_is_equal(&loopback.addr, &second.addr));
        REQUIRE(loopback.port != second.port);

        REQUIRE(clarinet_addr_is_equal(&first.addr, &second.addr));
        REQUIRE(first.port != second.port);

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);

        errcode = clarinet_udp_close(&other);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);
    }

    SECTION("Open 127.0.0.1:X twice without REUSEADDR")
    {
        int errcode = clarinet_udp_open(&socket, &loopback, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket != nullptr);

        clarinet_endpoint local_endpoint;
        errcode = clarinet_udp_get_endpoint(socket , &local_endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(clarinet_addr_is_equal(&loopback.addr, &local_endpoint.addr));
        REQUIRE(loopback.port != local_endpoint.port);

        clarinet_udp_socket* other_socket = nullptr;
        errcode = clarinet_udp_open(&other_socket, &local_endpoint, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EADDRINUSE));
        REQUIRE(other_socket == nullptr);

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);
    }

    SECTION("Open 127.0.0.1:X twice second time with REUSEADDR")
    {
        int errcode = clarinet_udp_open(&socket, &loopback, &clarinet_udp_settings_default, 0);
        REQUIRE(Hex(errcode) == Hex(CLARINET_ENONE));
        REQUIRE(socket != nullptr);

        clarinet_endpoint local_endpoint;
        errcode = clarinet_udp_get_endpoint(socket, &local_endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(clarinet_addr_is_equal(&loopback.addr, &local_endpoint.addr));
        REQUIRE(loopback.port != local_endpoint.port);

        clarinet_udp_socket* other_socket = nullptr;
        errcode = clarinet_udp_open(&other_socket, &local_endpoint, &clarinet_udp_settings_default, clarinet_socket_option_to_flag(CLARINET_SO_REUSEADDR));
        REQUIRE(Error(errcode) == Error(CLARINET_EACCES));
        REQUIRE(other_socket == nullptr);

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);
    }

    SECTION("Open 127.0.0.1:X twice both with REUSEADDR")
    {
        uint32_t flags = clarinet_socket_option_to_flag(CLARINET_SO_REUSEADDR);

        int errcode = clarinet_udp_open(&socket, &loopback, &clarinet_udp_settings_default, flags);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket != nullptr);

        clarinet_endpoint local_endpoint;
        errcode = clarinet_udp_get_endpoint(socket, &local_endpoint);
        REQUIRE(Hex(errcode) == Hex(CLARINET_ENONE));
        REQUIRE(clarinet_addr_is_equal(&loopback.addr, &local_endpoint.addr));
        REQUIRE(loopback.port != local_endpoint.port);

        clarinet_udp_socket* other_socket = nullptr;
        errcode = clarinet_udp_open(&other_socket, &local_endpoint, &clarinet_udp_settings_default, flags);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(other_socket != nullptr);

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);

        errcode = clarinet_udp_close(&other_socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(other_socket == nullptr);
    }
}

