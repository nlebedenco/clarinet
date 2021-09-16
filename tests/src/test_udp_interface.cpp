#include "test.h"

#if !CLARINET_ENABLE_IPV6
    WARN("IPv6 not supported by the platform. Skipping IPV6 tests. ")
#endif

TEST_CASE("Open/Close", "[udp]")
{
    SECTION("Invalid Arguments")
    {
        SECTION("Open NULL socket")
        {
            clarinet_endpoint any = { clarinet_addr_ipv4_any, 0 };
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
            clarinet_endpoint any = { clarinet_addr_ipv4_any, 0 };
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
            clarinet_endpoint invalid = {{0}};     
            clarinet_udp_socket* socket = nullptr;            
            int errcode = clarinet_udp_open(&socket, &invalid, &clarinet_udp_settings_default, 0);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(socket == nullptr);

        }        
    }
    
    SECTION("Open one address once")
    {
        const std::vector<std::tuple<int, clarinet_endpoint>> data =
        {
            { 0, { clarinet_addr_ipv4_any,      0 } }, 
            { 1, { clarinet_addr_ipv4_loopback, 0 } },
        #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_ipv6_any,      0 } }, 
            { 3, { clarinet_addr_ipv6_loopback, 0 } },
        #endif
 
        };

        int index;
        clarinet_endpoint endpoint;
        std::tie(index, endpoint) = GENERATE_REF(from_range(data));

        FROM(index);
        
        clarinet_udp_socket* socket = nullptr;
        
        SECTION("Open same socket twice")
        {
            int errcode = clarinet_udp_open(&socket, &endpoint, &clarinet_udp_settings_default, 0);
            const finalizer onexit = [&] { clarinet_udp_close(&socket); };
            
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
            int errcode = clarinet_udp_open(&socket, &endpoint, &clarinet_udp_settings_default, clarinet_socket_option_to_flag(CLARINET_SO_IPV6DUAL));
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
        const std::vector<std::tuple<int, clarinet_endpoint>> data =
        {
            { 0, { clarinet_addr_ipv4_any,      0 } }, 
            { 1, { clarinet_addr_ipv4_loopback, 0 } },
        #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_ipv6_any,      0 } }, 
            { 3, { clarinet_addr_ipv6_loopback, 0 } },
        #endif            
        };

        int index;
        clarinet_endpoint endpoint;
        std::tie(index, endpoint) = GENERATE_REF(from_range(data));

        FROM(index);
        
        clarinet_udp_socket* sa = nullptr;
        int errcode = clarinet_udp_open(&sa, &endpoint, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(sa != nullptr);
        const finalizer onexita = [&] { clarinet_udp_close(&sa); };
            
        clarinet_udp_socket* sb = nullptr;        
        errcode = clarinet_udp_open(&sb, &endpoint, &clarinet_udp_settings_default, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(sb != nullptr);
        const finalizer onexitb = [&] { clarinet_udp_close(&sb); };
        
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
        const uint32_t NONE = 0;
        const uint32_t REUSEADDR = clarinet_socket_option_to_flag(CLARINET_SO_REUSEADDR);
        const uint32_t IPV6DUAL = clarinet_socket_option_to_flag(CLARINET_SO_IPV6DUAL);

        const std::vector<std::tuple<int, clarinet_addr, uint32_t, clarinet_addr, uint32_t, int>> data =
        {
            {  0, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv4_any,      NONE,               CLARINET_EADDRINUSE }, 
            {  1, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv4_loopback, NONE,               CLARINET_EADDRINUSE }, 
            {  2, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv4_any,      NONE,               CLARINET_EADDRINUSE },            
            {  3, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv4_loopback, NONE,               CLARINET_EADDRINUSE },               
            {  4, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_EADDRINUSE }, 
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            {  5, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_ENONE      }, // BSD/Darwin
            #else
            {  5, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_EADDRINUSE }, // Others
            #endif    
            #if defined(__linux__)
            {  6, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_EADDRINUSE }, // Linux
            #else
            {  6, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_ENONE      }, // Others
            #endif
            {  7, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_EADDRINUSE },            
            {  8, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv4_any,      NONE,               CLARINET_EADDRINUSE }, 
            #if defined(_WIN32)
            {  9, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv4_loopback, NONE,               CLARINET_ENONE      }, // Windows
            #else
            {  9, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv4_loopback, NONE,               CLARINET_EADDRINUSE }, // Others
            #endif
            { 10, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv4_any,      NONE,               CLARINET_EADDRINUSE },            
            { 11, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv4_loopback, NONE,               CLARINET_EADDRINUSE },            
            { 12, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_ENONE      }, 
            { 13, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_ENONE      }, 
            { 14, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_ENONE      },            
            { 15, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_ENONE      },
                                    
        #if CLARINET_ENABLE_IPV6                        
            { 16, clarinet_addr_ipv6_any,      NONE,               clarinet_addr_ipv6_any,      NONE,               CLARINET_EADDRINUSE }, 
            { 17, clarinet_addr_ipv6_any,      NONE,               clarinet_addr_ipv6_loopback, NONE,               CLARINET_EADDRINUSE }, 
            { 18, clarinet_addr_ipv6_loopback, NONE,               clarinet_addr_ipv6_any,      NONE,               CLARINET_EADDRINUSE },            
            { 19, clarinet_addr_ipv6_loopback, NONE,               clarinet_addr_ipv6_loopback, NONE,               CLARINET_EADDRINUSE },               
            { 20, clarinet_addr_ipv6_any,      NONE,               clarinet_addr_ipv6_any,      REUSEADDR,          CLARINET_EADDRINUSE }, 
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            { 21, clarinet_addr_ipv6_any,      NONE,               clarinet_addr_ipv6_loopback, REUSEADDR,          CLARINET_ENONE      }, // BSD/Darwin
            #else
            { 21, clarinet_addr_ipv6_any,      NONE,               clarinet_addr_ipv6_loopback, REUSEADDR,          CLARINET_EADDRINUSE }, // Others
            #endif    
            #if defined(__linux__)
            { 22, clarinet_addr_ipv6_loopback, NONE,               clarinet_addr_ipv6_any,      REUSEADDR,          CLARINET_EADDRINUSE }, // Linux           
            #else
            { 22, clarinet_addr_ipv6_loopback, NONE,               clarinet_addr_ipv6_any,      REUSEADDR,          CLARINET_ENONE      }, // Others
            #endif    
            { 23, clarinet_addr_ipv6_loopback, NONE,               clarinet_addr_ipv6_loopback, REUSEADDR,          CLARINET_EADDRINUSE },            
            { 24, clarinet_addr_ipv6_any,      REUSEADDR,          clarinet_addr_ipv6_any,      NONE,               CLARINET_EADDRINUSE }, 
            #if defined(_WIN32)
            { 25, clarinet_addr_ipv6_any,      REUSEADDR,          clarinet_addr_ipv6_loopback, NONE,               CLARINET_ENONE      }, // Windows
            #else
            { 25, clarinet_addr_ipv6_any,      REUSEADDR,          clarinet_addr_ipv6_loopback, NONE,               CLARINET_EADDRINUSE }, // Others
            #endif
            { 26, clarinet_addr_ipv6_loopback, REUSEADDR,          clarinet_addr_ipv6_any,      NONE,               CLARINET_EADDRINUSE },            
            { 27, clarinet_addr_ipv6_loopback, REUSEADDR,          clarinet_addr_ipv6_loopback, NONE,               CLARINET_EADDRINUSE },            
            { 28, clarinet_addr_ipv6_any,      REUSEADDR,          clarinet_addr_ipv6_any,      REUSEADDR,          CLARINET_ENONE      }, 
            { 29, clarinet_addr_ipv6_any,      REUSEADDR,          clarinet_addr_ipv6_loopback, REUSEADDR,          CLARINET_ENONE      }, 
            { 30, clarinet_addr_ipv6_loopback, REUSEADDR,          clarinet_addr_ipv6_any,      REUSEADDR,          CLARINET_ENONE      },            
            { 31, clarinet_addr_ipv6_loopback, REUSEADDR,          clarinet_addr_ipv6_loopback, REUSEADDR,          CLARINET_ENONE      },
                                    
            { 32, clarinet_addr_ipv6_any,      NONE,               clarinet_addr_ipv4_any,      NONE,               CLARINET_ENONE      }, 
            { 33, clarinet_addr_ipv6_any,      NONE,               clarinet_addr_ipv4_loopback, NONE,               CLARINET_ENONE      }, 
            { 34, clarinet_addr_ipv6_loopback, NONE,               clarinet_addr_ipv4_any,      NONE,               CLARINET_ENONE      },            
            { 35, clarinet_addr_ipv6_loopback, NONE,               clarinet_addr_ipv4_loopback, NONE,               CLARINET_ENONE      },               
            { 36, clarinet_addr_ipv6_any,      NONE,               clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_ENONE      }, 
            { 37, clarinet_addr_ipv6_any,      NONE,               clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_ENONE      }, 
            { 38, clarinet_addr_ipv6_loopback, NONE,               clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_ENONE      },            
            { 39, clarinet_addr_ipv6_loopback, NONE,               clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_ENONE      },            
            { 40, clarinet_addr_ipv6_any,      REUSEADDR,          clarinet_addr_ipv4_any,      NONE,               CLARINET_ENONE      }, 
            { 41, clarinet_addr_ipv6_any,      REUSEADDR,          clarinet_addr_ipv4_loopback, NONE,               CLARINET_ENONE      }, 
            { 42, clarinet_addr_ipv6_loopback, REUSEADDR,          clarinet_addr_ipv4_any,      NONE,               CLARINET_ENONE      },            
            { 43, clarinet_addr_ipv6_loopback, REUSEADDR,          clarinet_addr_ipv4_loopback, NONE,               CLARINET_ENONE      },            
            { 44, clarinet_addr_ipv6_any,      REUSEADDR,          clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_ENONE      }, 
            { 45, clarinet_addr_ipv6_any,      REUSEADDR,          clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_ENONE      }, 
            { 46, clarinet_addr_ipv6_loopback, REUSEADDR,          clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_ENONE      },            
            { 47, clarinet_addr_ipv6_loopback, REUSEADDR,          clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_ENONE      },
                                    
            { 48, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv6_any,      NONE,               CLARINET_ENONE      }, 
            { 49, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv6_loopback, NONE,               CLARINET_ENONE      }, 
            { 40, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv6_any,      NONE,               CLARINET_ENONE      },            
            { 51, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv6_loopback, NONE,               CLARINET_ENONE      },               
            { 52, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv6_any,      REUSEADDR,          CLARINET_ENONE      }, 
            { 53, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv6_loopback, REUSEADDR,          CLARINET_ENONE      }, 
            { 54, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv6_any,      REUSEADDR,          CLARINET_ENONE      },            
            { 55, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv6_loopback, REUSEADDR,          CLARINET_ENONE      },            
            { 56, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv6_any,      NONE,               CLARINET_ENONE      }, 
            { 57, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv6_loopback, NONE,               CLARINET_ENONE      }, 
            { 58, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv6_any,      NONE,               CLARINET_ENONE      },            
            { 59, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv6_loopback, NONE,               CLARINET_ENONE      },            
            { 60, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv6_any,      REUSEADDR,          CLARINET_ENONE      }, 
            { 61, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv6_loopback, REUSEADDR,          CLARINET_ENONE      }, 
            { 62, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv6_any,      REUSEADDR,          CLARINET_ENONE      },            
            { 63, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv6_loopback, REUSEADDR,          CLARINET_ENONE      },
            
        #if CLARINET_ENABLE_IPV6DUAL           
            { 64, clarinet_addr_ipv6_any,      IPV6DUAL,           clarinet_addr_ipv4_any,      NONE,               CLARINET_EADDRINUSE }, 
            { 65, clarinet_addr_ipv6_any,      IPV6DUAL,           clarinet_addr_ipv4_loopback, NONE,               CLARINET_EADDRINUSE }, 
            { 66, clarinet_addr_ipv6_loopback, IPV6DUAL,           clarinet_addr_ipv4_any,      NONE,               CLARINET_ENONE      },            
            { 67, clarinet_addr_ipv6_loopback, IPV6DUAL,           clarinet_addr_ipv4_loopback, NONE,               CLARINET_ENONE      },               
            { 68, clarinet_addr_ipv6_any,      IPV6DUAL,           clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_EADDRINUSE }, 
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            { 69, clarinet_addr_ipv6_any,      IPV6DUAL,           clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_ENONE      }, // BSD/Darwin 
            #else
            { 69, clarinet_addr_ipv6_any,      IPV6DUAL,           clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_EADDRINUSE }, // Others    
            #endif
            { 70, clarinet_addr_ipv6_loopback, IPV6DUAL,           clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_ENONE      },         
            { 71, clarinet_addr_ipv6_loopback, IPV6DUAL,           clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_ENONE      },
            #if defined(_WIN32)            
            { 72, clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, clarinet_addr_ipv4_any,      NONE,               CLARINET_ENONE      }, // Windows
            { 73, clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, clarinet_addr_ipv4_loopback, NONE,               CLARINET_ENONE      }, // Windows
            #else
            { 72, clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, clarinet_addr_ipv4_any,      NONE,               CLARINET_EADDRINUSE }, // Others
            { 73, clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, clarinet_addr_ipv4_loopback, NONE,               CLARINET_EADDRINUSE }, // Others    
            #endif
            { 74, clarinet_addr_ipv6_loopback, IPV6DUAL|REUSEADDR, clarinet_addr_ipv4_any,      NONE,               CLARINET_ENONE      },         
            { 75, clarinet_addr_ipv6_loopback, IPV6DUAL|REUSEADDR, clarinet_addr_ipv4_loopback, NONE,               CLARINET_ENONE      },         
            { 76, clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_ENONE      }, 
            { 77, clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_ENONE      }, 
            { 78, clarinet_addr_ipv6_loopback, IPV6DUAL|REUSEADDR, clarinet_addr_ipv4_any,      REUSEADDR,          CLARINET_ENONE      },         
            { 79, clarinet_addr_ipv6_loopback, IPV6DUAL|REUSEADDR, clarinet_addr_ipv4_loopback, REUSEADDR,          CLARINET_ENONE      }, 
            
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            { 80, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv6_any,      IPV6DUAL,           CLARINET_ENONE      }, // BSD/Darwin
            #else
            { 80, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv6_any,      IPV6DUAL,           CLARINET_EADDRINUSE }, // Others
            #endif    
            { 81, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv6_loopback, IPV6DUAL,           CLARINET_ENONE      },
            { 82, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv6_any,      IPV6DUAL,           CLARINET_EADDRINUSE },        
            { 83, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv6_loopback, IPV6DUAL,           CLARINET_ENONE      },    
            #if defined(__linux__)            
            { 84, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, CLARINET_EADDRINUSE }, // Linux
            #else
            { 84, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, CLARINET_ENONE      }, // Others
            #endif
            { 85, clarinet_addr_ipv4_any,      NONE,               clarinet_addr_ipv6_loopback, IPV6DUAL|REUSEADDR, CLARINET_ENONE      }, 
            #if defined(__linux__)
            { 86, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, CLARINET_EADDRINUSE }, // Linux
            #else
            { 86, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, CLARINET_ENONE      }, // Others
            #endif    
            { 87, clarinet_addr_ipv4_loopback, NONE,               clarinet_addr_ipv6_loopback, IPV6DUAL|REUSEADDR, CLARINET_ENONE      },   
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )            
            { 88, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv6_any,      IPV6DUAL,           CLARINET_ENONE      }, // BSD/Darwin
            #else
            { 88, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv6_any,      IPV6DUAL,           CLARINET_EADDRINUSE }, // Others    
            #endif    
            { 89, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv6_loopback, IPV6DUAL,           CLARINET_ENONE      },
            { 90, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv6_any,      IPV6DUAL,           CLARINET_EADDRINUSE },        
            { 91, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv6_loopback, IPV6DUAL,           CLARINET_ENONE      },        
            { 92, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, CLARINET_ENONE      },
            { 93, clarinet_addr_ipv4_any,      REUSEADDR,          clarinet_addr_ipv6_loopback, IPV6DUAL|REUSEADDR, CLARINET_ENONE      },
            { 94, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv6_any,      IPV6DUAL|REUSEADDR, CLARINET_ENONE      },        
            { 95, clarinet_addr_ipv4_loopback, REUSEADDR,          clarinet_addr_ipv6_loopback, IPV6DUAL|REUSEADDR, CLARINET_ENONE      },
        #endif
        #endif                                                
        };

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
        const finalizer onexita = [&] { clarinet_udp_close(&sa); };
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(sa != nullptr);
                
        clarinet_endpoint saendp;
        errcode = clarinet_udp_get_endpoint(sa, &saendp);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            
        clarinet_endpoint endpb = { addrb, saendp.port };    
        clarinet_udp_socket* sb = nullptr;
        errcode = clarinet_udp_open(&sb, &endpb, &clarinet_udp_settings_default, fb);
        const finalizer onexitb = [&] { clarinet_udp_close(&sb); };
        CHECK(Error(errcode) == Error(expected));
    }
}

TEST_CASE("Get Endpoint", "[udp]")
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
