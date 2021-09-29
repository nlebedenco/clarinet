#include "test.h"

#if !CLARINET_ENABLE_IPV6
WARN("IPv6 suport is disabled. Some IPV6 tests may be skipped. ")
#endif

TEST_CASE("Open/Close", "[udp]")
{
    SECTION("Invalid Arguments")
    {
        SECTION("Open NULL socket")
        {
            clarinet_endpoint any = { clarinet_addr_any_ipv4, 0 };
            int errcode = clarinet_udp_open(nullptr, &any, &clarinet_udp_settings_default, 0);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Open NULL endpoint")
        {
            clarinet_udp_socket* socket = nullptr;
            int errcode = clarinet_udp_open(&socket, nullptr, &clarinet_udp_settings_default, 0);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(socket == nullptr);
        }

        SECTION("Open NULL settings")
        {
            clarinet_endpoint any = { clarinet_addr_any_ipv4, 0 };
            clarinet_udp_socket* socket = nullptr;
            int errcode = clarinet_udp_open(&socket, &any, nullptr, 0);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(socket == nullptr);
        }

        SECTION("Close NULL socket")
        {
            int errcode = clarinet_udp_close(nullptr);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Open invalid endpoint")
        {
            clarinet_endpoint invalid = { { 0 } };
            clarinet_udp_socket* socket = nullptr;
            int errcode = clarinet_udp_open(&socket, &invalid, &clarinet_udp_settings_default, 0);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(socket == nullptr);

        }
    }

    SECTION("Open one address once")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> data = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
#if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
#endif
        };
        // @formatter:on

        int index;
        clarinet_endpoint endpoint;
        std::tie(index, endpoint) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_udp_socket* socket = nullptr;

        SECTION("Open same socket twice")
        {
            int errcode = clarinet_udp_open(&socket, &endpoint, &clarinet_udp_settings_default, 0);
            const finalizer onexit = [&]
            {
                clarinet_udp_close(&socket);
            };

            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(socket != nullptr);

            const clarinet_udp_socket* original = socket;

            errcode = clarinet_udp_open(&socket, &endpoint, &clarinet_udp_settings_default, 0);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(socket == original);
        }

        SECTION("Close same socket twice")
        {
            int errcode = clarinet_udp_open(&socket, &endpoint, &clarinet_udp_settings_default, 0);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_udp_close(&socket);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(socket == nullptr);

            errcode = clarinet_udp_close(&socket);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(socket == nullptr);
        }

        SECTION("Open in normal mode")
        {
            int errcode = clarinet_udp_open(&socket, &endpoint, &clarinet_udp_settings_default, 0);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(socket != nullptr);

            errcode = clarinet_udp_close(&socket);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(socket == nullptr);
        }

        SECTION("Open in dual stack mode")
        {
            int errcode = clarinet_udp_open(&socket, &endpoint, &clarinet_udp_settings_default,
                                            clarinet_so_flag(CLARINET_SO_IPV6DUAL));
            if (endpoint.addr.family == CLARINET_AF_INET)
            {
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(socket == nullptr);
            }
            else if (endpoint.addr.family == CLARINET_AF_INET6)
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

    SECTION("Open one address twice with different ports")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> data = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
#if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
#endif
        };
        // @formatter:on

        int index;
        clarinet_endpoint endpoint;
        std::tie(index, endpoint) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_udp_socket* sa = nullptr;
        int errcode = clarinet_udp_open(&sa, &endpoint, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(sa != nullptr);
        const finalizer onexita = [&]
        {
            clarinet_udp_close(&sa);
        };

        clarinet_udp_socket* sb = nullptr;
        errcode = clarinet_udp_open(&sb, &endpoint, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(sb != nullptr);
        const finalizer onexitb = [&]
        {
            clarinet_udp_close(&sb);
        };

        clarinet_endpoint sa_endp;
        errcode = clarinet_udp_get_endpoint(sa, &sa_endp);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_endpoint sb_endp;
        errcode = clarinet_udp_get_endpoint(sb, &sb_endp);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        REQUIRE(sa_endp.port != sb_endp.port);
    }

    SECTION("Open two addresses with the same port")
    {
        const auto NONE = 0U;
        const auto REUSEADDR = clarinet_so_flag(CLARINET_SO_REUSEADDR);
        const auto IPV6DUAL = clarinet_so_flag(CLARINET_SO_IPV6DUAL);

        // @formatter:off
        // Table: index, first socket address | first socket flags | second socket address | second socket flags | expected result
        const std::vector<std::tuple<int, clarinet_addr, uint32_t, clarinet_addr, uint32_t, int>> data = {
            {  0, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_any_ipv4,         NONE,               CLARINET_EADDRINUSE },
            {  1, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_loopback_ipv4,    NONE,               CLARINET_EADDRINUSE },
            {  2, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_any_ipv4,         NONE,               CLARINET_EADDRINUSE },
            {  3, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_loopback_ipv4,    NONE,               CLARINET_EADDRINUSE },
            {  4, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_EADDRINUSE },
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            {  5, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_ENONE      }, // BSD/Darwin
            #else
            {  5, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_EADDRINUSE }, // Others
            #endif
            #if defined(__linux__)
            {  6, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_EADDRINUSE }, // Linux
            #else
            {  6, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_ENONE      }, // Others
            #endif
            {  7, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_EADDRINUSE },
            {  8, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_any_ipv4,         NONE,               CLARINET_EADDRINUSE },
            #if defined(_WIN32)
            {  9, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_loopback_ipv4,    NONE,               CLARINET_ENONE      }, // Windows
            #else
            {  9, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_loopback_ipv4,    NONE,               CLARINET_EADDRINUSE }, // Others
            #endif
            { 10, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_any_ipv4,         NONE,               CLARINET_EADDRINUSE },
            { 11, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_loopback_ipv4,    NONE,               CLARINET_EADDRINUSE },
            { 12, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_ENONE      },
            { 13, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_ENONE      },
            { 14, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_ENONE      },
            { 15, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_ENONE      },

    #if CLARINET_ENABLE_IPV6
            { 16, clarinet_addr_any_ipv6,       NONE,               clarinet_addr_any_ipv6,         NONE,               CLARINET_EADDRINUSE },
            { 17, clarinet_addr_any_ipv6,       NONE,               clarinet_addr_loopback_ipv6,    NONE,               CLARINET_EADDRINUSE },
            { 18, clarinet_addr_loopback_ipv6,  NONE,               clarinet_addr_any_ipv6,         NONE,               CLARINET_EADDRINUSE },
            { 19, clarinet_addr_loopback_ipv6,  NONE,               clarinet_addr_loopback_ipv6,    NONE,               CLARINET_EADDRINUSE },
            { 20, clarinet_addr_any_ipv6,       NONE,               clarinet_addr_any_ipv6,         REUSEADDR,          CLARINET_EADDRINUSE },
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            { 21, clarinet_addr_any_ipv6,       NONE,               clarinet_addr_loopback_ipv6,    REUSEADDR,          CLARINET_ENONE      }, // BSD/Darwin
            #else
            { 21, clarinet_addr_any_ipv6,       NONE,               clarinet_addr_loopback_ipv6,    REUSEADDR,          CLARINET_EADDRINUSE }, // Others
            #endif
            #if defined(__linux__)
            { 22, clarinet_addr_loopback_ipv6,  NONE,               clarinet_addr_any_ipv6,         REUSEADDR,          CLARINET_EADDRINUSE }, // Linux
            #else
            { 22, clarinet_addr_loopback_ipv6,  NONE,               clarinet_addr_any_ipv6,         REUSEADDR,          CLARINET_ENONE      }, // Others
            #endif
            { 23, clarinet_addr_loopback_ipv6,  NONE,               clarinet_addr_loopback_ipv6,    REUSEADDR,          CLARINET_EADDRINUSE },
            { 24, clarinet_addr_any_ipv6,       REUSEADDR,          clarinet_addr_any_ipv6,         NONE,               CLARINET_EADDRINUSE },
            #if defined(_WIN32)
            { 25, clarinet_addr_any_ipv6,       REUSEADDR,          clarinet_addr_loopback_ipv6,    NONE,               CLARINET_ENONE      }, // Windows
            #else
            { 25, clarinet_addr_any_ipv6,       REUSEADDR,          clarinet_addr_loopback_ipv6,    NONE,               CLARINET_EADDRINUSE }, // Others
            #endif
            { 26, clarinet_addr_loopback_ipv6,  REUSEADDR,          clarinet_addr_any_ipv6,         NONE,               CLARINET_EADDRINUSE },
            { 27, clarinet_addr_loopback_ipv6,  REUSEADDR,          clarinet_addr_loopback_ipv6,    NONE,               CLARINET_EADDRINUSE },
            { 28, clarinet_addr_any_ipv6,       REUSEADDR,          clarinet_addr_any_ipv6,         REUSEADDR,          CLARINET_ENONE      },
            { 29, clarinet_addr_any_ipv6,       REUSEADDR,          clarinet_addr_loopback_ipv6,    REUSEADDR,          CLARINET_ENONE      },
            { 30, clarinet_addr_loopback_ipv6,  REUSEADDR,          clarinet_addr_any_ipv6,         REUSEADDR,          CLARINET_ENONE      },
            { 31, clarinet_addr_loopback_ipv6,  REUSEADDR,          clarinet_addr_loopback_ipv6,    REUSEADDR,          CLARINET_ENONE      },

            { 32, clarinet_addr_any_ipv6,       NONE,               clarinet_addr_any_ipv4,         NONE,               CLARINET_ENONE      },
            { 33, clarinet_addr_any_ipv6,       NONE,               clarinet_addr_loopback_ipv4,    NONE,               CLARINET_ENONE      },
            { 34, clarinet_addr_loopback_ipv6,  NONE,               clarinet_addr_any_ipv4,         NONE,               CLARINET_ENONE      },
            { 35, clarinet_addr_loopback_ipv6,  NONE,               clarinet_addr_loopback_ipv4,    NONE,               CLARINET_ENONE      },
            { 36, clarinet_addr_any_ipv6,       NONE,               clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_ENONE      },
            { 37, clarinet_addr_any_ipv6,       NONE,               clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_ENONE      },
            { 38, clarinet_addr_loopback_ipv6,  NONE,               clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_ENONE      },
            { 39, clarinet_addr_loopback_ipv6,  NONE,               clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_ENONE      },
            { 40, clarinet_addr_any_ipv6,       REUSEADDR,          clarinet_addr_any_ipv4,         NONE,               CLARINET_ENONE      },
            { 41, clarinet_addr_any_ipv6,       REUSEADDR,          clarinet_addr_loopback_ipv4,    NONE,               CLARINET_ENONE      },
            { 42, clarinet_addr_loopback_ipv6,  REUSEADDR,          clarinet_addr_any_ipv4,         NONE,               CLARINET_ENONE      },
            { 43, clarinet_addr_loopback_ipv6,  REUSEADDR,          clarinet_addr_loopback_ipv4,    NONE,               CLARINET_ENONE      },
            { 44, clarinet_addr_any_ipv6,       REUSEADDR,          clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_ENONE      },
            { 45, clarinet_addr_any_ipv6,       REUSEADDR,          clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_ENONE      },
            { 46, clarinet_addr_loopback_ipv6,  REUSEADDR,          clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_ENONE      },
            { 47, clarinet_addr_loopback_ipv6,  REUSEADDR,          clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_ENONE      },

            { 48, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_any_ipv6,         NONE,               CLARINET_ENONE      },
            { 49, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_loopback_ipv6,    NONE,               CLARINET_ENONE      },
            { 40, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_any_ipv6,         NONE,               CLARINET_ENONE      },
            { 51, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_loopback_ipv6,    NONE,               CLARINET_ENONE      },
            { 52, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_any_ipv6,         REUSEADDR,          CLARINET_ENONE      },
            { 53, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_loopback_ipv6,    REUSEADDR,          CLARINET_ENONE      },
            { 54, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_any_ipv6,         REUSEADDR,          CLARINET_ENONE      },
            { 55, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_loopback_ipv6,    REUSEADDR,          CLARINET_ENONE      },
            { 56, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_any_ipv6,         NONE,               CLARINET_ENONE      },
            { 57, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_loopback_ipv6,    NONE,               CLARINET_ENONE      },
            { 58, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_any_ipv6,         NONE,               CLARINET_ENONE      },
            { 59, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_loopback_ipv6,    NONE,               CLARINET_ENONE      },
            { 60, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_any_ipv6,         REUSEADDR,          CLARINET_ENONE      },
            { 61, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_loopback_ipv6,    REUSEADDR,          CLARINET_ENONE      },
            { 62, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_any_ipv6,         REUSEADDR,          CLARINET_ENONE      },
            { 63, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_loopback_ipv6,    REUSEADDR,          CLARINET_ENONE      },

        #if CLARINET_ENABLE_IPV6DUAL
            { 64, clarinet_addr_any_ipv6,       IPV6DUAL,           clarinet_addr_any_ipv4,         NONE,               CLARINET_EADDRINUSE },
            { 65, clarinet_addr_any_ipv6,       IPV6DUAL,           clarinet_addr_loopback_ipv4,    NONE,               CLARINET_EADDRINUSE },
            { 66, clarinet_addr_loopback_ipv6,  IPV6DUAL,           clarinet_addr_any_ipv4,         NONE,               CLARINET_ENONE      },
            { 67, clarinet_addr_loopback_ipv6,  IPV6DUAL,           clarinet_addr_loopback_ipv4,    NONE,               CLARINET_ENONE      },
            { 68, clarinet_addr_any_ipv6,       IPV6DUAL,           clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_EADDRINUSE },
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            { 69, clarinet_addr_any_ipv6,       IPV6DUAL,           clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_ENONE      }, // BSD/Darwin
            #else
            { 69, clarinet_addr_any_ipv6,       IPV6DUAL,           clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_EADDRINUSE }, // Others
            #endif
            { 70, clarinet_addr_loopback_ipv6,  IPV6DUAL,           clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_ENONE      },
            { 71, clarinet_addr_loopback_ipv6,  IPV6DUAL,           clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_ENONE      },
            #if defined(_WIN32)
            { 72, clarinet_addr_any_ipv6,       IPV6DUAL|REUSEADDR, clarinet_addr_any_ipv4,         NONE,               CLARINET_ENONE      }, // Windows
            { 73, clarinet_addr_any_ipv6,       IPV6DUAL|REUSEADDR, clarinet_addr_loopback_ipv4,    NONE,               CLARINET_ENONE      }, // Windows
            #else
            { 72, clarinet_addr_any_ipv6,       IPV6DUAL|REUSEADDR, clarinet_addr_any_ipv4,         NONE,               CLARINET_EADDRINUSE }, // Others
            { 73, clarinet_addr_any_ipv6,       IPV6DUAL|REUSEADDR, clarinet_addr_loopback_ipv4,    NONE,               CLARINET_EADDRINUSE }, // Others
            #endif
            { 74, clarinet_addr_loopback_ipv6,  IPV6DUAL|REUSEADDR, clarinet_addr_any_ipv4,         NONE,               CLARINET_ENONE      },
            { 75, clarinet_addr_loopback_ipv6,  IPV6DUAL|REUSEADDR, clarinet_addr_loopback_ipv4,    NONE,               CLARINET_ENONE      },
            { 76, clarinet_addr_any_ipv6,       IPV6DUAL|REUSEADDR, clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_ENONE      },
            { 77, clarinet_addr_any_ipv6,       IPV6DUAL|REUSEADDR, clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_ENONE      },
            { 78, clarinet_addr_loopback_ipv6,  IPV6DUAL|REUSEADDR, clarinet_addr_any_ipv4,         REUSEADDR,          CLARINET_ENONE      },
            { 79, clarinet_addr_loopback_ipv6,  IPV6DUAL|REUSEADDR, clarinet_addr_loopback_ipv4,    REUSEADDR,          CLARINET_ENONE      },

            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            { 80, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_any_ipv6,         IPV6DUAL,           CLARINET_ENONE      }, // BSD/Darwin
            #else
            { 80, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_any_ipv6,         IPV6DUAL,           CLARINET_EADDRINUSE }, // Others
            #endif
            { 81, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_loopback_ipv6,    IPV6DUAL,           CLARINET_ENONE      },
            { 82, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_any_ipv6,         IPV6DUAL,           CLARINET_EADDRINUSE },
            { 83, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_loopback_ipv6,    IPV6DUAL,           CLARINET_ENONE      },
            #if defined(__linux__)
            { 84, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CLARINET_EADDRINUSE }, // Linux
            #else
            { 84, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CLARINET_ENONE      }, // Others
            #endif
            { 85, clarinet_addr_any_ipv4,       NONE,               clarinet_addr_loopback_ipv6,    IPV6DUAL|REUSEADDR, CLARINET_ENONE      },
            #if defined(__linux__)
            { 86, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CLARINET_EADDRINUSE }, // Linux
            #else
            { 86, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CLARINET_ENONE      }, // Others
            #endif
            { 87, clarinet_addr_loopback_ipv4,  NONE,               clarinet_addr_loopback_ipv6,    IPV6DUAL|REUSEADDR, CLARINET_ENONE      },
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            { 88, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_any_ipv6,         IPV6DUAL,           CLARINET_ENONE      }, // BSD/Darwin
            #else
            { 88, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_any_ipv6,         IPV6DUAL,           CLARINET_EADDRINUSE }, // Others
            #endif
            { 89, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_loopback_ipv6,    IPV6DUAL,           CLARINET_ENONE      },
            { 90, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_any_ipv6,         IPV6DUAL,           CLARINET_EADDRINUSE },
            { 91, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_loopback_ipv6,    IPV6DUAL,           CLARINET_ENONE      },
            { 92, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CLARINET_ENONE      },
            { 93, clarinet_addr_any_ipv4,       REUSEADDR,          clarinet_addr_loopback_ipv6,    IPV6DUAL|REUSEADDR, CLARINET_ENONE      },
            { 94, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CLARINET_ENONE      },
            { 95, clarinet_addr_loopback_ipv4,  REUSEADDR,          clarinet_addr_loopback_ipv6,    IPV6DUAL|REUSEADDR, CLARINET_ENONE      },
        #endif // CLARINET_ENABLE_IPV6DUAL
    #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        int index;
        clarinet_addr addra;
        uint32_t fa;
        clarinet_addr addrb;
        uint32_t fb;
        int expected;
        std::tie(index, addra, fa, addrb, fb, expected) = GENERATE_REF(from_range(data));

        FROM(index);

        clarinet_udp_socket* sa = nullptr;
        clarinet_endpoint endpa = { addra, 0 };
        int errcode = clarinet_udp_open(&sa, &endpa, &clarinet_udp_settings_default, fa);
        const finalizer onexita = [&]
        {
            clarinet_udp_close(&sa);
        };
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(sa != nullptr);

        clarinet_endpoint saendp;
        errcode = clarinet_udp_get_endpoint(sa, &saendp);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_endpoint endpb = { addrb, saendp.port };
        clarinet_udp_socket* sb = nullptr;
        errcode = clarinet_udp_open(&sb, &endpb, &clarinet_udp_settings_default, fb);
        const finalizer onexitb = [&]
        {
            clarinet_udp_close(&sb);
        };
        CHECK(Error(errcode) == Error(expected));
    }

    SECTION("Open with default settings")
    {
        // TODO: check against sysctl and windows docs
    }

    SECTION("Open with custom settings")
    {
        // TODO: check by getting the sock opts and comparing to the settings
    }
}

TEST_CASE("Get Endpoint", "[udp]")
{
    const std::vector<std::tuple<int, clarinet_endpoint>> data =
        {
            { 0, { clarinet_addr_loopback_ipv4, 0 } },
#if CLARINET_ENABLE_IPV6
            { 1, { clarinet_addr_loopback_ipv6, 0 } },
#endif
        };

    int index;
    clarinet_endpoint local;
    std::tie(index, local) = GENERATE_REF(from_range(data));

    FROM(index);

    clarinet_udp_socket* socket = nullptr;
    int errcode = clarinet_udp_open(&socket, &local, &clarinet_udp_settings_default, 0);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    REQUIRE(socket != nullptr);
    const finalizer onexit = [&]
    {
        clarinet_udp_close(&socket);
    };

    clarinet_endpoint actual = { { 0 } };
    errcode = clarinet_udp_get_endpoint(socket, &actual);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

    clarinet_endpoint expected = { { 0 } };
    expected.addr = local.addr;
    expected.port = local.port > 0 ? local.port : actual.port;
    REQUIRE(clarinet_endpoint_is_equal(&actual, &expected));
}

// TODO: define separate TEST cases for Windows, Linux and macOS highlighting their particularities.

TEST_CASE("Send", "[udp]")
{
    const clarinet_endpoint any = { clarinet_addr_any_ipv4, 0 };
    const clarinet_endpoint remote = clarinet_make_endpoint(clarinet_make_ipv4(8, 8, 8, 8), 443);
   // const clarinet_endpoint loopback = { clarinet_addr_loopback_ipv4, 9 }; // using Discard Service port

#if CLARINET_ENABLE_IPV6
    //const clarinet_endpoint any6 = { clarinet_addr_any_ipv6, 0 };
    //const clarinet_endpoint loopback6 = { clarinet_addr_loopback_ipv6, 9 }; // using Discard Service port
#endif

    // @formatter:off
    // Table: index | source | destination | send buffer size | send length | count | expected
    //
    // Windows, Linux and BSD/Darwin all have preemptive kernels with immediate dispatch to the NIC so tests may be able
    // to effectively transmit a message larger than the send-buffer or send more consecutive messages than the buffer
    // was expected to support.
    //
    // On Windows, the send-buffer does not include packet overhead and when the system is not under stress the first
    // message in a sequence may be dispatched directly to the NIC whil the remaining messages have to be buffered.
    // The buffer must be 1-byte larger than the space required by the messages. For example, a message with 2048 bytes
    // requires a free buffer size of 2048+1. Two messages of 2048 bytes require a free buffer size 2048*2 + 1. size for example, may be able to transmit up to 65507 bytes
    // in the first send operation bypassing the send-buffer completely while subsequent sends may need to be buffered.
    // The same applies to Linux and BSD/Darwin. Also, packets that are blocked by the firewall or cannot be routed
    // are dropped before taking up any space in the buffer or causing an error which might give the false impression
    // tthat the transmission was immediate. Therefore, make sure the remote address used in the tests is valid and not
    // blocked by a firewall.
    //
    // Linux has a fixed overhead per packet in memory but actual buffer space consumed is variable because the system
    // does not reserve the whole buffer size for each socket at once. Instead, memory is allocated and released on
    // demand as packets are created and disposed in the network pipeline with each socket keeping track of its ammount
    // of memory used so to remain within the buffer limits. In an attempt to prevent excessive heap fragmentation the
    // Linux kernel employs slab allocations which may consume considerably more memory per packet than expected.
    // Since Linux has the largest overhead compared to Windows and BSD/Darwin, buffer sizes are calculated according to
    // the payload size conversion table described in clarinet_socket_settings.
    const std::vector<std::tuple<int, clarinet_endpoint, clarinet_endpoint, int, int, int, int>> data =
    {
        // {  0,  any,    remote,  2048,     0, 1,    CLARINET_ENONE },
        // {  1,  any,    remote,  2048,     1, 3,    CLARINET_ENONE },
        // {  2,  any,    remote,  2048,     2, 3,    CLARINET_ENONE },
        // {  3,  any,    remote,  2048,     4, 3,    CLARINET_ENONE },
        // {  4,  any,    remote,  2048,     8, 3,    CLARINET_ENONE },
        // {  5,  any,    remote,  2048,    16, 3,    CLARINET_ENONE },
        // {  6,  any,    remote,  2048,    32, 3,    CLARINET_ENONE },
        // {  7,  any,    remote,  2048,    64, 2,    CLARINET_ENONE },
        // {  8,  any,    remote,  2048,   128, 2,    CLARINET_ENONE },
        // {  9,  any,    remote,  2048,   256, 2,    CLARINET_ENONE },
        // { 10,  any,    remote,  2048,   512, 1,    CLARINET_ENONE },
        // { 11,  any,    remote,  2048,  1024, 1,    CLARINET_ENONE },
        // { 12,  any,    remote, 4352*2,  2048, 3,    CLARINET_ENONE },
        { 13,  any,    remote, 9127,  9127, 100,    CLARINET_ENONE },

        //{ 13,  any,    remote, 93667, 65507, 1,    CLARINET_ENONE },
        //{ 14,  any,    remote, 93667, 65535, 1, CLARINET_EMSGSIZE },
        //{ 15,  any,  loopback,  2048,     0, 1,    CLARINET_ENONE },
        //{ 16,  any,  loopback,  2048,     1, 3,    CLARINET_ENONE },
        //{ 17,  any,  loopback,  2048,     2, 3,    CLARINET_ENONE },
        //{ 18,  any,  loopback,  2048,     4, 3,    CLARINET_ENONE },
        //{ 19,  any,  loopback,  2048,     8, 3,    CLARINET_ENONE },
        //{ 20,  any,  loopback,  2048,    16, 3,    CLARINET_ENONE },
        //{ 21,  any,  loopback,  2048,    32, 3,    CLARINET_ENONE },
        //{ 22,  any,  loopback,  2048,    64, 2,    CLARINET_ENONE },
        //{ 23,  any,  loopback,  2048,   128, 2,    CLARINET_ENONE },
        //{ 24,  any,  loopback,  2048,   256, 2,    CLARINET_ENONE },
        //{ 25,  any,  loopback,  2048,   512, 1,    CLARINET_ENONE },
        //{ 26,  any,  loopback,  2048,  1024, 1,    CLARINET_ENONE },
        //{ 27,  any,  loopback,  3328,  2048, 1,    CLARINET_ENONE },
        //{ 28,  any,  loopback, 93667, 65507, 1,    CLARINET_ENONE },
        //{ 29,  any,  loopback, 93667, 65535, 1, CLARINET_EMSGSIZE },
#if CLARINET_ENABLE_IPV6
        //{ 30, any6, loopback6, 93667, 65507, 1,    CLARINET_ENONE },
        //{ 31, any6, loopback6, 93667, 65527, 1,    CLARINET_ENONE }, // max udp payload size in ipv6 does not count the ipv6 header size
        //{ 32, any6, loopback6, 93667, 65535, 1, CLARINET_EMSGSIZE },
#endif
    };
    // @formatter:on

    int index;
    clarinet_endpoint source;
    clarinet_endpoint destination;
    int send_buffer_size;
    int send_length;
    int count;
    int expected;

    std::tie(index, source, destination,
             send_buffer_size, send_length,
             count,
             expected) = GENERATE_REF(from_range(data));

    FROM(index);

    // sanity: avoid obvious invalid test params
    REQUIRE(send_length <= INT_MAX);
    REQUIRE(count > 0);
    REQUIRE(expected <= (int)send_length);

    clarinet_udp_settings settings = clarinet_udp_settings_default;
    settings.send_buffer_size = (uint32_t)send_buffer_size;
    settings.ttl = 4;

    clarinet_udp_socket* sender = nullptr;
    int errcode = clarinet_udp_open(&sender, &source, &settings, 0);
    const finalizer sender_dtor = [&]
    {
        clarinet_udp_close(&sender);
    };
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    REQUIRE(sender != nullptr);

    void* send_buf = malloc((size_t)send_length);
    REQUIRE(send_buf != nullptr);
    const finalizer send_buf_dtor = [&]
    {
        free(send_buf);
    };

    for (int packet = 0; packet < count; ++packet)
    {
        FROM(packet);

        int n = clarinet_udp_send(sender, send_buf, (size_t)send_length, &destination);
        if (n > 0)
        {
            REQUIRE(Error(n) == Error(send_length));
        }
        else
        {
            REQUIRE(Error(n) == Error(expected));
        }
    }
}
#if 0

// TODO: define separate TEST cases for Windows, Linux and macOS highlighting their particularities.

TEST_CASE("Recv", "[udp]")
{
    const clarinet_endpoint loopback = { clarinet_addr_loopback_ipv4, 0 };
#if CLARINET_ENABLE_IPV6
    const clarinet_endpoint loopback6 = { clarinet_addr_loopback_ipv6, 0 };
#endif

    // @formatter:off
    // Table: index | source | destination | send buffer size | send length | recv buffer size | recv length | count | expected
    const std::vector<std::tuple<int, clarinet_endpoint, clarinet_endpoint, int, int, int, int, int, int>> data = {
        {  0,   loopback,     loopback,  2048,     0, 65535,   65535,  1,       CLARINET_ENONE },
        {  1,   loopback,     loopback,  2048,     1, 65535,   65535,  3,       CLARINET_ENONE },
        {  2,   loopback,     loopback,  2048,     2, 65535,   65535,  3,       CLARINET_ENONE },
        {  3,   loopback,     loopback,  2048,     4, 65535,   65535,  3,       CLARINET_ENONE },
        {  4,   loopback,     loopback,  2048,     8, 65535,   65535,  3,       CLARINET_ENONE },
        {  5,   loopback,     loopback,  2048,    16, 65535,   65535,  3,       CLARINET_ENONE },
        {  6,   loopback,     loopback,  2048,    32, 65535,   65535,  3,       CLARINET_ENONE },
        {  7,   loopback,     loopback,  2048,    64, 65535,   65535,  2,       CLARINET_ENONE },
        {  8,   loopback,     loopback,  2048,   128, 65535,   65535,  2,       CLARINET_ENONE },
        {  9,   loopback,     loopback,  2048,   256, 65535,   65535,  2,       CLARINET_ENONE },
        { 10,   loopback,     loopback,  2048,   512, 65535,   65535,  1,       CLARINET_ENONE },
        { 11,   loopback,     loopback,  2048,  1024,  2304,   65535,  1,       CLARINET_ENONE },
        { 12,   loopback,     loopback,  3328,  2048, 65535,   65535,  1,       CLARINET_ENONE },
        { 13,   loopback,     loopback,  3328,  2048,  2048,    2047,  1,    CLARINET_EMSGSIZE },
        { 14,   loopback,     loopback,  3328,  2048,  2048,    2048,  2, CLARINET_EWOULDBLOCK },
        // On Linux x64 an ipv4 datagram with 2048 bytes consume 4352 bytes from the buffer but in this test the NIC is
        // not under stress so the first datagram arrives bypassing the buffer and the actual recv buffer space required
        // is 4352 * 1 = 4352
        { 15,   loopback,     loopback,  2047,  2048,  4352,    2048,  2, CLARINET_EWOULDBLOCK },
        // On Linux x64 an ipv4 datagram with 1024 bytes consume 2304 bytes from the buffer but in this test the NIC is
        // not under stress so the first datagram arrives bypassing the buffer and the actual recv buffer space required
        // is 2304 * 9 = 20736
        { 15,   loopback,     loopback, 16640,  1024, 23040,   65535, 10,       CLARINET_ENONE },
        // On Linux x64 an ipv4 datagram with 512 bytes consume 1280 bytes from the buffer but in this test the NIC is
        // not under stress so the first datagram arrives bypassing the buffer and the actual recv buffer space required
        // is 1280 * 9 = 115200
        { 16,   loopback,     loopback, 11520,  512,  12800,   65535, 10,       CLARINET_ENONE }, // On Linux x64 an ipv4 datagram with 512 bytes consume 1280 bytes from the buffer
        { 17,   loopback,     loopback, 93667, 65507, 65507,   65507,  1,       CLARINET_ENONE },
        { 18,   loopback,     loopback, 93667, 65507, 65507,   65507,  1,       CLARINET_ENONE },
        { 19,   loopback,     loopback, 93667, 65507, 65535,   65535,  1,       CLARINET_ENONE },
        { 20,   loopback,     loopback, 93667, 65507,  2048,   65535,  1, CLARINET_EWOULDBLOCK },
        { 21,   loopback,     loopback, 93667, 65507, 65535,   65506,  1,    CLARINET_EMSGSIZE },
        { 22,   loopback,     loopback, 93667, 65507, 65535,   65507,  1,                65507 },
        { 23,   loopback,     loopback, 93667, 65507, 65535,   65535,  1,                65507 },
        { 24,   loopback,     loopback, 93667, 65507, 65535, 1048576,  1,                65507 },
#if CLARINET_ENABLE_IPV6
        { 25,  loopback6,    loopback6, 93667, 65527, 65527,   65527,  1,       CLARINET_ENONE },
        { 26,  loopback6,    loopback6, 93667, 65527, 65527,   65527,  1,       CLARINET_ENONE },
        { 27,  loopback6,    loopback6, 93667, 65527, 65535,   65535,  1,       CLARINET_ENONE },
        { 28,  loopback6,    loopback6, 93667, 65527,  2048,   65535,  1, CLARINET_EWOULDBLOCK },
        { 29,  loopback6,    loopback6, 93667, 65527, 65535,   65526,  1,    CLARINET_EMSGSIZE },
        { 30,  loopback6,    loopback6, 93667, 65527, 65535,   65527,  1,                65507 },
        { 31,  loopback6,    loopback6, 93667, 65527, 65535,   65535,  1,                65507 },
        { 32,  loopback6,    loopback6, 93667, 65527, 65535, 1048576,  1,                65507 },
#endif
        };
    // @formatter:on

    int index;
    clarinet_endpoint source;
    clarinet_endpoint destination;
    int send_buffer_size;
    int send_length;
    int recv_buffer_size;
    int recv_length;
    int count;
    int expected;

    std::tie(index, source, destination,
             send_buffer_size, send_length,
             recv_buffer_size, recv_length,
             count,
             expected) = GENERATE_REF(from_range(data));

    FROM(index);

    // sanity: avoid obvious invalid test params
    REQUIRE(send_length <= INT_MAX);
    REQUIRE(recv_length <= INT_MAX);
    REQUIRE(count > 0);
    REQUIRE(expected <= (int)send_length);

    clarinet_udp_settings settings = clarinet_udp_settings_default;
    settings.send_buffer_size = (uint32_t)send_buffer_size;
    settings.recv_buffer_size = (uint32_t)recv_buffer_size;
    settings.ttl = 4;

    clarinet_udp_socket* sender = nullptr;
    int errcode = clarinet_udp_open(&sender, &source, &settings, 0);
    const finalizer sender_dtor = [&]
    {
        clarinet_udp_close(&sender);
    };
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    REQUIRE(sender != nullptr);

    const size_t send_buf_len = (size_t)send_length * count;
    void* send_buf = malloc(send_buf_len);
    REQUIRE(send_buf != nullptr);
    const finalizer send_buf_dtor = [&]
    {
        free(send_buf);
    };

    memnoise(send_buf, send_buf_len);

    clarinet_udp_socket* receiver = nullptr;
    errcode = clarinet_udp_open(&receiver, &destination, &settings, 0);
    const finalizer receiver_dtor = [&]
    {
        clarinet_udp_close(&receiver);
    };
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    REQUIRE(receiver != nullptr);

    void* recv_buf = malloc((size_t)recv_length);
    REQUIRE(recv_buf != nullptr);
    const finalizer recv_buf_dtor = [&]
    {
        free(recv_buf);
    };

    errcode = clarinet_udp_get_endpoint(receiver, &destination);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

    for (int packet = 0; packet < count; ++packet)
    {
        FROM(packet);

        int n = clarinet_udp_send(sender, (void*)((uint8_t*)send_buf + (send_length * packet)), (size_t)send_length, &destination);
        REQUIRE(Error(n) == Error(send_length));
    }

    for (int packet = 0; packet < count; ++packet)
    {
        FROM(packet);

        int n = clarinet_udp_recv(receiver, recv_buf, (size_t)recv_length, &source);
        if (n > 0)
        {
            REQUIRE(Error(n) == Error(send_length));
            REQUIRE_THAT(memory(recv_buf, (size_t)n), Equals(memory((void*)((uint8_t*)send_buf + (send_length * packet)), (size_t)send_length)));
        }
        else
        {
            REQUIRE(Error(n) == Error(expected));
        }
    }
}
#endif
TEST_CASE("Set/Get socket options", "[udp]")
{
    SECTION("Get CLARINET_SO_REUSEADDR")
    {

    }

    SECTION("Get CLARINET_SO_KEEPALIVE")
    {

    }

    SECTION("Get CLARINET_SO_IPV6DUAL")
    {

    }

    SECTION("Get CLARINET_SO_TTL")
    {

    }

    SECTION("Get CLARINET_SO_SNDBUF")
    {

    }

    SECTION("Get CLARINET_SO_RCVBUF")
    {

    }

    SECTION("Get CLARINET_SO_LINGER")
    {

    }

    SECTION("Get CLARINET_SO_DONTLINGER")
    {

    }

    SECTION("Set CLARINET_SO_REUSEADDR")
    {
        // TODO: should fail
    }

    SECTION("Set CLARINET_SO_KEEPALIVE")
    {

    }

    SECTION("Set CLARINET_SO_IPV6DUAL")
    {
        // TODO: should fail
    }

    SECTION("Set CLARINET_SO_TTL")
    {
        // TODO: should fail
    }

    SECTION("Set CLARINET_SO_SNDBUF")
    {
        // TODO: should fail
    }

    SECTION("Set CLARINET_SO_RCVBUF")
    {
        // TODO: should fail
    }

    SECTION("Set CLARINET_SO_LINGER")
    {

    }

    SECTION("Set CLARINET_SO_DONTLINGER")
    {

    }
}
