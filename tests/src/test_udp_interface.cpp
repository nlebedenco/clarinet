#include "test.h"

TEST_CASE("Open/Close", "[udp]")
{
    #if !CLARINET_ENABLE_IPV6
        WARN("Skipping IPV6 tests. IPv6 not supported by the platform.")
    #endif

    const std::vector<std::tuple<int, clarinet_endpoint, clarinet_endpoint>> data =
    {
#if CLARINET_ENABLE_IPV6
        { 0, { clarinet_addr_ipv6_any, 0 }, {clarinet_addr_ipv6_loopback, 0} },
#endif
        { 1, { clarinet_addr_ipv4_any, 0 }, {clarinet_addr_ipv4_loopback, 0} },
    };

    int index;
    clarinet_endpoint any;
    clarinet_endpoint loopback;
    std::tie(index, any, loopback) = GENERATE_REF(from_range(data));

    FROM(index);
    
    clarinet_udp_socket* socket = nullptr;
    
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

        const clarinet_udp_socket* original = socket;

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
    
    SECTION("Open invalid endpoint")
    {
        clarinet_endpoint invalid = {{0}};        
        int errcode = clarinet_udp_open(&socket, &invalid, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        REQUIRE(socket == nullptr);

    }
    
    SECTION("Open any")
    {
        int errcode = clarinet_udp_open(&socket, &any, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket != nullptr);

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);
    }

    SECTION("Open loopback twice different ports")
    {
        int errcode = clarinet_udp_open(&socket, &loopback, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket != nullptr);

        clarinet_endpoint first;
        errcode = clarinet_udp_get_endpoint(socket, &first);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(clarinet_addr_is_equal(&loopback.addr, &first.addr));
        REQUIRE(loopback.port != first.port);

        clarinet_udp_socket* other_socket = nullptr;
        errcode = clarinet_udp_open(&other_socket, &loopback, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(other_socket != nullptr);

        clarinet_endpoint second;
        errcode = clarinet_udp_get_endpoint(other_socket, &second);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(clarinet_addr_is_equal(&loopback.addr, &second.addr));
        REQUIRE(loopback.port != second.port);

        REQUIRE(clarinet_addr_is_equal(&first.addr, &second.addr));
        REQUIRE(first.port != second.port);

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);

        errcode = clarinet_udp_close(&other_socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(other_socket == nullptr);
    }

    SECTION("Open any and loopback same port - both without REUSEADDR")
    {
        int errcode = clarinet_udp_open(&socket, &any, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket != nullptr);

        clarinet_endpoint first;
        errcode = clarinet_udp_get_endpoint(socket, &first);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(any.port != first.port);

        clarinet_endpoint second = { loopback.addr, first.port };
        clarinet_udp_socket* other_socket = nullptr;
        errcode = clarinet_udp_open(&other_socket, &second, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EADDRINUSE));
        REQUIRE(other_socket == nullptr);

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);
    }
    
    SECTION("Open any and loopback same port - second with REUSEADDR")
    {
        int errcode = clarinet_udp_open(&socket, &any, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket != nullptr);

        clarinet_endpoint first;
        errcode = clarinet_udp_get_endpoint(socket, &first);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(any.port != first.port);

        clarinet_endpoint second = { loopback.addr, first.port };
        clarinet_udp_socket* other_socket = nullptr;
        errcode = clarinet_udp_open(&other_socket, &second, &clarinet_udp_settings_default, clarinet_socket_option_to_flag(CLARINET_SO_REUSEADDR));        
        #if HAVE_EXACT_REUSEADDR        
        REQUIRE(Error(errcode) == Error(CLARINET_EADDRINUSE));
        REQUIRE(other_socket == nullptr);
        #else
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(other_socket != nullptr);
        #endif
    
        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);

        #if !HAVE_EXACT_REUSEADDR
        errcode = clarinet_udp_close(&other_socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(other_socket == nullptr);
        #endif        
    }
    
    SECTION("Open loopback same port twice - both without REUSEADDR")
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

    SECTION("Open loopback twice same port - second time with REUSEADDR")
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
        REQUIRE(Error(errcode) == Error(CLARINET_EADDRINUSE));
        REQUIRE(other_socket == nullptr);

        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);
    }
    
     SECTION("Open loopback twice same port - first time with REUSEADDR")
    {
        int errcode = clarinet_udp_open(&socket, &loopback, &clarinet_udp_settings_default, clarinet_socket_option_to_flag(CLARINET_SO_REUSEADDR));
        REQUIRE(Hex(errcode) == Hex(CLARINET_ENONE));
        REQUIRE(socket != nullptr);

        clarinet_endpoint local_endpoint;
        errcode = clarinet_udp_get_endpoint(socket, &local_endpoint);
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

    SECTION("Open loopback twice same port - both with REUSEADDR")
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
        #if HAVE_EXACT_REUSEADDR
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(other_socket != nullptr);
        #else
        REQUIRE(Error(errcode) == Error(CLARINET_EADDRINUSE));
        REQUIRE(other_socket == nullptr);
        #endif    
        
        errcode = clarinet_udp_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(socket == nullptr);

        #if HAVE_EXACT_REUSEADDR
        errcode = clarinet_udp_close(&other_socket);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(other_socket == nullptr);
        #endif
    }
    
    SECTION("Open any in dual stack mode")
    {
        int errcode = clarinet_udp_open(&socket, &any, &clarinet_udp_settings_default, clarinet_socket_option_to_flag(CLARINET_SO_IPV6DUAL));
        if (any.addr.family == CLARINET_AF_INET)
        {            
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(socket == nullptr);
        }
        else if (any.addr.family == CLARINET_AF_INET6)
        {            
        #if CLARINET_ENABLE_IPV6DUAL        
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(socket != nullptr);
            
            errcode = clarinet_udp_close(&socket);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(socket == nullptr);
        #else            
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(socket == nullptr);
        #endif
        }
        else
        {
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(socket == nullptr);            
        }
    }
}

TEST_CASE("Get End Point", "[udp]")
{    
    const std::vector<std::tuple<int, clarinet_endpoint>> data =
    {
#if CLARINET_ENABLE_IPV6
        { 0, { clarinet_addr_ipv6_loopback, 0 } },
#endif
        { 1, { clarinet_addr_ipv4_loopback, 0 } },
    };

    int index;
    clarinet_endpoint local;
    std::tie(index, local) = GENERATE_REF(from_range(data));

    FROM(index);
    
    clarinet_udp_socket* socket = nullptr;
    int errcode = clarinet_udp_open(&socket, &local, &clarinet_udp_settings_default, 0);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    REQUIRE(socket != nullptr);

    clarinet_endpoint actual = {{0}};
    errcode = clarinet_udp_get_endpoint(socket, &actual);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    
    clarinet_endpoint expected = {{0}};
    expected.addr = local.addr;
    expected.port = local.port > 0 ? local.port : actual.port;       
    REQUIRE(clarinet_endpoint_is_equal(&actual, &expected));
    
    errcode = clarinet_udp_close(&socket);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    REQUIRE(socket == nullptr);
}
