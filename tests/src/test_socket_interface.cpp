#include "test.h"

#if defined(__linux__)
#include <fstream>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

static starter init([] // NOLINT(cert-err58-cpp)
{
    #if !CLARINET_ENABLE_IPV6
    WARN("IPv6 suport is disabled. Some IPV6 tests may be skipped. ");
    #endif
});

// Scope initialize and finalize the library
static autoload loader;

// All families compatible with clarinet_socket_open()
#define OPEN_COMPATIBLE_FAMILIES_COMMON \
CLARINET_AF_INET,

#if CLARINET_ENABLE_IPV6
#define OPEN_COMPATIBLE_FAMILIES OPEN_COMPATIBLE_FAMILIES_COMMON CLARINET_AF_INET6,
#else
#define OPEN_COMPATIBLE_FAMILIES OPEN_COMPATIBLE_FAMILIES_COMMON
#endif

// All families incompatible with clarinet_socket_open()
#define OPEN_INCOMPATIBLE_FAMILIES_COMMON \
CLARINET_AF_UNSPEC, \
CLARINET_AF_LINK,

#if CLARINET_ENABLE_IPV6
#define OPEN_INCOMPATIBLE_FAMILIES OPEN_INCOMPATIBLE_FAMILIES_COMMON
#else
#define OPEN_INCOMPATIBLE_FAMILIES OPEN_INCOMPATIBLE_FAMILIES_COMMON CLARINET_AF_INET6,
#endif

// All protocols compatible with clarinet_socket_open()
#define OPEN_COMPATIBLE_PROTOCOLS \
CLARINET_PROTO_UDP, \
CLARINET_PROTO_TCP,

// All protocols incompatible with clarinet_socket_open()
#define OPEN_INCOMPATIBLE_PROTOCOLS \
CLARINET_PROTO_NONE, \
CLARINET_PROTO_DTLC, \
CLARINET_PROTO_DTLS, \
CLARINET_PROTO_TLS, \
CLARINET_PROTO_GDTP, \
CLARINET_PROTO_GDTPS, \
CLARINET_PROTO_ENET, \
CLARINET_PROTO_ENETS,

// All protocols compatible with clarinet_socket_listen(), clarinet_socket_accept() and clarinet_socket_connect()
#define LISTEN_COMPATIBLE_PROTOCOLS \
CLARINET_PROTO_TCP,

// All protocols incompatible with clarinet_socket_listen()
#define LISTEN_INCOMPATIBLE_PROTOCOLS \
CLARINET_PROTO_NONE, \
CLARINET_PROTO_UDP, \
CLARINET_PROTO_DTLC, \
CLARINET_PROTO_DTLS, \
CLARINET_PROTO_TLS, \
CLARINET_PROTO_GDTP, \
CLARINET_PROTO_GDTPS, \
CLARINET_PROTO_ENET, \
CLARINET_PROTO_ENETS,

TEST_CASE("Initialize")
{
    SECTION("Memory unmodified")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);
        REQUIRE(sp->family == CLARINET_AF_UNSPEC);
        REQUIRE(sp->proto == CLARINET_PROTO_NONE);
    }

    SECTION("Memory with all zeroes")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        memset(sp, 0, sizeof(clarinet_socket));

        clarinet_socket_init(sp);
        REQUIRE(sp->family == CLARINET_AF_UNSPEC);
        REQUIRE(sp->proto == CLARINET_PROTO_NONE);
    }

    SECTION("Memory with all ones")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        memset(sp, 0xFF, sizeof(clarinet_socket));

        clarinet_socket_init(sp);
        REQUIRE(sp->family == CLARINET_AF_UNSPEC);
        REQUIRE(sp->proto == CLARINET_PROTO_NONE);
    }
}

TEST_CASE("Open/Close")
{
    SECTION("NULL socket")
    {
        clarinet_family family = GENERATE(values({ OPEN_COMPATIBLE_FAMILIES OPEN_INCOMPATIBLE_FAMILIES }));
        clarinet_proto proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS OPEN_INCOMPATIBLE_PROTOCOLS }));

        FROM(family);
        FROM(proto);

        int errcode = clarinet_socket_open(nullptr, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("INITIALIZED socket with INCOMPATIBLE family")
    {
        clarinet_family family = GENERATE(values({ OPEN_INCOMPATIBLE_FAMILIES }));
        clarinet_proto proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS OPEN_INCOMPATIBLE_PROTOCOLS }));

        FROM(family);
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);
        REQUIRE(sp->family == CLARINET_AF_UNSPEC);
        REQUIRE(sp->proto == CLARINET_PROTO_NONE);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_EAFNOSUPPORT));
    }

    SECTION("INITIALIZED socket with INCOMPATIBLE protocol")
    {
        clarinet_family family = GENERATE(values({ OPEN_COMPATIBLE_FAMILIES }));
        clarinet_proto proto = GENERATE(values({ OPEN_INCOMPATIBLE_PROTOCOLS }));

        FROM(family);
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_EPROTONOSUPPORT));
    }

    SECTION("INITIALIZED socket with COMPATIBLE family and protocol")
    {
        clarinet_family family = GENERATE(values({ OPEN_COMPATIBLE_FAMILIES }));
        clarinet_proto proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS }));

        FROM(family);
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        CHECK(sp->family == family);
        CHECK(sp->proto == proto);

        errcode = clarinet_socket_close(sp);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    }

    SECTION("SAME socket twice")
    {
        clarinet_family family = GENERATE(values({ OPEN_COMPATIBLE_FAMILIES }));
        clarinet_proto proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS }));

        FROM(family);
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_open(sp, family, proto);
        CHECK(Error(errcode) == Error(CLARINET_EINVAL));

        errcode = clarinet_socket_close(sp);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    }

    SECTION("Close NULL socket")
    {
        int errcode = clarinet_socket_close(nullptr);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("Close EMPTY socket")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_close(&socket);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("Close SAME socket twice")
    {
        clarinet_family family = GENERATE(values({ OPEN_COMPATIBLE_FAMILIES }));
        clarinet_proto proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS }));

        FROM(family);
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_close(sp);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        /* It should be safe to close a socket multiple times as long as the previous attempt did not fail */
        errcode = clarinet_socket_close(sp);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }
}

TEST_CASE("Get/Set Option")
{
    const int VAL_INIT = -1; /* 0xFFFFFFFF */
    const size_t LEN_INIT = 4 * sizeof(int);

    SECTION("NULL socket")
    {
        int val = VAL_INIT;
        size_t len = LEN_INIT;

        int errcode = clarinet_socket_getopt(nullptr, 0, &val, &len);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        REQUIRE(val == VAL_INIT);
        REQUIRE(len == LEN_INIT);

        errcode = clarinet_socket_getopt(nullptr, 0, nullptr, nullptr);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));

        errcode = clarinet_socket_setopt(nullptr, 0, &val, sizeof(val));
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        REQUIRE(val == VAL_INIT);

        errcode = clarinet_socket_setopt(nullptr, 0, nullptr, 0);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("INITIALIZED socket")
    {
        clarinet_family family = GENERATE(values({ OPEN_COMPATIBLE_FAMILIES }));
        clarinet_proto proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS }));

        FROM(family);
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        const int on = 1;
        const int off = 0;

        SECTION("Invalid optname")
        {
            int val = VAL_INIT;
            size_t len = LEN_INIT;

            errcode = clarinet_socket_getopt(sp, 0, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(val == VAL_INIT);
            REQUIRE(len == LEN_INIT);

            errcode = clarinet_socket_getopt(sp, 0, nullptr, nullptr);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(val == VAL_INIT);
            REQUIRE(len == LEN_INIT);

            errcode = clarinet_socket_setopt(sp, 0, &val, sizeof(val));
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            REQUIRE(val == VAL_INIT);

            errcode = clarinet_socket_setopt(sp, 0, nullptr, 0);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("Valid optname")
        {
            int val = VAL_INIT;
            size_t len = LEN_INIT;

            int optname = GENERATE(CLARINET_SO_NONBLOCK, CLARINET_SO_REUSEADDR, CLARINET_SO_SNDBUF, CLARINET_SO_RCVBUF,
                CLARINET_SO_SNDTIMEO, CLARINET_SO_RCVTIMEO, CLARINET_SO_KEEPALIVE,
                CLARINET_SO_LINGER, CLARINET_SO_DONTLINGER);

            SECTION("NULL optval")
            {
                errcode = clarinet_socket_getopt(sp, optname, nullptr, nullptr);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));

                errcode = clarinet_socket_getopt(sp, optname, nullptr, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(len == LEN_INIT);

                errcode = clarinet_socket_setopt(sp, optname, nullptr, sizeof(val));
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));

                errcode = clarinet_socket_setopt(sp, optname, nullptr, 0);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            }

            SECTION("NULL optlen")
            {
                errcode = clarinet_socket_getopt(sp, optname, &val, nullptr);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(val == VAL_INIT);
            }

            SECTION("Invalid optlen")
            {
                int target = GENERATE(0, 1, 2, 3);

                len = (size_t)target;
                errcode = clarinet_socket_getopt(sp, optname, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(val == VAL_INIT);
                REQUIRE(len == (size_t)target);

                errcode = clarinet_socket_setopt(sp, optname, &val, target);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(val == VAL_INIT);
            }
        }

        SECTION("CLARINET_SO_NONBLOCK")
        {
            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            /* All platforms are expected to open the socket in blocking mode */
            errcode = clarinet_socket_getopt(sp, CLARINET_SO_NONBLOCK, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE_FALSE(val);
            REQUIRE(len == sizeof(val));

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_NONBLOCK, &on, sizeof(on));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_NONBLOCK, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val);
            REQUIRE(len == sizeof(val));

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_NONBLOCK, &off, sizeof(off));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_NONBLOCK, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE_FALSE(val);
            REQUIRE(len == sizeof(val));
        }

        SECTION("CLARINET_SO_REUSEADDR")
        {
            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            /* All platforms are expected to open the socket without address reuse */
            errcode = clarinet_socket_getopt(sp, CLARINET_SO_REUSEADDR, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE_FALSE(val);
            REQUIRE(len == sizeof(val));

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_REUSEADDR, &on, sizeof(on));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_REUSEADDR, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val);
            REQUIRE(len == sizeof(val));

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_REUSEADDR, &off, sizeof(off));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_REUSEADDR, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE_FALSE(val);
            REQUIRE(len == sizeof(val));
        }

        SECTION("CLARINET_SO_SNDBUF")
        {
            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            /* Every platform has a different default buffer size but all are > 0 */
            errcode = clarinet_socket_getopt(sp, CLARINET_SO_SNDBUF, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val > 0);
            REQUIRE(len == sizeof(val));

            /* These are safe values supposed to be between the minimum and maximum supported by all platforms so a call
             * to clarinet_socket_getopt() will return the expected value.
             *
             * Minimum SO_SNDBUF is:
             *  - WINDOWS: 0 (effectively disables the buffer and leaves only the net driver queue)
             *  - LINUX (x64): 4608
             *  - BSD/DARWIN: 1
             *
             * Maximum SO_SNDBUF is:
             *  - WINDOWS: 2147483648 but in practice it is bounded by available memory
             *  - LINUX (x64): 212992 (defined by net.core.wmem_max)
             *  - BSD/DARWIN: 2097152 (defined by kern.ipc.maxsockbuf)
             */
            int target = GENERATE(8191, 8192, 16383, 16384);
            FROM(target);

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_SNDBUF, &target, sizeof(target));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_SNDBUF, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            #if defined(__linux__) /* linux is special as we always round down odd buffer sizes to the nearest even number */
            REQUIRE(val == (target & -2)); /* target & 0XFFFFFFFE */
            #else
            REQUIRE(val == target);
            #endif // defined(__linux__)
            REQUIRE(len == sizeof(val));
        }

        SECTION("CLARINET_SO_RCVBUF")
        {
            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            /* Every platform has a different default buffer size but all are > 0 */
            errcode = clarinet_socket_getopt(sp, CLARINET_SO_RCVBUF, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val > 0);
            REQUIRE(len == sizeof(val));

            /* These are safe values supposed to be between the minimum and maximum supported by all platforms so a call
             * to clarinet_socket_getopt() will return the expected value.
             *
             * Minimum SO_RCVBUF is:
             *  - WINDOWS: 0 (effectively disables the buffer and leaves only the net driver queue)
             *  - LINUX (x64): 2292
             *  - BSD/DARWIN: 1
             *
             * Maximum SO_RCVBUF is:
             *  - WINDOWS: 2147483648 but in practice it is bounded by available memory
             *  - LINUX (x64): 212992 (defined by net.core.wmem_max)
             *  - BSD/DARWIN: 2097152 (defined by kern.ipc.maxsockbuf)
             */
            int target = GENERATE(8191, 8192, 16383, 16384);
            FROM(target);

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_RCVBUF, &target, sizeof(target));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_RCVBUF, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            #if defined(__linux__) /* linux is special as we always round down odd buffer sizes to the nearest even number */
            REQUIRE(val == (target & -2)); /* target & 0XFFFFFFFE */
            #else
            REQUIRE(val == target);
            #endif // defined(__linux__)
            REQUIRE(len == sizeof(val));
        }

        SECTION("CLARINET_SO_SNDTIMEO")
        {
            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            /* All platforms are expected to have a default timeout of 0. */
            errcode = clarinet_socket_getopt(sp, CLARINET_SO_SNDTIMEO, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val == 0);
            REQUIRE(len == sizeof(val));

            int target = GENERATE(0, 10, 250, 500, 1000, 5000, 60000);
            FROM(target);

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_SNDTIMEO, &target, sizeof(target));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_SNDTIMEO, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val == target);
            REQUIRE(len == sizeof(val));
        }

        SECTION("CLARINET_SO_RCVTIMEO")
        {
            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            /* All platforms are expected to have a default timeout of 0. */
            errcode = clarinet_socket_getopt(sp, CLARINET_SO_RCVTIMEO, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val == 0);
            REQUIRE(len == sizeof(val));

            int target = GENERATE(0, 10, 250, 500, 1000, 5000, 60000);
            FROM(target);

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_RCVTIMEO, &target, sizeof(target));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_RCVTIMEO, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val == target);
            REQUIRE(len == sizeof(val));
        }

        SECTION("CLARINET_SO_KEEPALIVE")
        {
            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            /* All platforms are expected to open the socket keep alive off */
            errcode = clarinet_socket_getopt(sp, CLARINET_SO_KEEPALIVE, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE_FALSE(val);

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_KEEPALIVE, &on, sizeof(on));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_KEEPALIVE, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val);
            REQUIRE(len == sizeof(val));

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_KEEPALIVE, &off, sizeof(off));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_KEEPALIVE, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE_FALSE(val);
            REQUIRE(len == sizeof(val));
        }

        SECTION("CLARINET_SO_LINGER")
        {
            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            clarinet_linger linger;
            memset(&linger, 0xFF, sizeof(linger));
            size_t lingerlen = sizeof(linger);

            /* All platforms are expected to open the socket without linger */
            errcode = clarinet_socket_getopt(sp, CLARINET_SO_LINGER, &linger, &lingerlen);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(lingerlen == sizeof(clarinet_linger));
            REQUIRE_FALSE(linger.enabled);

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_DONTLINGER, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val);
            REQUIRE(len == sizeof(val));

            int target = GENERATE(0, 5, 10, 250, 500, 1000, 5000, 65535);
            FROM(target);

            linger.enabled = true;
            linger.seconds = (uint16_t)target;
            errcode = clarinet_socket_setopt(sp, CLARINET_SO_LINGER, &linger, sizeof(linger));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(linger.enabled);
            REQUIRE(linger.seconds == target);

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_LINGER, &linger, &lingerlen);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(lingerlen == sizeof(clarinet_linger));
            REQUIRE(linger.enabled);
            REQUIRE(linger.seconds == target);

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_DONTLINGER, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE_FALSE(val);
            REQUIRE(len == sizeof(val));

            linger.enabled = false;
            errcode = clarinet_socket_setopt(sp, CLARINET_SO_LINGER, &linger, sizeof(linger));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE_FALSE(linger.enabled);
            REQUIRE(linger.seconds == target);

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_LINGER, &linger, &lingerlen);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(lingerlen == sizeof(clarinet_linger));
            REQUIRE_FALSE(linger.enabled);
            REQUIRE(linger.seconds == target);

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_DONTLINGER, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val);
            REQUIRE(len == sizeof(val));
        }

        SECTION("CLARINET_SO_DONTLINGER")
        {
            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            clarinet_linger linger;
            memset(&linger, 0xFF, sizeof(linger));
            size_t lingerlen = sizeof(linger);

            /* All platforms are expected to open the socket without linger this means ON */
            errcode = clarinet_socket_getopt(sp, CLARINET_SO_DONTLINGER, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val);
            REQUIRE(len == sizeof(val));

            /* All platforms are expected to open the socket without linger */
            errcode = clarinet_socket_getopt(sp, CLARINET_SO_LINGER, &linger, &lingerlen);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(lingerlen == sizeof(clarinet_linger));
            REQUIRE_FALSE(linger.enabled);

            uint16_t seconds = linger.seconds;

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_DONTLINGER, &off, sizeof(off));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_DONTLINGER, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE_FALSE(val);
            REQUIRE(len == sizeof(val));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_LINGER, &linger, &lingerlen);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(lingerlen == sizeof(clarinet_linger));
            REQUIRE(linger.enabled);
            REQUIRE(linger.seconds == seconds);

            seconds = 5; /* arbitrary non-zero value - we just want to check that CLARINET_SO_DONTLINGER does not affect the timeout previously set here */
            linger.seconds = seconds;
            errcode = clarinet_socket_setopt(sp, CLARINET_SO_LINGER, &linger, sizeof(linger));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(linger.enabled);
            REQUIRE(linger.seconds == seconds);

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_DONTLINGER, &on, sizeof(on));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_DONTLINGER, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val);
            REQUIRE(len == sizeof(val));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_LINGER, &linger, &lingerlen);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(lingerlen == sizeof(clarinet_linger));
            REQUIRE_FALSE(linger.enabled);
            REQUIRE(linger.seconds == seconds);

            errcode = clarinet_socket_setopt(sp, CLARINET_SO_DONTLINGER, &off, sizeof(off));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_DONTLINGER, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE_FALSE(val);
            REQUIRE(len == sizeof(val));

            errcode = clarinet_socket_getopt(sp, CLARINET_SO_LINGER, &linger, &lingerlen);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(lingerlen == sizeof(clarinet_linger));
            REQUIRE(linger.enabled);
            REQUIRE(linger.seconds == seconds);
        }

        SECTION("CLARINET_IP_V6ONLY")
        {
            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            #if CLARINET_ENABLE_IPV6

            errcode = clarinet_socket_getopt(sp, CLARINET_IP_V6ONLY, &val, &len);
            if (sp->family == CLARINET_AF_INET6)
            {
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                /* Each platform has a specific default initial values assuming unmodified system configurations. */
                #if defined(_WIN32)
                REQUIRE(val);
                #elif defined(__linux__)
                REQUIRE_FALSE(val);
                #elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
                REQUIRE_FALSE(val);
                #else
                REQUIRE_FALSE(val);
                #endif
                REQUIRE(len == sizeof(val));
            }
            else
            {
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            }

            errcode = clarinet_socket_setopt(sp, CLARINET_IP_V6ONLY, &on, sizeof(on));
            if (sp->family == CLARINET_AF_INET6)
            {
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                errcode = clarinet_socket_getopt(sp, CLARINET_IP_V6ONLY, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE(val);
                REQUIRE(len == sizeof(val));

                errcode = clarinet_socket_setopt(sp, CLARINET_IP_V6ONLY, &off, sizeof(off));
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                errcode = clarinet_socket_getopt(sp, CLARINET_IP_V6ONLY, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE_FALSE(val);
                REQUIRE(len == sizeof(val));
            }
            else
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));

            #else // CLARINET_ENABLE_IPV6

            errcode = clarinet_socket_getopt(sp, CLARINET_IP_V6ONLY, &val, &len);
            CHECK(Error(errcode) == Error(CLARINET_ENOTSUP));

            errcode = clarinet_socket_setopt(sp, CLARINET_IP_V6ONLY, &on, sizeof(on));
            CHECK(Error(errcode) == Error(CLARINET_ENOTSUP));

            #endif // CLARINET_ENABLE_IPV6
        }

        SECTION("CLARINET_IP_TTL")
        {
            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            /* Every platform has a different default TTL but all are > 0 */
            errcode = clarinet_socket_getopt(sp, CLARINET_IP_TTL, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val > 0);
            REQUIRE(len == sizeof(val));

            int target = GENERATE(32, 64, 128);
            FROM(target);

            errcode = clarinet_socket_setopt(sp, CLARINET_IP_TTL, &target, sizeof(target));
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_getopt(sp, CLARINET_IP_TTL, &val, &len);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(val == target);
            REQUIRE(len == sizeof(val));
        }

        SECTION("CLARINET_IP_MTU")
        {
            #define CLARINET_IP_MTU_TEST_VALUES 0, 32, 576, 1280, 1500, 4096

            int val = -1; /* 0xFFFFFFFF */
            size_t len = sizeof(val);

            SECTION("Disconnected")
            {
                SECTION("Get")
                {
                    /* MTU cannot be retrieved until the scoket is connected */
                    errcode = clarinet_socket_getopt(sp, CLARINET_IP_MTU, &val, &len);
                    REQUIRE(Error(errcode) == Error(CLARINET_ENOTCONN));
                }

                SECTION("Set")
                {
                    /* All attempts to set CLARINET_IP_MTU should fail since it is a read-only option */
                    int target = GENERATE(CLARINET_IP_MTU_TEST_VALUES);
                    FROM(target);

                    errcode = clarinet_socket_setopt(sp, CLARINET_IP_MTU, &target, sizeof(target));
                    REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                }
            }

            SECTION("Connected")
            {
                /* TODO: connect */

                SECTION("Get")
                {
                    errcode = clarinet_socket_getopt(sp, CLARINET_IP_MTU, &val, &len);
                    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                    REQUIRE(val > 0);
                    REQUIRE(len == sizeof(val));
                }

                SECTION("Set")
                {
                    /* All attempts to set CLARINET_IP_MTU should fail since it is a read-only option */
                    int target = GENERATE(CLARINET_IP_MTU_TEST_VALUES);
                    FROM(target);

                    errcode = clarinet_socket_setopt(sp, CLARINET_IP_MTU, &target, sizeof(target));
                    REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                }
            }

            #undef CLARINET_IP_MTU_TEST_VALUES
        }

        SECTION("CLARINET_IP_PMTUD")
        {
        }
    }
}

TEST_CASE("Bind")
{
    SECTION("ONE socket once")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> data = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
        #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
        #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(data);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(data));
        clarinet_proto proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS }));

        FROM(sample);
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        errcode = clarinet_socket_bind(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    }

    SECTION("ONE socket TWICE")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> data = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
        #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
        #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(data);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(data));
        clarinet_proto proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS }));

        FROM(sample);
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        errcode = clarinet_socket_bind(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_bind(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("ONE socket with dual stack")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> data = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
        #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
        #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(data);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(data));
        clarinet_proto proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS }));

        FROM(sample);
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int family = endpoint.addr.family;
        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        const int off = 1;
        #if CLARINET_ENABLE_IPV6DUAL
        const int expected = (family == CLARINET_AF_INET6) ? CLARINET_ENONE : CLARINET_EINVAL;
        #else
        const int expected = CLARINET_EINVAL;
        #endif // CLARINET_ENABLE_IPV6DUAL
        errcode = clarinet_socket_setopt(sp, CLARINET_IP_V6ONLY, &off, sizeof(off));
        REQUIRE(Error(errcode) == Error(expected));

        errcode = clarinet_socket_bind(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    }

    SECTION("TWO sockets with SAME address and port but different protocols")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> data = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
        #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
        #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(data);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(data));

        FROM(sample);

        clarinet_socket sa;
        clarinet_socket* spa = &sa;
        clarinet_socket_init(spa);

        /* Any two protocols should do here. There is no need to test all combinations. */
        int proto_a = GENERATE(CLARINET_PROTO_UDP);
        int proto_b = GENERATE(CLARINET_PROTO_TCP);

        FROM(proto_a);
        FROM(proto_b);

        int errcode = clarinet_socket_open(spa, endpoint.addr.family, proto_a);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexita = finalizer([&spa]
        {
            clarinet_socket_close(spa);
        });

        errcode = clarinet_socket_bind(spa, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_endpoint local;
        errcode = clarinet_socket_local_endpoint(spa, &local);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        endpoint.port = local.port;

        clarinet_socket sb;
        clarinet_socket* spb = &sb;
        clarinet_socket_init(spb);

        errcode = clarinet_socket_open(spb, endpoint.addr.family, proto_b);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexitb = finalizer([&spb]
        {
            clarinet_socket_close(spb);
        });

        errcode = clarinet_socket_bind(spb, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);
    }

    SECTION("TWO sockets with SAME address and protocol but different ports")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> data = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
        #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
        #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(data);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(data));
        clarinet_proto proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS }));

        FROM(sample);
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int family = endpoint.addr.family;
        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        const int off = 1;
        #if CLARINET_ENABLE_IPV6DUAL
        const int expected = (family == CLARINET_AF_INET6) ? CLARINET_ENONE : CLARINET_EINVAL;
        #else
        const int expected = CLARINET_EINVAL;
        #endif // CLARINET_ENABLE_IPV6DUAL
        errcode = clarinet_socket_setopt(sp, CLARINET_IP_V6ONLY, &off, sizeof(off));
        REQUIRE(Error(errcode) == Error(expected));

        errcode = clarinet_socket_bind(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    }

    SECTION("TWO sockets with SAME port and protocol")
    {
        const int NONE = 0;
        const int REUSEADDR = (1 << 0);
        const int IPV6DUAL = (1 << 1);

        // @formatter:off
        // Table: sample, first socket address | first socket flags | second socket address | second socket flags | expected result
        const std::vector<std::tuple<int, clarinet_addr, int, clarinet_addr, int, int>> data = {
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

        SAMPLES(data);

        int sample;
        clarinet_addr addra;
        int fa;
        clarinet_addr addrb;
        int fb;
        int expected;
        std::tie(sample, addra, fa, addrb, fb, expected) = GENERATE_REF(from_samples(data));
        clarinet_proto proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS }));

        FROM(sample);
        FROM(proto);

        const int reuseaddr[] = { fa & REUSEADDR ? 1 : 0, fb & REUSEADDR ? 1 : 0 };
        const int ipv6only[] = { fa & IPV6DUAL ? 0 : 1, fb & IPV6DUAL ? 0 : 1 };

        clarinet_socket sa;
        clarinet_socket* spa = &sa;
        clarinet_socket_init(spa);

        int errcode = clarinet_socket_open(spa, addra.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexita = finalizer([&spa]
        {
            clarinet_socket_close(spa);
        });

        errcode = clarinet_socket_setopt(spa, CLARINET_SO_REUSEADDR, &reuseaddr[0], sizeof(reuseaddr[0]));
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_setopt(spa, CLARINET_IP_V6ONLY, &ipv6only[0], sizeof(ipv6only[0]));
        REQUIRE(Error(errcode) == Error(spa->family == CLARINET_AF_INET6 ? CLARINET_ENONE : CLARINET_EINVAL));

        clarinet_endpoint endpoint = { addra, 0 };
        errcode = clarinet_socket_bind(spa, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_endpoint local;
        errcode = clarinet_socket_local_endpoint(spa, &local);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_socket sb;
        clarinet_socket* spb = &sb;
        clarinet_socket_init(spb);

        errcode = clarinet_socket_open(spb, addrb.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexitb = finalizer([&spb]
        {
            clarinet_socket_close(spb);
        });

        errcode = clarinet_socket_setopt(spb, CLARINET_SO_REUSEADDR, &reuseaddr[1], sizeof(reuseaddr[2]));
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_setopt(spb, CLARINET_IP_V6ONLY, &ipv6only[1], sizeof(ipv6only[2]));
        REQUIRE(Error(errcode) == Error(spb->family == CLARINET_AF_INET6 ? CLARINET_ENONE : CLARINET_EINVAL));

        endpoint.addr = addrb;
        endpoint.port = local.port;
        errcode = clarinet_socket_bind(spb, &endpoint);
        CHECK(Error(errcode) == Error(expected));
    }
}

TEST_CASE("Listen")
{
    SECTION("INCOMPATIBLE protocols")
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

        SAMPLES(data);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(data));

        /* All protocols compatible with listen can be opened but not all protocols *incompatible* with listen can, so
         * they must be filtered out */
        const std::vector<int> protos = { OPEN_COMPATIBLE_PROTOCOLS };
        const int can_open = std::accumulate(protos.begin(), protos.end(), (int)CLARINET_PROTO_NONE, std::bit_or<>());
        clarinet_proto proto = GENERATE_REF(filter([&can_open](int v)
        {
            return v & can_open;
        }, values({ LISTEN_INCOMPATIBLE_PROTOCOLS })));

        /* Cover the minimum supported backlog values but shouldn't be any different to any other value really */
        int backlog = GENERATE(0, 1, 2, 3, 4, 5);

        FROM(sample);
        FROM(proto);
        FROM(backlog);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        errcode = clarinet_socket_bind(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_listen(sp, backlog);
        REQUIRE(Error(errcode) == Error(CLARINET_EPROTONOSUPPORT));
    }

    SECTION("COMPATIBLE protocols")
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

        SAMPLES(data);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(data));

        /* All protocols compatible with listen should be compatible with open too so no need to filter */
        int proto = GENERATE(values({ LISTEN_COMPATIBLE_PROTOCOLS }));

        /* Cover the minimum supported backlog values */
        int backlog = GENERATE(0, 1, 2, 3, 4, 5);
        int calls = GENERATE(1, 2, 3, 4);

        FROM(sample);
        FROM(Protocol(proto));
        FROM(backlog);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        errcode = clarinet_socket_bind(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        for (int repeats = 1; repeats < calls; ++repeats)
        {
            FROM(repeats);
            errcode = clarinet_socket_listen(sp, backlog);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        }
    }
}

TEST_CASE("Connect")
{

}

TEST_CASE("Accept")
{

}

TEST_CASE("Get local endpoint")
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

    SAMPLES(data);

    int sample;
    clarinet_endpoint endpoint;
    std::tie(sample, endpoint) = GENERATE_REF(from_samples(data));
    int proto = GENERATE(values({ OPEN_COMPATIBLE_PROTOCOLS }));

    FROM(sample);
    FROM(Protocol(proto));

    clarinet_socket socket;
    clarinet_socket* sp = &socket;
    clarinet_socket_init(sp);

    int errcode = clarinet_socket_open(sp, endpoint.addr.family, proto);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    const auto onexit = finalizer([&sp]
    {
        clarinet_socket_close(sp);
    });

    clarinet_endpoint unbound = { { 0 } };
    unbound.addr.family = sp->family;

    clarinet_endpoint local;

    memset(&local, 0, sizeof(local));
    errcode = clarinet_socket_local_endpoint(sp, &local);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

    /* before bind the local endpoint should be empty except for the family */
    int addr_is_equal = clarinet_endpoint_is_equal(&local, &unbound);
    REQUIRE(addr_is_equal);

    errcode = clarinet_socket_bind(sp, &endpoint);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

    memset(&local, 0, sizeof(local));
    errcode = clarinet_socket_local_endpoint(sp, &local);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

    /* after a successful bind the local endpoint address should be defined although the port may still vary */
    addr_is_equal = clarinet_addr_is_equal(&local.addr, &endpoint.addr);
    REQUIRE(addr_is_equal);
    if (endpoint.port == 0)
        REQUIRE(local.port > 0);
    else
        REQUIRE(local.port == endpoint.port);
}

TEST_CASE("Get remote endpoint")
{

}

TEST_CASE("Send")
{

}

TEST_CASE("Send To")
{

}

TEST_CASE("Send To on " CONFIG_SYSTEM_NAME, "[!nonportable]")
{

}

TEST_CASE("Recv")
{

}

TEST_CASE("Recv From")
{

}

TEST_CASE("Shutdown")
{

}

#if 0

{
SECTION("Open one address once")
{
// @formatter:off
        const std::vector<std::tuple<int, cl_endpoint>> data = {
            { 0, { cl_addr_any_ipv4,      0 } },
            { 1, { cl_addr_loopback_ipv4, 0 } },
            #if CL_ENABLE_IPV6
            { 2, { cl_addr_any_ipv6,      0 } },
            { 3, { cl_addr_loopback_ipv6, 0 } },
            #endif
        };
        // @formatter:on

SAMPLES(data);

int sample;
cl_endpoint endpoint;
std::tie(sample, endpoint
) = GENERATE_REF(from_samples(data));

FROM(sample);

cl_udp_socket* socket = nullptr;

SECTION("Open same socket twice")
{
int errcode = cl_udp_open(&socket, &endpoint, &cl_udp_settings_default, 0);
const auto onexit = finalizer([&]
{
    cl_udp_close(&socket);
});

REQUIRE(Error(errcode) == Error(CL_ENONE));
REQUIRE(socket != nullptr);

const cl_udp_socket* original = socket;

errcode = cl_udp_open(&socket, &endpoint, &cl_udp_settings_default, 0);
REQUIRE(Error(errcode) == Error(CL_EINVAL));
REQUIRE(socket == original);
}

SECTION("Close same socket twice")
{
int errcode = cl_udp_open(&socket, &endpoint, &cl_udp_settings_default, 0);
REQUIRE(Error(errcode) == Error(CL_ENONE));

errcode = cl_udp_close(&socket);
REQUIRE(Error(errcode) == Error(CL_ENONE));
REQUIRE(socket == nullptr);

errcode = cl_udp_close(&socket);
REQUIRE(Error(errcode) == Error(CL_EINVAL));
REQUIRE(socket == nullptr);
}

SECTION("Open in normal mode")
{
int errcode = cl_udp_open(&socket, &endpoint, &cl_udp_settings_default, 0);
REQUIRE(Error(errcode) == Error(CL_ENONE));
REQUIRE(socket != nullptr);

errcode = cl_udp_close(&socket);
REQUIRE(Error(errcode) == Error(CL_ENONE));
REQUIRE(socket == nullptr);
}

SECTION("Open in dual stack mode")
{
int errcode = cl_udp_open(&socket, &endpoint, &cl_udp_settings_default,
    cl_flag(CL_SO_IPV6DUAL));
if (endpoint.addr.family == CL_AF_INET)
{
REQUIRE(Error(errcode) == Error(CL_EINVAL));
REQUIRE(socket == nullptr);
}
else if (endpoint.addr.family == CL_AF_INET6)
{
#if CL_ENABLE_IPV6DUAL
REQUIRE(Error(errcode) == Error(CL_ENONE));
REQUIRE(socket != nullptr);

errcode = cl_udp_close(&socket);
REQUIRE(Error(errcode) == Error(CL_ENONE));
REQUIRE(socket == nullptr);
#else
REQUIRE(Error(errcode) == Error(CL_EINVAL));
REQUIRE(socket == nullptr);
#endif
}
else
{
REQUIRE(Error(errcode) == Error(CL_EINVAL));
REQUIRE(socket == nullptr);
}
}
}

SECTION("Open one address twice with different ports")
{
// @formatter:off
        const std::vector<std::tuple<int, cl_endpoint>> data = {
            { 0, { cl_addr_any_ipv4,      0 } },
            { 1, { cl_addr_loopback_ipv4, 0 } },
            #if CL_ENABLE_IPV6
            { 2, { cl_addr_any_ipv6,      0 } },
            { 3, { cl_addr_loopback_ipv6, 0 } },
            #endif
        };
        // @formatter:on

SAMPLES(data);

int sample;
cl_endpoint endpoint;
std::tie(sample, endpoint
) = GENERATE_REF(from_samples(data));

FROM(sample);

cl_udp_socket* sa = nullptr;
int errcode = cl_udp_open(&sa, &endpoint, &cl_udp_settings_default, 0);
REQUIRE(Error(errcode) == Error(CL_ENONE));
REQUIRE(sa != nullptr);
const auto onexita = finalizer([&]
{
    cl_udp_close(&sa);
});

cl_udp_socket* sb = nullptr;
errcode = cl_udp_open(&sb, &endpoint, &cl_udp_settings_default, 0);
REQUIRE(Error(errcode) == Error(CL_ENONE));
REQUIRE(sb != nullptr);
const auto onexitb = finalizer([&]
{
    cl_udp_close(&sb);
});

cl_endpoint sa_endp;
errcode = cl_udp_get_endpoint(sa, &sa_endp);
REQUIRE(Error(errcode) == Error(CL_ENONE));

cl_endpoint sb_endp;
errcode = cl_udp_get_endpoint(sb, &sb_endp);
REQUIRE(Error(errcode) == Error(CL_ENONE));

REQUIRE(sa_endp.port != sb_endp.port);
}

SECTION("Open two addresses with the same port")
{
const auto NONE = 0U;
const auto REUSEADDR = cl_flag(CL_SO_REUSEADDR);
const auto IPV6DUAL = cl_flag(CL_SO_IPV6DUAL);

        // @formatter:off
        // Table: sample, first socket address | first socket flags | second socket address | second socket flags | expected result
        const std::vector<std::tuple<int, cl_addr, uint32_t, cl_addr, uint32_t, int>> data = {
            {  0, cl_addr_any_ipv4,       NONE,               cl_addr_any_ipv4,         NONE,               CL_EADDRINUSE },
            {  1, cl_addr_any_ipv4,       NONE,               cl_addr_loopback_ipv4,    NONE,               CL_EADDRINUSE },
            {  2, cl_addr_loopback_ipv4,  NONE,               cl_addr_any_ipv4,         NONE,               CL_EADDRINUSE },
            {  3, cl_addr_loopback_ipv4,  NONE,               cl_addr_loopback_ipv4,    NONE,               CL_EADDRINUSE },
            {  4, cl_addr_any_ipv4,       NONE,               cl_addr_any_ipv4,         REUSEADDR,          CL_EADDRINUSE },
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            {  5, cl_addr_any_ipv4,       NONE,               cl_addr_loopback_ipv4,    REUSEADDR,          CL_ENONE      }, // BSD/Darwin
            #else
            {  5, cl_addr_any_ipv4,       NONE,               cl_addr_loopback_ipv4,    REUSEADDR,          CL_EADDRINUSE }, // Others
            #endif
            #if defined(__linux__)
            {  6, cl_addr_loopback_ipv4,  NONE,               cl_addr_any_ipv4,         REUSEADDR,          CL_EADDRINUSE }, // Linux
            #else
            {  6, cl_addr_loopback_ipv4,  NONE,               cl_addr_any_ipv4,         REUSEADDR,          CL_ENONE      }, // Others
            #endif
            {  7, cl_addr_loopback_ipv4,  NONE,               cl_addr_loopback_ipv4,    REUSEADDR,          CL_EADDRINUSE },
            {  8, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_any_ipv4,         NONE,               CL_EADDRINUSE },
            #if defined(_WIN32)
            {  9, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_loopback_ipv4,    NONE,               CL_ENONE      }, // Windows
            #else
            {  9, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_loopback_ipv4,    NONE,               CL_EADDRINUSE }, // Others
            #endif
            { 10, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_any_ipv4,         NONE,               CL_EADDRINUSE },
            { 11, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_loopback_ipv4,    NONE,               CL_EADDRINUSE },
            { 12, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_any_ipv4,         REUSEADDR,          CL_ENONE      },
            { 13, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_loopback_ipv4,    REUSEADDR,          CL_ENONE      },
            { 14, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_any_ipv4,         REUSEADDR,          CL_ENONE      },
            { 15, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_loopback_ipv4,    REUSEADDR,          CL_ENONE      },

            #if CL_ENABLE_IPV6
            { 16, cl_addr_any_ipv6,       NONE,               cl_addr_any_ipv6,         NONE,               CL_EADDRINUSE },
            { 17, cl_addr_any_ipv6,       NONE,               cl_addr_loopback_ipv6,    NONE,               CL_EADDRINUSE },
            { 18, cl_addr_loopback_ipv6,  NONE,               cl_addr_any_ipv6,         NONE,               CL_EADDRINUSE },
            { 19, cl_addr_loopback_ipv6,  NONE,               cl_addr_loopback_ipv6,    NONE,               CL_EADDRINUSE },
            { 20, cl_addr_any_ipv6,       NONE,               cl_addr_any_ipv6,         REUSEADDR,          CL_EADDRINUSE },
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            { 21, cl_addr_any_ipv6,       NONE,               cl_addr_loopback_ipv6,    REUSEADDR,          CL_ENONE      }, // BSD/Darwin
            #else
            { 21, cl_addr_any_ipv6,       NONE,               cl_addr_loopback_ipv6,    REUSEADDR,          CL_EADDRINUSE }, // Others
            #endif
            #if defined(__linux__)
            { 22, cl_addr_loopback_ipv6,  NONE,               cl_addr_any_ipv6,         REUSEADDR,          CL_EADDRINUSE }, // Linux
            #else
            { 22, cl_addr_loopback_ipv6,  NONE,               cl_addr_any_ipv6,         REUSEADDR,          CL_ENONE      }, // Others
            #endif
            { 23, cl_addr_loopback_ipv6,  NONE,               cl_addr_loopback_ipv6,    REUSEADDR,          CL_EADDRINUSE },
            { 24, cl_addr_any_ipv6,       REUSEADDR,          cl_addr_any_ipv6,         NONE,               CL_EADDRINUSE },
            #if defined(_WIN32)
            { 25, cl_addr_any_ipv6,       REUSEADDR,          cl_addr_loopback_ipv6,    NONE,               CL_ENONE      }, // Windows
            #else
            { 25, cl_addr_any_ipv6,       REUSEADDR,          cl_addr_loopback_ipv6,    NONE,               CL_EADDRINUSE }, // Others
            #endif
            { 26, cl_addr_loopback_ipv6,  REUSEADDR,          cl_addr_any_ipv6,         NONE,               CL_EADDRINUSE },
            { 27, cl_addr_loopback_ipv6,  REUSEADDR,          cl_addr_loopback_ipv6,    NONE,               CL_EADDRINUSE },
            { 28, cl_addr_any_ipv6,       REUSEADDR,          cl_addr_any_ipv6,         REUSEADDR,          CL_ENONE      },
            { 29, cl_addr_any_ipv6,       REUSEADDR,          cl_addr_loopback_ipv6,    REUSEADDR,          CL_ENONE      },
            { 30, cl_addr_loopback_ipv6,  REUSEADDR,          cl_addr_any_ipv6,         REUSEADDR,          CL_ENONE      },
            { 31, cl_addr_loopback_ipv6,  REUSEADDR,          cl_addr_loopback_ipv6,    REUSEADDR,          CL_ENONE      },

            { 32, cl_addr_any_ipv6,       NONE,               cl_addr_any_ipv4,         NONE,               CL_ENONE      },
            { 33, cl_addr_any_ipv6,       NONE,               cl_addr_loopback_ipv4,    NONE,               CL_ENONE      },
            { 34, cl_addr_loopback_ipv6,  NONE,               cl_addr_any_ipv4,         NONE,               CL_ENONE      },
            { 35, cl_addr_loopback_ipv6,  NONE,               cl_addr_loopback_ipv4,    NONE,               CL_ENONE      },
            { 36, cl_addr_any_ipv6,       NONE,               cl_addr_any_ipv4,         REUSEADDR,          CL_ENONE      },
            { 37, cl_addr_any_ipv6,       NONE,               cl_addr_loopback_ipv4,    REUSEADDR,          CL_ENONE      },
            { 38, cl_addr_loopback_ipv6,  NONE,               cl_addr_any_ipv4,         REUSEADDR,          CL_ENONE      },
            { 39, cl_addr_loopback_ipv6,  NONE,               cl_addr_loopback_ipv4,    REUSEADDR,          CL_ENONE      },
            { 40, cl_addr_any_ipv6,       REUSEADDR,          cl_addr_any_ipv4,         NONE,               CL_ENONE      },
            { 41, cl_addr_any_ipv6,       REUSEADDR,          cl_addr_loopback_ipv4,    NONE,               CL_ENONE      },
            { 42, cl_addr_loopback_ipv6,  REUSEADDR,          cl_addr_any_ipv4,         NONE,               CL_ENONE      },
            { 43, cl_addr_loopback_ipv6,  REUSEADDR,          cl_addr_loopback_ipv4,    NONE,               CL_ENONE      },
            { 44, cl_addr_any_ipv6,       REUSEADDR,          cl_addr_any_ipv4,         REUSEADDR,          CL_ENONE      },
            { 45, cl_addr_any_ipv6,       REUSEADDR,          cl_addr_loopback_ipv4,    REUSEADDR,          CL_ENONE      },
            { 46, cl_addr_loopback_ipv6,  REUSEADDR,          cl_addr_any_ipv4,         REUSEADDR,          CL_ENONE      },
            { 47, cl_addr_loopback_ipv6,  REUSEADDR,          cl_addr_loopback_ipv4,    REUSEADDR,          CL_ENONE      },

            { 48, cl_addr_any_ipv4,       NONE,               cl_addr_any_ipv6,         NONE,               CL_ENONE      },
            { 49, cl_addr_any_ipv4,       NONE,               cl_addr_loopback_ipv6,    NONE,               CL_ENONE      },
            { 40, cl_addr_loopback_ipv4,  NONE,               cl_addr_any_ipv6,         NONE,               CL_ENONE      },
            { 51, cl_addr_loopback_ipv4,  NONE,               cl_addr_loopback_ipv6,    NONE,               CL_ENONE      },
            { 52, cl_addr_any_ipv4,       NONE,               cl_addr_any_ipv6,         REUSEADDR,          CL_ENONE      },
            { 53, cl_addr_any_ipv4,       NONE,               cl_addr_loopback_ipv6,    REUSEADDR,          CL_ENONE      },
            { 54, cl_addr_loopback_ipv4,  NONE,               cl_addr_any_ipv6,         REUSEADDR,          CL_ENONE      },
            { 55, cl_addr_loopback_ipv4,  NONE,               cl_addr_loopback_ipv6,    REUSEADDR,          CL_ENONE      },
            { 56, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_any_ipv6,         NONE,               CL_ENONE      },
            { 57, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_loopback_ipv6,    NONE,               CL_ENONE      },
            { 58, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_any_ipv6,         NONE,               CL_ENONE      },
            { 59, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_loopback_ipv6,    NONE,               CL_ENONE      },
            { 60, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_any_ipv6,         REUSEADDR,          CL_ENONE      },
            { 61, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_loopback_ipv6,    REUSEADDR,          CL_ENONE      },
            { 62, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_any_ipv6,         REUSEADDR,          CL_ENONE      },
            { 63, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_loopback_ipv6,    REUSEADDR,          CL_ENONE      },

            #if CL_ENABLE_IPV6DUAL
            { 64, cl_addr_any_ipv6,       IPV6DUAL,           cl_addr_any_ipv4,         NONE,               CL_EADDRINUSE },
            { 65, cl_addr_any_ipv6,       IPV6DUAL,           cl_addr_loopback_ipv4,    NONE,               CL_EADDRINUSE },
            { 66, cl_addr_loopback_ipv6,  IPV6DUAL,           cl_addr_any_ipv4,         NONE,               CL_ENONE      },
            { 67, cl_addr_loopback_ipv6,  IPV6DUAL,           cl_addr_loopback_ipv4,    NONE,               CL_ENONE      },
            { 68, cl_addr_any_ipv6,       IPV6DUAL,           cl_addr_any_ipv4,         REUSEADDR,          CL_EADDRINUSE },
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            { 69, cl_addr_any_ipv6,       IPV6DUAL,           cl_addr_loopback_ipv4,    REUSEADDR,          CL_ENONE      }, // BSD/Darwin
            #else
            { 69, cl_addr_any_ipv6,       IPV6DUAL,           cl_addr_loopback_ipv4,    REUSEADDR,          CL_EADDRINUSE }, // Others
            #endif
            { 70, cl_addr_loopback_ipv6,  IPV6DUAL,           cl_addr_any_ipv4,         REUSEADDR,          CL_ENONE      },
            { 71, cl_addr_loopback_ipv6,  IPV6DUAL,           cl_addr_loopback_ipv4,    REUSEADDR,          CL_ENONE      },
            #if defined(_WIN32)
            { 72, cl_addr_any_ipv6,       IPV6DUAL|REUSEADDR, cl_addr_any_ipv4,         NONE,               CL_ENONE      }, // Windows
            { 73, cl_addr_any_ipv6,       IPV6DUAL|REUSEADDR, cl_addr_loopback_ipv4,    NONE,               CL_ENONE      }, // Windows
            #else
            { 72, cl_addr_any_ipv6,       IPV6DUAL|REUSEADDR, cl_addr_any_ipv4,         NONE,               CL_EADDRINUSE }, // Others
            { 73, cl_addr_any_ipv6,       IPV6DUAL|REUSEADDR, cl_addr_loopback_ipv4,    NONE,               CL_EADDRINUSE }, // Others
            #endif
            { 74, cl_addr_loopback_ipv6,  IPV6DUAL|REUSEADDR, cl_addr_any_ipv4,         NONE,               CL_ENONE      },
            { 75, cl_addr_loopback_ipv6,  IPV6DUAL|REUSEADDR, cl_addr_loopback_ipv4,    NONE,               CL_ENONE      },
            { 76, cl_addr_any_ipv6,       IPV6DUAL|REUSEADDR, cl_addr_any_ipv4,         REUSEADDR,          CL_ENONE      },
            { 77, cl_addr_any_ipv6,       IPV6DUAL|REUSEADDR, cl_addr_loopback_ipv4,    REUSEADDR,          CL_ENONE      },
            { 78, cl_addr_loopback_ipv6,  IPV6DUAL|REUSEADDR, cl_addr_any_ipv4,         REUSEADDR,          CL_ENONE      },
            { 79, cl_addr_loopback_ipv6,  IPV6DUAL|REUSEADDR, cl_addr_loopback_ipv4,    REUSEADDR,          CL_ENONE      },

            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            { 80, cl_addr_any_ipv4,       NONE,               cl_addr_any_ipv6,         IPV6DUAL,           CL_ENONE      }, // BSD/Darwin
            #else
            { 80, cl_addr_any_ipv4,       NONE,               cl_addr_any_ipv6,         IPV6DUAL,           CL_EADDRINUSE }, // Others
            #endif
            { 81, cl_addr_any_ipv4,       NONE,               cl_addr_loopback_ipv6,    IPV6DUAL,           CL_ENONE      },
            { 82, cl_addr_loopback_ipv4,  NONE,               cl_addr_any_ipv6,         IPV6DUAL,           CL_EADDRINUSE },
            { 83, cl_addr_loopback_ipv4,  NONE,               cl_addr_loopback_ipv6,    IPV6DUAL,           CL_ENONE      },
            #if defined(__linux__)
            { 84, cl_addr_any_ipv4,       NONE,               cl_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CL_EADDRINUSE }, // Linux
            #else
            { 84, cl_addr_any_ipv4,       NONE,               cl_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CL_ENONE      }, // Others
            #endif
            { 85, cl_addr_any_ipv4,       NONE,               cl_addr_loopback_ipv6,    IPV6DUAL|REUSEADDR, CL_ENONE      },
            #if defined(__linux__)
            { 86, cl_addr_loopback_ipv4,  NONE,               cl_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CL_EADDRINUSE }, // Linux
            #else
            { 86, cl_addr_loopback_ipv4,  NONE,               cl_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CL_ENONE      }, // Others
            #endif
            { 87, cl_addr_loopback_ipv4,  NONE,               cl_addr_loopback_ipv6,    IPV6DUAL|REUSEADDR, CL_ENONE      },
            #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
            { 88, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_any_ipv6,         IPV6DUAL,           CL_ENONE      }, // BSD/Darwin
            #else
            { 88, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_any_ipv6,         IPV6DUAL,           CL_EADDRINUSE }, // Others
            #endif
            { 89, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_loopback_ipv6,    IPV6DUAL,           CL_ENONE      },
            { 90, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_any_ipv6,         IPV6DUAL,           CL_EADDRINUSE },
            { 91, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_loopback_ipv6,    IPV6DUAL,           CL_ENONE      },
            { 92, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CL_ENONE      },
            { 93, cl_addr_any_ipv4,       REUSEADDR,          cl_addr_loopback_ipv6,    IPV6DUAL|REUSEADDR, CL_ENONE      },
            { 94, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_any_ipv6,         IPV6DUAL|REUSEADDR, CL_ENONE      },
            { 95, cl_addr_loopback_ipv4,  REUSEADDR,          cl_addr_loopback_ipv6,    IPV6DUAL|REUSEADDR, CL_ENONE      },
            #endif // CL_ENABLE_IPV6DUAL
            #endif // CL_ENABLE_IPV6
        };
        // @formatter:on

SAMPLES(data);

int sample;
cl_addr addra;
uint32_t fa;
cl_addr addrb;
uint32_t fb;
int expected;
std::tie(sample, addra, fa, addrb, fb, expected
) = GENERATE_REF(from_samples(data));

FROM(sample);

cl_udp_socket* sa = nullptr;
cl_endpoint endpa = { addra, 0 };
int errcode = cl_udp_open(&sa, &endpa, &cl_udp_settings_default, fa);
const auto onexita = finalizer([&]
{
    cl_udp_close(&sa);
});
REQUIRE(Error(errcode) == Error(CL_ENONE));
REQUIRE(sa != nullptr);

cl_endpoint saendp;
errcode = cl_udp_get_endpoint(sa, &saendp);
REQUIRE(Error(errcode) == Error(CL_ENONE));

cl_endpoint endpb = { addrb, saendp.port };
cl_udp_socket* sb = nullptr;
errcode = cl_udp_open(&sb, &endpb, &cl_udp_settings_default, fb);
const auto onexitb = finalizer([&]
{
    cl_udp_close(&sb);
});
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
    // @formatter:off
    const std::vector<std::tuple<int, cl_endpoint>> data = {
        { 0, { cl_addr_loopback_ipv4, 0 } },
        #if CL_ENABLE_IPV6
        { 1, { cl_addr_loopback_ipv6, 0 } },
        #endif
    };
    // @formatter:on

    SAMPLES(data);

    int sample;
    cl_endpoint local;
    std::tie(sample, local) = GENERATE_REF(from_samples(data));

    FROM(sample);

    cl_udp_socket* socket = nullptr;
    int errcode = cl_udp_open(&socket, &local, &cl_udp_settings_default, 0);
    REQUIRE(Error(errcode) == Error(CL_ENONE));
    REQUIRE(socket != nullptr);
    const auto onexit = finalizer([&]
    {
        cl_udp_close(&socket);
    });

    cl_endpoint actual = { { 0 } };
    errcode = cl_udp_get_endpoint(socket, &actual);
    REQUIRE(Error(errcode) == Error(CL_ENONE));

    cl_endpoint expected = { { 0 } };
    expected.addr = local.addr;
    expected.port = local.port > 0 ? local.port : actual.port;
    REQUIRE(cl_endpoint_is_equal(&actual, &expected));
}


// Windows, Linux and BSD/Darwin all have preemptive kernels with immediate dispatch to the NIC so tests may be able
// to effectively transmit a message larger than the send-buffer or send more consecutive messages than the buffer
// was expected to support. Therefore, send and recv tests are split into different test cases. Basic test cases cover
// common functionality and may define oversized buffers to ensure minimum requirements in all platforms. Specific
// peculiarities of each platform are tested separately an allowed to fail so assumptions can be continusouly validated
// without compromising the functional aspect of the testing process.

static
void
TEST_SEND(int sample,
          const cl_endpoint* source,
          const cl_endpoint* destination,
          int send_buffer_size,
          int send_length,
          int count,
          int expected)
{
    FROM(sample);

    // sanity: avoid obvious invalid test params
    REQUIRE(send_length >= 0);
    REQUIRE(send_length <= INT_MAX);
    REQUIRE(count > 0);
    REQUIRE(count <= INT_MAX);
    REQUIRE(expected <= (int)send_length);

    cl_udp_settings settings = cl_udp_settings_default;
    settings.send_buffer_size = (int32_t)send_buffer_size;
    settings.ttl = 4;

    cl_udp_socket* sender = nullptr;
    const int errcode = cl_udp_open(&sender, source, &settings, 0);
    const auto sender_dtor = finalizer([&]
    {
        cl_udp_close(&sender);
    });
    REQUIRE(Error(errcode) == Error(CL_ENONE));
    REQUIRE(sender != nullptr);

    void* send_buf = malloc((size_t)send_length);
    REQUIRE(send_buf != nullptr);
    const auto send_buf_dtor = finalizer([&]
    {
        free(send_buf);
    });

    for (int packet = 0; packet < count; ++packet)
    {
        FROM(packet);

        const int n = cl_udp_send(sender, send_buf, (size_t)send_length, destination);
        CHECKED_IF(n > 0)
        {
            REQUIRE(Error(n) == Error(send_length));
        }
        CHECKED_ELSE(n > 0)
        {
            REQUIRE(Error(n) == Error(expected));
        }
    }
}

TEST_CASE("Send", "[udp]")
{
    const cl_endpoint any = { cl_addr_any_ipv4, 0 };
    const cl_endpoint remote = cl_make_endpoint(cl_make_ipv4(8, 8, 8, 8), 9);

    #if defined(CL_ENABLE_IPV6)
    // const cl_endpoint remote6 = cl_make_endpoint(cl_make_ipv6(0, 0x2001, 0x4860, 0x4860, 0, 0, 0, 0, 0x8888, 0), 443);
    #endif

    // This is the common ground test. All the samples should work in all platforms. We only send up to 8192 bytes
    // because Darwin (macOS) by default limits datagrams to 9126 bytes.

    // @formatter:off
    // Table: sample | source | destination | send buffer size | send length | count | expected
    const std::vector<std::tuple<int, cl_endpoint, cl_endpoint, int, int, int, int>> data = {
        {  0,  any,    remote,     -1,     0, 1,    CL_ENONE },
        {  1,  any,    remote,     -1,     1, 1,    CL_ENONE },
        {  2,  any,    remote,     -1,     2, 1,    CL_ENONE },
        {  3,  any,    remote,     -1,     4, 1,    CL_ENONE },
        {  4,  any,    remote,     -1,     8, 1,    CL_ENONE },
        {  5,  any,    remote,     -1,    16, 1,    CL_ENONE },
        {  6,  any,    remote,     -1,    32, 1,    CL_ENONE },
        {  7,  any,    remote,     -1,    64, 1,    CL_ENONE },
        {  8,  any,    remote,     -1,   128, 1,    CL_ENONE },
        {  9,  any,    remote,     -1,   256, 1,    CL_ENONE },
        { 10,  any,    remote,     -1,   512, 1,    CL_ENONE },
        { 11,  any,    remote,     -1,  1024, 1,    CL_ENONE },
        { 12,  any,    remote,     -1,  2048, 1,    CL_ENONE },
        { 13,  any,    remote,     -1,  4096, 1,    CL_ENONE },
        { 14,  any,    remote,     -1,  8192, 1,    CL_ENONE },

        { 15,  any,    remote,    768,     0, 1,    CL_ENONE },
        { 16,  any,    remote,    768,     1, 1,    CL_ENONE },
        { 17,  any,    remote,    768,     2, 1,    CL_ENONE },
        { 18,  any,    remote,    768,     4, 1,    CL_ENONE },
        { 19,  any,    remote,    768,     8, 1,    CL_ENONE },
        { 20,  any,    remote,    768,    16, 1,    CL_ENONE },
        { 21,  any,    remote,    768,    32, 1,    CL_ENONE },
        { 22,  any,    remote,    768,    64, 1,    CL_ENONE },
        { 23,  any,    remote,   1280,   128, 1,    CL_ENONE },
        { 24,  any,    remote,   1280,   256, 1,    CL_ENONE },
        { 25,  any,    remote,   1280,   512, 1,    CL_ENONE },
        { 26,  any,    remote,   2304,  1024, 1,    CL_ENONE },
        { 27,  any,    remote,   4352,  2048, 1,    CL_ENONE },
        { 28,  any,    remote,   8448,  4096, 1,    CL_ENONE },
        { 29,  any,    remote,  16640,  8192, 1,    CL_ENONE },
    };
    // @formatter:on

    SAMPLES(data);

    int sample;
    cl_endpoint source;
    cl_endpoint destination;
    int send_buffer_size;
    int send_length;
    int count;
    int expected;

    std::tie(sample, source, destination,
        send_buffer_size, send_length,
        count,
        expected) = GENERATE_REF(from_samples(data));

    REQUIRE(send_length <= 8192);
    TEST_SEND(sample, &source, &destination, send_buffer_size, send_length, count, expected);
}

// Send may have specific characteristics in different platforms such as distinct buffer size overheads so this test
// case serves to validate assumptions with a dataset for each platform.
TEST_CASE("Send on " CONFIG_SYSTEM_NAME, "[udp][!nonportable]")
{
    // We try to avoid platform specific code in tests but in this case certain system settings are critical for the
    // outcome of the test case and will most certainly get testers by surprise. On Linux some test samples require
    // the maximum buffer memory to be increased and on BSD/Darwin some samples require the maximum datagram size to be
    // increased.
    #if defined(__linux__)
    {
        std::fstream fwmem_max("/proc/sys/net/core/wmem_max", std::ios_base::in);
        int net_core_wmem_max = 0;
        fwmem_max >> net_core_wmem_max;
        REQUIRE(net_core_wmem_max >= 6398722);
        fwmem_max.close();
    }
    #elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
    {
        int maxdgram = 0;
        size_t len = sizeof(maxdgram);
        int err = sysctlbyname("net.inet.udp.maxdgram", &maxdgram, &len, nullptr, 0);
        REQUIRE(err == 0);
        REQUIRE(maxdgram >= 65535);
    }
    #endif

    const cl_endpoint any = { cl_addr_any_ipv4, 0 };
    const cl_endpoint remote = cl_make_endpoint(cl_make_ipv4(8, 8, 8, 8), 9);

    // @formatter:off
    // Table: sample | source | destination | send buffer size | send length | count | expected
    const std::vector<std::tuple<int, cl_endpoint, cl_endpoint, int, int, int, int>> data =
    {
        // Passing a send-buffer size of 0 should fall back to using the system default in each platform.

        #if defined(_WIN32)

        // Setting buffer size to -1 has the effect of using the system's default value which on Windows is 8192.
        // CHECK: Is there a registry key to change this default?

        {   0,  any,    remote,     -1,               0,    1,       CL_ENONE },
        {   1,  any,    remote,     -1,               1,    1,       CL_ENONE },
        {   2,  any,    remote,     -1,               2,    1,       CL_ENONE },
        {   3,  any,    remote,     -1,               4,    1,       CL_ENONE },
        {   4,  any,    remote,     -1,               8,    1,       CL_ENONE },
        {   5,  any,    remote,     -1,              16,    1,       CL_ENONE },
        {   6,  any,    remote,     -1,              32,    1,       CL_ENONE },
        {   7,  any,    remote,     -1,              64,    1,       CL_ENONE },
        {   8,  any,    remote,     -1,             128,    1,       CL_ENONE },
        {   9,  any,    remote,     -1,             256,    1,       CL_ENONE },
        {  10,  any,    remote,     -1,             512,    1,       CL_ENONE },
        {  11,  any,    remote,     -1,            1024,    1,       CL_ENONE },
        {  12,  any,    remote,     -1,            2048,    1,       CL_ENONE },
        {  13,  any,    remote,     -1,            4096,    1,       CL_ENONE },
        {  14,  any,    remote,     -1,            8192,    1,       CL_ENONE },
        {  15,  any,    remote,     -1,           16384,    1,       CL_ENONE },
        {  16,  any,    remote,     -1,           32768,    1,       CL_ENONE },
        {  17,  any,    remote,     -1,           65507,    1,       CL_ENONE },
        {  18,  any,    remote,     -1,           65508,    1,    CL_EMSGSIZE },
        {  19,  any,    remote,     -1,           65535,    1,    CL_EMSGSIZE },

        // Windows does not impose a minimum send buffer size so 256 is actually 256 but the first message always
        // bypasses the buffer regardless of the buffer size. It is not clear if this is due to a separate internal
        // buffer or if this is because the message is dispatched directly to the NIC tx queue.
        // CHECK: Is this always really the case or does it depend on a specific feature of the NIC ?
        // Windows mentions SO_MAX_MSG_SIZE should be used to determine the maximum message size supported by the
        // protocol but this is known to be 65507 for udp/ipv4 and 65527 for udp/ipv6.

        {  20,  any,    remote,    256,               0,    1,       CL_ENONE },
        {  21,  any,    remote,    256,               1,    1,       CL_ENONE },
        {  22,  any,    remote,    256,               2,    1,       CL_ENONE },
        {  23,  any,    remote,    256,               4,    1,       CL_ENONE },
        {  24,  any,    remote,    256,               8,    1,       CL_ENONE },
        {  25,  any,    remote,    256,              16,    1,       CL_ENONE },
        {  26,  any,    remote,    256,              32,    1,       CL_ENONE },
        {  27,  any,    remote,    256,              64,    1,       CL_ENONE },
        {  28,  any,    remote,    256,             128,    1,       CL_ENONE },
        {  29,  any,    remote,    256,             256,    1,       CL_ENONE },
        {  30,  any,    remote,    256,             512,    1,       CL_ENONE },
        {  31,  any,    remote,    256,            1024,    1,       CL_ENONE },
        {  32,  any,    remote,    256,            2048,    1,       CL_ENONE },
        {  33,  any,    remote,    256,            4096,    1,       CL_ENONE },
        {  34,  any,    remote,    256,            8192,    1,       CL_ENONE },
        {  35,  any,    remote,    256,           16384,    1,       CL_ENONE },
        {  36,  any,    remote,    256,           32768,    1,       CL_ENONE },
        {  37,  any,    remote,    256,           65507,    1,       CL_ENONE },
        {  38,  any,    remote,    256,           65508,    1,    CL_EMSGSIZE },
        {  39,  any,    remote,    256,           65535,    1,    CL_EMSGSIZE },
        {  40,  any,    remote,   4096,               0,    1,       CL_ENONE },
        {  41,  any,    remote,   4096,               1,    1,       CL_ENONE },
        {  42,  any,    remote,   4096,               2,    1,       CL_ENONE },
        {  43,  any,    remote,   4096,               4,    1,       CL_ENONE },
        {  44,  any,    remote,   4096,               8,    1,       CL_ENONE },
        {  45,  any,    remote,   4096,              16,    1,       CL_ENONE },
        {  46,  any,    remote,   4096,              32,    1,       CL_ENONE },
        {  47,  any,    remote,   4096,              64,    1,       CL_ENONE },
        {  48,  any,    remote,   4096,             128,    1,       CL_ENONE },
        {  49,  any,    remote,   4096,             256,    1,       CL_ENONE },
        {  50,  any,    remote,   4096,             512,    1,       CL_ENONE },
        {  51,  any,    remote,   4096,            1024,    1,       CL_ENONE },
        {  52,  any,    remote,   4096,            2048,    1,       CL_ENONE },
        {  53,  any,    remote,   4096,            4096,    1,       CL_ENONE },
        {  54,  any,    remote,   4096,            8192,    1,       CL_ENONE },
        {  55,  any,    remote,   4096,           16384,    1,       CL_ENONE },
        {  56,  any,    remote,   4096,           32768,    1,       CL_ENONE },
        {  57,  any,    remote,   4096,           65507,    1,       CL_ENONE },
        {  58,  any,    remote,   4096,           65508,    1,    CL_EMSGSIZE },
        {  59,  any,    remote,   4096,           65535,    1,    CL_EMSGSIZE },
        {  60,  any,    remote,   4096,               0,    2,       CL_ENONE },
        {  61,  any,    remote,   4096,               1,    2,       CL_ENONE },
        {  62,  any,    remote,   4096,               2,    2,       CL_ENONE },
        {  63,  any,    remote,   4096,               4,    2,       CL_ENONE },
        {  64,  any,    remote,   4096,               8,    2,       CL_ENONE },
        {  65,  any,    remote,   4096,              16,    2,       CL_ENONE },
        {  66,  any,    remote,   4096,              32,    2,       CL_ENONE },
        {  67,  any,    remote,   4096,              64,    2,       CL_ENONE },
        {  68,  any,    remote,   4096,             128,    2,       CL_ENONE },
        {  69,  any,    remote,   4096,             256,    2,       CL_ENONE },
        {  70,  any,    remote,   4096,             512,    2,       CL_ENONE },
        {  71,  any,    remote,   4096,            1024,    2,       CL_ENONE },
        {  72,  any,    remote,   4096,            2048,    2,       CL_ENONE },

        // Messages do not require extra space in the buffer for protocol/system overhead so the MTU is irrelevant
        // (whether fragmented or not the message is going to take the exact same space). However, the last byte of
        // the send-buffer is reserved so a message can only be buffered if size < available space (not <=). In the
        // following samples the first message is expected to bypass the buffer but the remaining ones are stored.

        {  73,  any,    remote,   4096,            4096,    2,      CL_EAGAIN },
        {  74,  any,    remote,   4096 * 1 + 1,    4096,    2,       CL_ENONE },
        {  75,  any,    remote,   4096,            4096,    3,      CL_EAGAIN },
        {  76,  any,    remote,   4096 * 3,        4096,    3,       CL_ENONE },
        {  77,  any,    remote,   4096 * 2,        4096,    3,      CL_EAGAIN },
        {  78,  any,    remote,   4096 * 2 + 1,    4096,    3,       CL_ENONE },
        {  79,  any,    remote,   4096 * 9 + 1,    4096,   10,       CL_ENONE },
        {  80,  any,    remote,   4096,            2048,    3,      CL_EAGAIN },
        {  81,  any,    remote,   4096 + 1,        2048,    3,       CL_ENONE },
        {  82,  any,    remote,     16 * 199,        16,  200,      CL_EAGAIN },
        {  83,  any,    remote,     16 * 199 + 1,    16,  200,       CL_ENONE },
        {  84,  any,    remote,     48 * 4999,       48, 5000,      CL_EAGAIN },
        {  85,  any,    remote,     48 * 4999 + 1,   48, 5000,       CL_ENONE },
        {  86,  any,    remote,     49 * 4999,       49, 5000,      CL_EAGAIN },
        {  87,  any,    remote,     49 * 4999 + 1,   49, 5000,       CL_ENONE },

        // Setting the buffer size to 0 effectively disables buffering on Windows so every second packet should fail
        // except when the message size is zero in which case it is always successful.

        {  88,  any,    remote,      0,               0,   99,       CL_ENONE },
        {  89,  any,    remote,      0,               1,    2,      CL_EAGAIN },
        {  90,  any,    remote,      0,               2,    2,      CL_EAGAIN },
        {  91,  any,    remote,      0,               4,    2,      CL_EAGAIN },
        {  92,  any,    remote,      0,               8,    2,      CL_EAGAIN },
        {  93,  any,    remote,      0,              16,    2,      CL_EAGAIN },
        {  94,  any,    remote,      0,              32,    2,      CL_EAGAIN },
        {  95,  any,    remote,      0,              64,    2,      CL_EAGAIN },
        {  96,  any,    remote,      0,             128,    2,      CL_EAGAIN },
        {  97,  any,    remote,      0,             256,    2,      CL_EAGAIN },
        {  98,  any,    remote,      0,             512,    2,      CL_EAGAIN },
        {  99,  any,    remote,      0,            1024,    2,      CL_EAGAIN },
        { 100,  any,    remote,      0,            2048,    2,      CL_EAGAIN },
        { 101,  any,    remote,      0,            4096,    2,      CL_EAGAIN },
        { 102,  any,    remote,      0,            8192,    2,      CL_EAGAIN },
        { 103,  any,    remote,      0,           16384,    2,      CL_EAGAIN },
        { 104,  any,    remote,      0,           32768,    2,      CL_EAGAIN },
        { 105,  any,    remote,      0,           65507,    2,      CL_EAGAIN },
        { 106,  any,    remote,      0,           65508,    2,    CL_EMSGSIZE },
        { 107,  any,    remote,      0,           65535,    2,    CL_EMSGSIZE },

        #elif defined(__linux__)

        // By default, the send-buffer size on Linux x64 is 212992 but the kernel may be able to dispatch packets to the
        // NIC between calls to send(2) giving the impression that more packets can be sent in a burst than the buffer
        // could hold. It's also hard to track the overhead per packet on Linux. With MTU=1500 payload size can be at
        // most 1480 bytes on ipv4 and the overhead will be either 768, 1280 or 2304 depending on the payload size.
        // Another complication is that the kernel allows the send-buffer to overflow by 1 message + (overhead - 1 byte)
        // because it checks whether the current amount of memory allocated for a socket is lower than the limit without
        // taking into account the size of the message being sent. Unless the message could not fit in the buffer even
        // if it was empty. For example, with the default buffer size of 212992 it's possible to send 167 messages of
        // 256 bytes, each taking 1280 bytes in the buffer to a total 213760 bytes until send(2) fails with EWOULDBLOCK.
        // Finally, the way we divide buffer size settings by 2 on Linux causes odd numbers to be effectively rounded
        // down to an even number, so we have to stick to using even buffer sizes. This is all included in the API
        // documentation. For example, in order to be able to send 10 messages of 256 bytes (each taking 1280 bytes with
        // overhead included) the send_buffer_size setting must be at least 1280*9+2 = 11522 and not 1280*9+1 (= 11521).
        // Path MTU discovery is assumed disabled so packets are expected to be fragmented if payload + headers > MTU.

        // Setting the buffer size to -1 has the effect of using the system's default value which on Linux x64 is 212992.

        {   0,  any,    remote,     -1,               0,    1,       CL_ENONE },
        {   1,  any,    remote,     -1,               1,    1,       CL_ENONE },
        {   2,  any,    remote,     -1,               2,    1,       CL_ENONE },
        {   3,  any,    remote,     -1,               4,    1,       CL_ENONE },
        {   4,  any,    remote,     -1,               8,    1,       CL_ENONE },
        {   5,  any,    remote,     -1,              16,    1,       CL_ENONE },
        {   6,  any,    remote,     -1,              32,    1,       CL_ENONE },
        {   7,  any,    remote,     -1,              64,    1,       CL_ENONE },
        {   8,  any,    remote,     -1,             128,    1,       CL_ENONE },
        {   9,  any,    remote,     -1,             256,    1,       CL_ENONE },
        {  10,  any,    remote,     -1,             512,    1,       CL_ENONE },
        {  11,  any,    remote,     -1,            1024,    1,       CL_ENONE },
        {  12,  any,    remote,     -1,            2048,    1,       CL_ENONE },
        {  13,  any,    remote,     -1,            4096,    1,       CL_ENONE },
        {  14,  any,    remote,     -1,            8192,    1,       CL_ENONE },
        {  15,  any,    remote,     -1,           16384,    1,       CL_ENONE },
        {  16,  any,    remote,     -1,           32768,    1,       CL_ENONE },
        {  17,  any,    remote,     -1,           65507,    1,       CL_ENONE },
        {  18,  any,    remote,     -1,           65508,    1,    CL_EMSGSIZE },
        {  19,  any,    remote,     -1,           65535,    1,    CL_EMSGSIZE },

        // Setting the buffer size to 0 has the effect of using the system's minimum value which on Linux x64 is 4608.

        {  20,  any,    remote,      0,               0,    1,       CL_ENONE },
        {  21,  any,    remote,      0,               1,    1,       CL_ENONE },
        {  22,  any,    remote,      0,               2,    1,       CL_ENONE },
        {  23,  any,    remote,      0,               4,    1,       CL_ENONE },
        {  24,  any,    remote,      0,               8,    1,       CL_ENONE },
        {  25,  any,    remote,      0,              16,    1,       CL_ENONE },
        {  26,  any,    remote,      0,              32,    1,       CL_ENONE },
        {  27,  any,    remote,      0,              64,    1,       CL_ENONE },
        {  28,  any,    remote,      0,             128,    1,       CL_ENONE },
        {  29,  any,    remote,      0,             256,    1,       CL_ENONE },
        {  30,  any,    remote,      0,             512,    1,       CL_ENONE },
        {  31,  any,    remote,      0,            1024,    1,       CL_ENONE },
        {  32,  any,    remote,      0,            2048,    1,       CL_ENONE },

        // With MTU=1500, a message of 4096 bytes will produce 3 ipv4 fragments with payloads of 1472, 1480 and 1142
        // bytes respectively. The first fragment bypasses the buffer but the other 2 are buffered and require 2304
        // bytes each so the total buffer space used is 2304 * 2 = 4608 bytes. Since we're setting the buffer size to
        // 256, which is below minimum, the system will round it up to 4608 on x64 (the minimum) which is exactly the
        // size we need.
        // TODO: 33 and 34 may fail on x86 because the minimum send buffer size might be lower than 4608 as it depends
        // on the size of struct sk_buff aligned to 32. Should we have an ifdef for x86 and expect CL_ENOBUFS
        // instead ?

        {  33,  any,    remote,      0,            4096,    1,       CL_ENONE },

        // With MTU=1500, a message of 4432 bytes fragments into payloads of 1472, 1480 and 1480 bytes respectively. So
        // this is the maximum message size the minimum buffers can transmit.

        {  34,  any,    remote,      0,            4432,    1,       CL_ENONE },

        // With MTU=1500, a message of 4433 bytes will produce 4 fragments: 2 with 1480 bytes of payload, 1 with 1473
        // bytes of payload and the last one with 0 payload bytes just carrying the udp header.

        {  35,  any,    remote,      0,            4433,    1,     CL_ENOBUFS },
        {  34,  any,    remote,      0,            8192,    1,     CL_ENOBUFS },
        {  35,  any,    remote,      0,           16384,    1,     CL_ENOBUFS },
        {  36,  any,    remote,      0,           32768,    1,     CL_ENOBUFS },
        {  37,  any,    remote,      0,           65507,    1,     CL_ENOBUFS },
        {  38,  any,    remote,      0,           65508,    1,    CL_EMSGSIZE },
        {  39,  any,    remote,      0,           65535,    1,    CL_EMSGSIZE },

        // The packet overhead in Linux is directly related to the payload size. A payload size of 16 bytes requires 768
        // bytes of buffer space (to send).

        {  40,  any,    remote,    768 *  99,         1,  100,      CL_EAGAIN },
        {  41,  any,    remote,    768 *  99 + 1,     1,  100,      CL_EAGAIN },
        {  42,  any,    remote,    768 *  99 + 2,     1,  100,       CL_ENONE },
        {  43,  any,    remote,    768 *  99 + 2,     5,  100,       CL_ENONE },
        {  44,  any,    remote,    768 *  99 + 2,     6,  100,      CL_EAGAIN },
        {  45,  any,    remote,   1280 *  99,         6,  100,      CL_EAGAIN },
        {  46,  any,    remote,   1280 *  99 + 1,     6,  100,      CL_EAGAIN },
        {  47,  any,    remote,   1280 *  99 + 2,     6,  100,       CL_ENONE },
        {  48,  any,    remote,   1280 *  99 + 2,   517,  100,       CL_ENONE },
        {  49,  any,    remote,   1280 *  99 + 2,   518,  100,      CL_EAGAIN },
        {  50,  any,    remote,   2304 *  99,       518,  100,      CL_EAGAIN },
        {  51,  any,    remote,   2304 *  99 + 1,   518,  100,      CL_EAGAIN },
        {  52,  any,    remote,   2304 *  99 + 2,   518,  100,       CL_ENONE },
        {  53,  any,    remote,   2304 *  99 + 2,  1472,  100,       CL_ENONE },
        {  54,  any,    remote,   2304 * 299,      4096,  100,      CL_EAGAIN },
        {  55,  any,    remote,   2304 * 299 + 1,  4096,  100,      CL_EAGAIN },

        // The following samples require net.core.wmem_max=6398722

        {  56,  any,    remote,   2304 *  299 + 2, 4096,  100,       CL_ENONE },
        {  57,  any,    remote,   1280 *  199,       16,  200,      CL_EAGAIN },
        {  58,  any,    remote,   1280 *  199 + 2,   16,  200,       CL_ENONE },
        {  59,  any,    remote,   1280 * 4999,       48, 5000,      CL_EAGAIN },
        {  60,  any,    remote,   1280 * 4999 + 2,   48, 5000,       CL_ENONE },
        {  61,  any,    remote,   1280 * 4999,       49, 5000,      CL_EAGAIN },
        {  62,  any,    remote,   1280 * 4999 + 2,   49, 5000,       CL_ENONE },

        #elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )

        // BSD/Drawin does not have a proper send buffer for UDP sockets. Instead the value of SO_SNDBUF is only used to
        // limit the size of a the message that can be transmitted by the socket.

        // Setting the buffer size to -1 has the effect of using the system's default value which on Darwin is the
        // value of sysctl:net.inet.udp.maxdgram.

        {   0,  any,    remote,     -1,               0,    1,       CL_ENONE },
        {   1,  any,    remote,     -1,               1,    1,       CL_ENONE },
        {   2,  any,    remote,     -1,               2,    1,       CL_ENONE },
        {   3,  any,    remote,     -1,               4,    1,       CL_ENONE },
        {   4,  any,    remote,     -1,               8,    1,       CL_ENONE },
        {   5,  any,    remote,     -1,              16,    1,       CL_ENONE },
        {   6,  any,    remote,     -1,              32,    1,       CL_ENONE },
        {   7,  any,    remote,     -1,              64,    1,       CL_ENONE },
        {   8,  any,    remote,     -1,             128,    1,       CL_ENONE },
        {   9,  any,    remote,     -1,             256,    1,       CL_ENONE },
        {  10,  any,    remote,     -1,             512,    1,       CL_ENONE },
        {  11,  any,    remote,     -1,            1024,    1,       CL_ENONE },
        {  12,  any,    remote,     -1,            2048,    1,       CL_ENONE },
        {  13,  any,    remote,     -1,            4096,    1,       CL_ENONE },
        {  14,  any,    remote,     -1,            8192,    1,       CL_ENONE },

        // Setting the buffer size to 0 also falls back to using the system's default in our implementation because
        // BSD/Darwin would otherwise reject it with EINVAL.

        {  15,  any,    remote,      0,               0,    1,       CL_ENONE },
        {  16,  any,    remote,      0,               1,    1,       CL_ENONE },
        {  17,  any,    remote,      0,               2,    1,       CL_ENONE },
        {  18,  any,    remote,      0,               4,    1,       CL_ENONE },
        {  19,  any,    remote,      0,               8,    1,       CL_ENONE },
        {  20,  any,    remote,      0,              16,    1,       CL_ENONE },
        {  21,  any,    remote,      0,              32,    1,       CL_ENONE },
        {  22,  any,    remote,      0,              64,    1,       CL_ENONE },
        {  23,  any,    remote,      0,             128,    1,       CL_ENONE },
        {  24,  any,    remote,      0,             256,    1,       CL_ENONE },
        {  25,  any,    remote,      0,             512,    1,       CL_ENONE },
        {  26,  any,    remote,      0,            1024,    1,       CL_ENONE },
        {  27,  any,    remote,      0,            2048,    1,       CL_ENONE },
        {  28,  any,    remote,      0,            4096,    1,       CL_ENONE },
        {  29,  any,    remote,      0,            8192,    1,       CL_ENONE },

        // There is no overhead per message in the send buffer and there is no system minimum.

        {  30,  any,    remote,      1,               0,    1,       CL_ENONE },
        {  31,  any,    remote,      1,               1,    1,       CL_ENONE },
        {  32,  any,    remote,      1,               2,    1,    CL_EMSGSIZE },
        {  33,  any,    remote,   8192,               4,    1,       CL_ENONE },
        {  34,  any,    remote,   8192,               8,    1,       CL_ENONE },
        {  35,  any,    remote,   8192,              16,    1,       CL_ENONE },
        {  36,  any,    remote,   8192,              32,    1,       CL_ENONE },
        {  37,  any,    remote,   8192,              64,    1,       CL_ENONE },
        {  38,  any,    remote,   8192,             128,    1,       CL_ENONE },
        {  39,  any,    remote,   8192,             256,    1,       CL_ENONE },
        {  40,  any,    remote,   8192,             512,    1,       CL_ENONE },
        {  41,  any,    remote,   8192,            1024,    1,       CL_ENONE },
        {  42,  any,    remote,   8192,            2048,    1,       CL_ENONE },
        {  43,  any,    remote,   8192,            4096,    1,       CL_ENONE },
        {  44,  any,    remote,   8192,            8192,    1,       CL_ENONE },
        {  45,  any,    remote,   8191,            8192,    2,    CL_EMSGSIZE },
        {  46,  any,    remote,   8192,            8192,    2,       CL_ENONE },

        // With MTU=1500, a message of 4096 bytes will produce 3 ipv4 fragments with payloads of 1472, 1480 and 1142
        // bytes respectively but since there is no actual send buffer we should be able to send as many fragments as
        // the network layer can queue. The problem is that if the queue is full packets are silently dropped and there
        // is no way around it. The call to send(2) doesn't fail and the user program never gets to know those packets
        // were never transmitted.

        {  47,  any,    remote,   4096,            4096,    1,       CL_ENONE },
        {  48,  any,    remote,   4096,            4096,    2,       CL_ENONE },
        {  49,  any,    remote,   4096,            4096,   10,       CL_ENONE },
        {  50,  any,    remote,   4096,            4096,  100,       CL_ENONE },
        {  51,  any,    remote,   4096,            4096,  500,       CL_ENONE },
        {  52,  any,    remote,   4096,              48, 5000,       CL_ENONE },
        {  53,  any,    remote,   4096,              49, 5000,       CL_ENONE },
        {  54,  any,    remote,   4096,            4097, 5000,    CL_EMSGSIZE },

        // The following samples require net.inet.udp.maxdgram=65535

        {  55,  any,    remote,     -1,           16384,    1,     CL_ENOBUFS },
        {  56,  any,    remote,     -1,           32768,    1,     CL_ENOBUFS },
        {  57,  any,    remote,     -1,           65507,    1,     CL_ENOBUFS },
        {  58,  any,    remote,     -1,           65508,    1,    CL_EMSGSIZE },
        {  59,  any,    remote,     -1,           65535,    1,    CL_EMSGSIZE },

        {  60,  any,    remote,      0,           16384,    1,     CL_ENOBUFS },
        {  61,  any,    remote,      0,           32768,    1,     CL_ENOBUFS },
        {  62,  any,    remote,      0,           65507,    1,     CL_ENOBUFS },
        {  63,  any,    remote,      0,           65508,    1,    CL_EMSGSIZE },
        {  64,  any,    remote,      0,           65535,    1,    CL_EMSGSIZE },

        {  65,  any,    remote,  65535,           16384,    1,     CL_ENOBUFS },
        {  66,  any,    remote,  65535,           32768,    1,     CL_ENOBUFS },
        {  67,  any,    remote,  65535,           65507,    1,     CL_ENOBUFS },
        {  68,  any,    remote,  65535,           65508,    1,    CL_EMSGSIZE },
        {  69,  any,    remote,  65535,           65535,    1,    CL_EMSGSIZE },

        #endif
    };
    // @formatter:on

    SAMPLES(data);

    int sample;
    cl_endpoint source;
    cl_endpoint destination;
    int send_buffer_size;
    int send_length;
    int count;
    int expected;

    std::tie(sample, source, destination,
        send_buffer_size, send_length,
        count,
        expected) = GENERATE_REF(from_samples(data));

    TEST_SEND(sample, &source, &destination, send_buffer_size, send_length, count, expected);
}

// TODO: Use echo server from https://github.com/Kong/tcpbin or echo.u-blox.com
#if 0

// TODO: define separate TEST cases for Windows, Linux and macOS highlighting their particularities.

TEST_CASE("Recv", "[udp]")
{
    const cl_endpoint loopback = { cl_addr_loopback_ipv4, 0 };
    #if CL_ENABLE_IPV6
    const cl_endpoint loopback6 = { cl_addr_loopback_ipv6, 0 };
    #endif

    // @formatter:off
    // Table: sample | source | destination | send buffer size | send length | recv buffer size | recv length | count | expected
    const std::vector<std::tuple<int, cl_endpoint, cl_endpoint, int, int, int, int, int, int>> data = {
        {  0,   loopback,     loopback,  2048,     0, 65535,   65535,  1,       CL_ENONE },
        {  1,   loopback,     loopback,  2048,     1, 65535,   65535,  3,       CL_ENONE },
        {  2,   loopback,     loopback,  2048,     2, 65535,   65535,  3,       CL_ENONE },
        {  3,   loopback,     loopback,  2048,     4, 65535,   65535,  3,       CL_ENONE },
        {  4,   loopback,     loopback,  2048,     8, 65535,   65535,  3,       CL_ENONE },
        {  5,   loopback,     loopback,  2048,    16, 65535,   65535,  3,       CL_ENONE },
        {  6,   loopback,     loopback,  2048,    32, 65535,   65535,  3,       CL_ENONE },
        {  7,   loopback,     loopback,  2048,    64, 65535,   65535,  2,       CL_ENONE },
        {  8,   loopback,     loopback,  2048,   128, 65535,   65535,  2,       CL_ENONE },
        {  9,   loopback,     loopback,  2048,   256, 65535,   65535,  2,       CL_ENONE },
        { 10,   loopback,     loopback,  2048,   512, 65535,   65535,  1,       CL_ENONE },
        { 11,   loopback,     loopback,  2048,  1024,  2304,   65535,  1,       CL_ENONE },
        { 12,   loopback,     loopback,  3328,  2048, 65535,   65535,  1,       CL_ENONE },
        { 13,   loopback,     loopback,  3328,  2048,  2048,    2047,  1,    CL_EMSGSIZE },
        { 14,   loopback,     loopback,  3328,  2048,  2048,    2048,  2,      CL_EAGAIN },
        // On Linux x64 an ipv4 datagram with 2048 bytes consume 4352 bytes from the buffer but in this test the NIC is
        // not under stress so the first datagram arrives bypassing the buffer and the actual recv buffer space required
        // is 4352 * 1 = 4352
        { 15,   loopback,     loopback,  2047,  2048,  4352,    2048,  2,      CL_EAGAIN },
        // On Linux x64 an ipv4 datagram with 1024 bytes consume 2304 bytes from the buffer but in this test the NIC is
        // not under stress so the first datagram arrives bypassing the buffer and the actual recv buffer space required
        // is 2304 * 9 = 20736
        { 15,   loopback,     loopback, 16640,  1024, 23040,   65535, 10,       CL_ENONE },
        // On Linux x64 an ipv4 datagram with 512 bytes consume 1280 bytes from the buffer but in this test the NIC is
        // not under stress so the first datagram arrives bypassing the buffer and the actual recv buffer space required
        // is 1280 * 9 = 115200
        { 16,   loopback,     loopback, 11520,  512,  12800,   65535, 10,       CL_ENONE }, // On Linux x64 an ipv4 datagram with 512 bytes consume 1280 bytes from the buffer
        { 17,   loopback,     loopback, 93667, 65507, 65507,   65507,  1,       CL_ENONE },
        { 18,   loopback,     loopback, 93667, 65507, 65507,   65507,  1,       CL_ENONE },
        { 19,   loopback,     loopback, 93667, 65507, 65535,   65535,  1,       CL_ENONE },
        { 20,   loopback,     loopback, 93667, 65507,  2048,   65535,  1,      CL_EAGAIN },
        { 21,   loopback,     loopback, 93667, 65507, 65535,   65506,  1,    CL_EMSGSIZE },
        { 22,   loopback,     loopback, 93667, 65507, 65535,   65507,  1,                65507 },
        { 23,   loopback,     loopback, 93667, 65507, 65535,   65535,  1,                65507 },
        { 24,   loopback,     loopback, 93667, 65507, 65535, 1048576,  1,                65507 },
        #if CL_ENABLE_IPV6
        { 25,  loopback6,    loopback6, 93667, 65527, 65527,   65527,  1,       CL_ENONE },
        { 26,  loopback6,    loopback6, 93667, 65527, 65527,   65527,  1,       CL_ENONE },
        { 27,  loopback6,    loopback6, 93667, 65527, 65535,   65535,  1,       CL_ENONE },
        { 28,  loopback6,    loopback6, 93667, 65527,  2048,   65535,  1,      CL_EAGAIN },
        { 29,  loopback6,    loopback6, 93667, 65527, 65535,   65526,  1,    CL_EMSGSIZE },
        { 30,  loopback6,    loopback6, 93667, 65527, 65535,   65527,  1,                65507 },
        { 31,  loopback6,    loopback6, 93667, 65527, 65535,   65535,  1,                65507 },
        { 32,  loopback6,    loopback6, 93667, 65527, 65535, 1048576,  1,                65507 },
        #endif
        };
    // @formatter:on

    SAMPLES(data);

    int sample;
    cl_endpoint source;
    cl_endpoint destination;
    int send_buffer_size;
    int send_length;
    int recv_buffer_size;
    int recv_length;
    int count;
    int expected;

    std::tie(sample, source, destination,
             send_buffer_size, send_length,
             recv_buffer_size, recv_length,
             count,
             expected) = GENERATE_REF(from_samples(data));

    FROM(sample);

    // sanity: avoid obvious invalid test params
    REQUIRE(send_length <= INT_MAX);
    REQUIRE(recv_length <= INT_MAX);
    REQUIRE(count > 0);
    REQUIRE(expected <= (int)send_length);

    cl_udp_settings settings = cl_udp_settings_default;
    settings.send_buffer_size = (uint32_t)send_buffer_size;
    settings.recv_buffer_size = (uint32_t)recv_buffer_size;
    settings.ttl = 4;

    cl_udp_socket* sender = nullptr;
    int errcode = cl_udp_open(&sender, &source, &settings, 0);
    const finalizer sender_dtor = [&]
    {
        cl_udp_close(&sender);
    };
    REQUIRE(Error(errcode) == Error(CL_ENONE));
    REQUIRE(sender != nullptr);

    const size_t send_buf_len = (size_t)send_length * count;
    void* send_buf = malloc(send_buf_len);
    REQUIRE(send_buf != nullptr);
    const finalizer send_buf_dtor = [&]
    {
        free(send_buf);
    };

    memnoise(send_buf, send_buf_len);

    cl_udp_socket* receiver = nullptr;
    errcode = cl_udp_open(&receiver, &destination, &settings, 0);
    const finalizer receiver_dtor = [&]
    {
        cl_udp_close(&receiver);
    };
    REQUIRE(Error(errcode) == Error(CL_ENONE));
    REQUIRE(receiver != nullptr);

    void* recv_buf = malloc((size_t)recv_length);
    REQUIRE(recv_buf != nullptr);
    const finalizer recv_buf_dtor = [&]
    {
        free(recv_buf);
    };

    errcode = cl_udp_get_bind(receiver, &destination);
    REQUIRE(Error(errcode) == Error(CL_ENONE));

    for (int packet = 0; packet < count; ++packet)
    {
        FROM(packet);

        int n = cl_udp_send(sender, (void*)((uint8_t*)send_buf + (send_length * packet)), (size_t)send_length, &destination);
        REQUIRE(Error(n) == Error(send_length));
    }

    for (int packet = 0; packet < count; ++packet)
    {
        FROM(packet);

        int n = cl_udp_recv(receiver, recv_buf, (size_t)recv_length, &source);
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
    SECTION("Get CL_SO_REUSEADDR")
    {

    }

    SECTION("Get CL_SO_KEEPALIVE")
    {

    }

    SECTION("Get CL_SO_IPV6DUAL")
    {

    }

    SECTION("Get CL_SO_TTL")
    {

    }

    SECTION("Get CL_SO_SNDBUF")
    {

    }

    SECTION("Get CL_SO_RCVBUF")
    {

    }

    SECTION("Get CL_SO_LINGER")
    {

    }

    SECTION("Get CL_SO_DONTLINGER")
    {

    }

    SECTION("Set CL_SO_REUSEADDR")
    {
        // TODO: should fail
    }

    SECTION("Set CL_SO_KEEPALIVE")
    {

    }

    SECTION("Set CL_SO_IPV6DUAL")
    {
        // TODO: should fail
    }

    SECTION("Set CL_SO_TTL")
    {
        // TODO: should fail
    }

    SECTION("Set CL_SO_SNDBUF")
    {
        // TODO: should fail
    }

    SECTION("Set CL_SO_RCVBUF")
    {
        // TODO: should fail
    }

    SECTION("Set CL_SO_LINGER")
    {

    }

    SECTION("Set CL_SO_DONTLINGER")
    {

    }
}

#endif // 0
