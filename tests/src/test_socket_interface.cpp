#include "test.h"

#if defined(__linux__)
#include <fstream>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

static starter init([] // NOLINT(cert-err58-cpp)
{
    CLARINET_TEST_DEPENDS_ON_IPV6();
});

// Scope initialize and finalize the library
static autoload loader;

#define CLARINET_TEST_DECLARE_LIST_ITEM(i)                      (i),
#define CLARINET_TEST_DECLARE_BSET_ITEM(i)                      | (i)

#if CLARINET_ENABLE_IPV6
#define CLARINET_TEST_DECLARE_LIST_ITEM_WHEN_IPV6_ENABLED(i)    CLARINET_TEST_DECLARE_LIST_ITEM(i)
#define CLARINET_TEST_DECLARE_LIST_ITEM_WHEN_IPV6_DISABLED(i)
#else
#define CLARINET_TEST_DECLARE_LIST_ITEM_WHEN_IPV6_ENABLED(i)
#define CLARINET_TEST_DECLARE_LIST_ITEM_WHEN_IPV6_DISABLED(i)   CLARINET_TEST_DECLARE_LIST_ITEM(i)
#endif

// All families compatible with clarinet_socket_open():  D - Declare; X - Conditional Declare
#define CLARINET_TEST_SOCKET_OPEN_SUPPORTED_FAMILIES(D, X) \
    D(CLARINET_AF_INET) \
    X(CLARINET_AF_INET6) \

// All families incompatible with clarinet_socket_open():  D - Declare; X - Conditional Declare
#define CLARINET_TEST_SOCKET_OPEN_UNSUPPORTED_FAMILIES(D, X) \
    D(CLARINET_AF_UNSPEC) \
    X(CLARINET_AF_INET6) \
    D(CLARINET_AF_LINK) \

// All protocols compatible with clarinet_socket_open()
#define CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTOS(D) \
    D(CLARINET_PROTO_UDP) \
    D(CLARINET_PROTO_TCP) \

// All protocols incompatible with clarinet_socket_open()
#define CLARINET_TEST_SOCKET_OPEN_UNSUPPORTED_PROTOS(D) \
    D(CLARINET_PROTO_NONE) \
    D(CLARINET_PROTO_DTLC) \
    D(CLARINET_PROTO_DTLS) \
    D(CLARINET_PROTO_TLS) \
    D(CLARINET_PROTO_GDTP) \
    D(CLARINET_PROTO_GDTPS) \
    D(CLARINET_PROTO_ENET) \
    D(CLARINET_PROTO_ENETS) \

// All protocols compatible with clarinet_socket_listen() and clarinet_socket_accept()
#define CLARINET_TEST_SOCKET_LISTEN_SUPPORTED_PROTOS(D) \
    D(CLARINET_PROTO_TCP) \

// All protocols incompatible with clarinet_socket_listen()and clarinet_socket_accept()
#define CLARINET_TEST_SOCKET_LISTEN_UNSUPPORTED_PROTOS(D) \
    D(CLARINET_PROTO_NONE) \
    D(CLARINET_PROTO_UDP) \
    D(CLARINET_PROTO_DTLC) \
    D(CLARINET_PROTO_DTLS) \
    D(CLARINET_PROTO_TLS) \
    D(CLARINET_PROTO_GDTP) \
    D(CLARINET_PROTO_GDTPS) \
    D(CLARINET_PROTO_ENET) \
    D(CLARINET_PROTO_ENETS) \

// All protocols compatible with clarinet_socket_connect()
#define CLARINET_TEST_SOCKET_CONNECT_SUPPORTED_PROTOS(D) \
    D(CLARINET_PROTO_UDP) \
    D(CLARINET_PROTO_TCP) \

// All protocols incompatible with clarinet_socket_connect()
#define CLARINET_TEST_SOCKET_CONNECT_UNSUPPORTED_PROTOS(D) \
    D(CLARINET_PROTO_NONE) \
    D(CLARINET_PROTO_DTLC) \
    D(CLARINET_PROTO_DTLS) \
    D(CLARINET_PROTO_TLS) \
    D(CLARINET_PROTO_GDTP) \
    D(CLARINET_PROTO_GDTPS) \
    D(CLARINET_PROTO_ENET) \
    D(CLARINET_PROTO_ENETS) \

#define CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST             CLARINET_TEST_SOCKET_OPEN_SUPPORTED_FAMILIES(CLARINET_TEST_DECLARE_LIST_ITEM, CLARINET_TEST_DECLARE_LIST_ITEM_WHEN_IPV6_ENABLED)
#define CLARINET_TEST_SOCKET_OPEN_UNSUPPORTED_AF_LIST           CLARINET_TEST_SOCKET_OPEN_UNSUPPORTED_FAMILIES(CLARINET_TEST_DECLARE_LIST_ITEM, CLARINET_TEST_DECLARE_LIST_ITEM_WHEN_IPV6_DISABLED)

#define CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST          CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTOS(CLARINET_TEST_DECLARE_LIST_ITEM)
#define CLARINET_TEST_SOCKET_OPEN_UNSUPPORTED_PROTO_LIST        CLARINET_TEST_SOCKET_OPEN_UNSUPPORTED_PROTOS(CLARINET_TEST_DECLARE_LIST_ITEM)
#define CLARINET_TEST_SOCKET_OPEN_PROTO_BSET                 (0 CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTOS(CLARINET_TEST_DECLARE_BSET_ITEM))

#define CLARINET_TEST_SOCKET_LISTEN_SUPPORTED_PROTO_LIST        CLARINET_TEST_SOCKET_LISTEN_SUPPORTED_PROTOS(CLARINET_TEST_DECLARE_LIST_ITEM)
#define CLARINET_TEST_SOCKET_LISTEN_UNSUPPORTED_PROTO_LIST      CLARINET_TEST_SOCKET_LISTEN_UNSUPPORTED_PROTOS(CLARINET_TEST_DECLARE_LIST_ITEM)
#define CLARINET_TEST_SOCKET_LISTEN_PROTO_BSET               (0 CLARINET_TEST_SOCKET_LISTEN_SUPPORTED_PROTOS(CLARINET_TEST_DECLARE_BSET_ITEM))

#define CLARINET_TEST_SOCKET_CONNECT_SUPPORTED_PROTO_LIST       CLARINET_TEST_SOCKET_CONNECT_SUPPORTED_PROTOS(CLARINET_TEST_DECLARE_LIST_ITEM)
#define CLARINET_TEST_SOCKET_CONNECT_UNSUPPORTED_PROTO_LIST     CLARINET_TEST_SOCKET_CONNECT_UNSUPPORTED_PROTOS(CLARINET_TEST_DECLARE_LIST_ITEM)
#define CLARINET_TEST_SOCKET_CONNECT_PROTO_BSET              (0 CLARINET_TEST_SOCKET_CONNECT_SUPPORTED_PROTOS(CLARINET_TEST_DECLARE_BSET_ITEM))

// Listen and Accept protocols always match. These definitions are just for readability.
#define CLARINET_TEST_SOCKET_ACCEPT_SUPPORTED_PROTO_LIST        CLARINET_TEST_SOCKET_LISTEN_SUPPORTED_PROTO_LIST
#define CLARINET_TEST_SOCKET_ACCEPT_UNSUPPORTED_PROTO_LIST      CLARINET_TEST_SOCKET_LISTEN_UNSUPPORTED_PROTO_LIST
#define CLARINET_TEST_SOCKET_ACCEPT_PROTO_BSET                  CLARINET_TEST_SOCKET_LISTEN_PROTO_BSET

// TODO: Test BLOCKING and NON-BLOCKING socket variations in Connect, Accept, Send and Recv
// TODO: create tests for CLARINET_IP_TTL values < 1 and > 255 and document for each platform
// TODO: Test Listen with more TCP connections than backlog
// TODO: Test Listen with backlog=0
// TODO: Test Listen with backlog=-1
// TODO: Test and document multiple calls to Listen on different platforms.
// TODO: Test Connect BLOCKING connect on non-listening remote socket
// TODO: Test Connect NON-BLOCKING connect on non-listening remote socket
// TODO: Test Connect with more TCP connections than backlog
// TODO: Test Connect for the issue with connecting UDP sockets using REUSEADDR on Windows (vs Linux and Darwin)
// TODO: Test Accept socket does not inherit NON-BLOCKING option
// TODO: Test Accept for which options are inherited by the accepted socket

TEST_CASE("Socket Initialize")
{
    SECTION("With memory unmodified")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);
        REQUIRE(sp->family == CLARINET_AF_UNSPEC);
    }

    SECTION("With memory set to all zeroes")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        memset(sp, 0, sizeof(clarinet_socket));

        clarinet_socket_init(sp);
        REQUIRE(sp->family == CLARINET_AF_UNSPEC);
    }

    SECTION("With memory set to all ones")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        memset(sp, 0xFF, sizeof(clarinet_socket));

        clarinet_socket_init(sp);
        REQUIRE(sp->family == CLARINET_AF_UNSPEC);
    }
}

TEST_CASE("Socket Open/Close")
{
    SECTION("Open")
    {
        SECTION("With NULL socket")
        {
            clarinet_family family = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
                CLARINET_TEST_SOCKET_OPEN_UNSUPPORTED_AF_LIST
            }));
            clarinet_proto proto = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
                CLARINET_TEST_SOCKET_OPEN_UNSUPPORTED_PROTO_LIST
            }));

            FROM(family);
            FROM(proto);

            int errcode = clarinet_socket_open(nullptr, family, proto);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("With an UNSUPPORTED family")
        {
            clarinet_family family = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_UNSUPPORTED_AF_LIST
            }));
            clarinet_proto proto = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
                CLARINET_TEST_SOCKET_OPEN_UNSUPPORTED_PROTO_LIST
            }));

            FROM(family);
            FROM(proto);

            clarinet_socket socket;
            clarinet_socket* sp = &socket;
            clarinet_socket_init(sp);
            REQUIRE(sp->family == CLARINET_AF_UNSPEC);

            int errcode = clarinet_socket_open(sp, family, proto);
            REQUIRE(Error(errcode) == Error(CLARINET_EAFNOSUPPORT));
        }

        SECTION("With an UNSUPPORTED protocol")
        {
            clarinet_family family = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
            }));

            clarinet_proto proto = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_UNSUPPORTED_PROTO_LIST
            }));

            FROM(family);
            FROM(proto);

            clarinet_socket socket;
            clarinet_socket* sp = &socket;
            clarinet_socket_init(sp);

            int errcode = clarinet_socket_open(sp, family, proto);
            REQUIRE(Error(errcode) == Error(CLARINET_EPROTONOSUPPORT));
        }

        SECTION("With COMPATIBLE family and protocol")
        {
            clarinet_family family = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
            }));

            clarinet_proto proto = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
            }));

            FROM(family);
            FROM(proto);

            clarinet_socket socket;
            clarinet_socket* sp = &socket;
            clarinet_socket_init(sp);

            int errcode = clarinet_socket_open(sp, family, proto);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            CHECK(sp->family == family);

            errcode = clarinet_socket_close(sp);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            REQUIRE(sp->family == CLARINET_AF_UNSPEC);
        }

        SECTION("SAME socket TWICE")
        {
            clarinet_family family = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
            }));

            clarinet_proto proto = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
            }));

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
    }

    SECTION("Close")
    {
        SECTION("With NULL socket")
        {
            int errcode = clarinet_socket_close(nullptr);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("With UNOPEN socket")
        {
            clarinet_socket socket;
            clarinet_socket* sp = &socket;
            clarinet_socket_init(sp);

            int errcode = clarinet_socket_close(&socket);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }

        SECTION("SAME socket TWICE")
        {
            clarinet_family family = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
            }));

            clarinet_proto proto = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
            }));

            FROM(family);
            FROM(proto);

            clarinet_socket socket;
            clarinet_socket* sp = &socket;
            clarinet_socket_init(sp);

            int errcode = clarinet_socket_open(sp, family, proto);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            errcode = clarinet_socket_close(sp);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            // It should be safe to close a socket multiple times as long as the previous attempt did not fail
            errcode = clarinet_socket_close(sp);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        }
    }
}

TEST_CASE("Socket Get/Set Option")
{
    const int32_t VAL_INIT = -1; // 0xFFFFFFFF
    const size_t LEN_INIT = 4 * sizeof(int32_t); // deliberately larger than sizeof(int32_t)

    SECTION("With NULL socket")
    {
        int32_t val = VAL_INIT;
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

    SECTION("With INVALID option arguments")
    {
        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));

        FROM(family);
        FROM(proto);

        // Run the same tests with UNOPEN, CLOSED and OPEN sockets

        clarinet_socket unopen_socket;
        clarinet_socket* usp = &unopen_socket;
        clarinet_socket_init(usp);

        clarinet_socket closed_socket;
        clarinet_socket* csp = &closed_socket;
        clarinet_socket_init(csp);

        int errcode = clarinet_socket_open(csp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        errcode = clarinet_socket_close(csp);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_socket open_socket;
        clarinet_socket* osp = &open_socket;
        clarinet_socket_init(osp);

        errcode = clarinet_socket_open(osp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&osp]
        {
            clarinet_socket_close(osp);
        });

        const char* state;
        clarinet_socket* sp;

        // @formatter:off
        std::tie(state, sp) = GENERATE_REF(table<const char*, clarinet_socket*>({
            { "UNOPEN", usp },
            { "CLOSED", csp },
            { "OPEN",   osp }
        }));
        // @formatter:on

        FROM(state);

        int32_t val = VAL_INIT;
        size_t len = LEN_INIT;

        SECTION("With INVALID optname")
        {
            int optname = GENERATE(-1, 0, 1 << 16, 1 << 24, INT_MAX);
            FROM(optname);

            SECTION("Get option")
            {
                errcode = clarinet_socket_getopt(sp, optname, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(val == VAL_INIT);
                REQUIRE(len == LEN_INIT);

                errcode = clarinet_socket_getopt(sp, optname, nullptr, nullptr);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(val == VAL_INIT);
                REQUIRE(len == LEN_INIT);
            }

            SECTION("Set option")
            {
                errcode = clarinet_socket_setopt(sp, optname, &val, sizeof(val));
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(val == VAL_INIT);

                errcode = clarinet_socket_setopt(sp, optname, nullptr, 0);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            }
        }

        SECTION("With VALID optname but INVALID optval/optlen")
        {
            #define TABLE_ITEM(s) { #s "(" CLARINET_XSTR(s) ")", (s) }
            const char* option;
            int optname;
            std::tie(option, optname) = GENERATE(table<const char*, int>({
                TABLE_ITEM(CLARINET_SO_NONBLOCK),
                TABLE_ITEM(CLARINET_SO_REUSEADDR),
                TABLE_ITEM(CLARINET_SO_SNDBUF),
                TABLE_ITEM(CLARINET_SO_RCVBUF),
                TABLE_ITEM(CLARINET_SO_SNDTIMEO),
                TABLE_ITEM(CLARINET_SO_RCVTIMEO),
                TABLE_ITEM(CLARINET_SO_KEEPALIVE),
                TABLE_ITEM(CLARINET_SO_LINGER),
                TABLE_ITEM(CLARINET_SO_DONTLINGER),
                TABLE_ITEM(CLARINET_IP_TTL),
                TABLE_ITEM(CLARINET_IP_V6ONLY),
                TABLE_ITEM(CLARINET_IP_MTU),
                TABLE_ITEM(CLARINET_IP_MTU_DISCOVER),
            }));
            #undef TABLE_ITEM
            FROM(option);

            SECTION("With NULL optval")
            {
                SECTION("Get Option")
                {
                    errcode = clarinet_socket_getopt(sp, optname, nullptr, nullptr);
                    REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));

                    errcode = clarinet_socket_getopt(sp, optname, nullptr, &len);
                    REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                    REQUIRE(len == LEN_INIT);
                }

                SECTION("Set Option")
                {
                    errcode = clarinet_socket_setopt(sp, optname, nullptr, sizeof(val));
                    REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));

                    errcode = clarinet_socket_setopt(sp, optname, nullptr, 0);
                    REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                }
            }

            SECTION("With NULL optlen")
            {
                errcode = clarinet_socket_getopt(sp, optname, &val, nullptr);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(val == VAL_INIT);
            }

            SECTION("With INVALID optlen")
            {
                auto target = (size_t)GENERATE(range(0, 3));

                SECTION("Get option")
                {
                    len = (size_t)target;
                    errcode = clarinet_socket_getopt(sp, optname, &val, &len);
                    REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                    REQUIRE(val == VAL_INIT);
                    REQUIRE(len == (size_t)target);
                }

                SECTION("Set option")
                {
                    errcode = clarinet_socket_setopt(sp, optname, &val, target);
                    REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                    REQUIRE(val == VAL_INIT);
                }
            }
        }
    }

    SECTION("With VALID option arguments")
    {
        const int32_t on = 1;
        const int32_t off = 0;

        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));

        FROM(family);
        FROM(proto);

        SECTION("With UNOPEN socket")
        {
            clarinet_socket unopen_socket;
            clarinet_socket* usp = &unopen_socket;
            clarinet_socket_init(usp);

            clarinet_socket closed_socket;
            clarinet_socket* csp = &closed_socket;
            clarinet_socket_init(csp);

            int errcode = clarinet_socket_open(csp, family, proto);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            errcode = clarinet_socket_close(csp);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

            const char* state;
            clarinet_socket* sp;

            // @formatter:off
            std::tie(state, sp) = GENERATE_REF(table<const char*, clarinet_socket*>({
                { "UNOPEN", usp },
                { "CLOSED", csp },
            }));
            // @formatter:on

            FROM(state);

            #define TABLE_ITEM(s) { #s "(" CLARINET_XSTR(s) ")", (s) }
            const char* option;
            int optname;
            std::tie(option, optname) = GENERATE(table<const char*, int>({
                TABLE_ITEM(CLARINET_SO_NONBLOCK),
                TABLE_ITEM(CLARINET_SO_REUSEADDR),
                TABLE_ITEM(CLARINET_SO_SNDBUF),
                TABLE_ITEM(CLARINET_SO_RCVBUF),
                TABLE_ITEM(CLARINET_SO_SNDTIMEO),
                TABLE_ITEM(CLARINET_SO_RCVTIMEO),
                TABLE_ITEM(CLARINET_SO_KEEPALIVE),
                TABLE_ITEM(CLARINET_SO_LINGER),
                TABLE_ITEM(CLARINET_SO_DONTLINGER),
                TABLE_ITEM(CLARINET_IP_TTL),
                TABLE_ITEM(CLARINET_IP_V6ONLY),
                TABLE_ITEM(CLARINET_IP_MTU),
                TABLE_ITEM(CLARINET_IP_MTU_DISCOVER),
            }));
            #undef TABLE_ITEM
            FROM(option);

            SECTION("Get")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                errcode = clarinet_socket_getopt(sp, optname, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(val == VAL_INIT);
                REQUIRE(len == LEN_INIT);
            }

            SECTION("Set")
            {
                // It's ok to test read-only socket options here too because they'd return CALRINET_EINVAL anyway
                int32_t val = on;
                errcode = clarinet_socket_setopt(sp, optname, &val, sizeof(val));
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(val == on);
            }
        }

        SECTION("With OPEN socket")
        {
            clarinet_socket socket;
            clarinet_socket* sp = &socket;
            clarinet_socket_init(sp);

            int errcode = clarinet_socket_open(sp, family, proto);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            const auto onexit = finalizer([&sp]
            {
                clarinet_socket_close(sp);
            });

            SECTION("With optname CLARINET_SO_NONBLOCK")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                // All platforms are expected to open the socket in blocking mode but the option CLARINET_SO_NONBLOCK
                // is write-only so this cannot be confirmed here.
                errcode = clarinet_socket_getopt(sp, CLARINET_SO_NONBLOCK, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                REQUIRE(val == VAL_INIT);
                REQUIRE(len == LEN_INIT);

                errcode = clarinet_socket_setopt(sp, CLARINET_SO_NONBLOCK, &on, sizeof(on));
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                errcode = clarinet_socket_setopt(sp, CLARINET_SO_NONBLOCK, &off, sizeof(off));
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            }

            SECTION("With optname CLARINET_SO_REUSEADDR")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                // All platforms are expected to open the socket without address reuse
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

            SECTION("With optname CLARINET_SO_SNDBUF")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                // Every platform has a different default buffer size but all are > 0
                errcode = clarinet_socket_getopt(sp, CLARINET_SO_SNDBUF, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE(val > 0);
                REQUIRE(len == sizeof(val));

                // These are safe values supposed to be between the minimum and maximum supported by all platforms so a call
                // to clarinet_socket_getopt() will return the expected value.
                //
                // Minimum SO_SNDBUF is:
                //  - WINDOWS: 0 (effectively disables the buffer and leaves only the net driver queue)
                //  - LINUX (x64): 4608
                //  - BSD/DARWIN: 1
                //
                // Maximum SO_SNDBUF is:
                //  - WINDOWS: 2147483648 but in practice it is bounded by available memory
                //  - LINUX (x64): 212992 (defined by net.core.wmem_max)
                //  - BSD/DARWIN: 2097152 (defined by kern.ipc.maxsockbuf)
                auto target = (int32_t)GENERATE(8191, 8192, 16383, 16384);
                FROM(target);

                errcode = clarinet_socket_setopt(sp, CLARINET_SO_SNDBUF, &target, sizeof(target));
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                errcode = clarinet_socket_getopt(sp, CLARINET_SO_SNDBUF, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                #if defined(__linux__) // linux is special as we always round down odd buffer sizes to the nearest even number
                REQUIRE(val == (target & -2)); // target & 0XFFFFFFFE
                #else
                REQUIRE(val == target);
                #endif // defined(__linux__)
                REQUIRE(len == sizeof(val));
            }

            SECTION("With optname CLARINET_SO_RCVBUF")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                // Every platform has a different default buffer size but all are > 0
                errcode = clarinet_socket_getopt(sp, CLARINET_SO_RCVBUF, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE(val > 0);
                REQUIRE(len == sizeof(val));

                // These are safe values supposed to be between the minimum and maximum supported by all platforms so a call
                // to clarinet_socket_getopt() will return the expected value.
                //
                // Minimum SO_RCVBUF is:
                //  - WINDOWS: 0 (effectively disables the buffer and leaves only the net driver queue)
                //  - LINUX (x64): 2292
                //  - BSD/DARWIN: 1
                //
                // Maximum SO_RCVBUF is:
                //  - WINDOWS: 2147483648 but in practice it is bounded by available memory
                //  - LINUX (x64): 212992 (defined by net.core.wmem_max)
                //  - BSD/DARWIN: 2097152 (defined by kern.ipc.maxsockbuf)
                //
                auto target = (int32_t)GENERATE(8191, 8192, 16383, 16384);
                FROM(target);

                errcode = clarinet_socket_setopt(sp, CLARINET_SO_RCVBUF, &target, sizeof(target));
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                errcode = clarinet_socket_getopt(sp, CLARINET_SO_RCVBUF, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                #if defined(__linux__) // linux is special as we always round down odd buffer sizes to the nearest even number
                REQUIRE(val == (target & -2)); // target & 0XFFFFFFFE
                #else
                REQUIRE(val == target);
                #endif // defined(__linux__)
                REQUIRE(len == sizeof(val));
            }

            SECTION("With optname CLARINET_SO_SNDTIMEO")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                // All platforms are expected to have a default timeout of 0.
                errcode = clarinet_socket_getopt(sp, CLARINET_SO_SNDTIMEO, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE(val == 0);
                REQUIRE(len == sizeof(val));

                auto target = (int32_t)GENERATE(0, 10, 250, 500, 1000, 5000, 60000);
                FROM(target);

                errcode = clarinet_socket_setopt(sp, CLARINET_SO_SNDTIMEO, &target, sizeof(target));
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                errcode = clarinet_socket_getopt(sp, CLARINET_SO_SNDTIMEO, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE(val == target);
                REQUIRE(len == sizeof(val));
            }

            SECTION("With optname CLARINET_SO_RCVTIMEO")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                // All platforms are expected to have a default timeout of 0.
                errcode = clarinet_socket_getopt(sp, CLARINET_SO_RCVTIMEO, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE(val == 0);
                REQUIRE(len == sizeof(val));

                auto target = (int32_t)GENERATE(0, 10, 250, 500, 1000, 5000, 60000);
                FROM(target);

                errcode = clarinet_socket_setopt(sp, CLARINET_SO_RCVTIMEO, &target, sizeof(target));
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                errcode = clarinet_socket_getopt(sp, CLARINET_SO_RCVTIMEO, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE(val == target);
                REQUIRE(len == sizeof(val));
            }

            SECTION("With optname CLARINET_SO_KEEPALIVE")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                if (proto == CLARINET_PROTO_TCP)
                {
                    // All platforms are expected to open the socket keep alive off
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
                else
                {
                    errcode = clarinet_socket_getopt(sp, CLARINET_SO_KEEPALIVE, &val, &len);
                    REQUIRE(Error(errcode) == Error(CLARINET_EPROTONOSUPPORT));
                    REQUIRE(val == VAL_INIT);
                    REQUIRE(len == LEN_INIT);

                    errcode = clarinet_socket_setopt(sp, CLARINET_SO_KEEPALIVE, &val, sizeof(val));
                    REQUIRE(Error(errcode) == Error(CLARINET_EPROTONOSUPPORT));
                    REQUIRE(val == VAL_INIT);
                }
            }

            SECTION("With optname CLARINET_SO_LINGER")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                size_t lingerlen = sizeof(clarinet_linger);
                clarinet_linger LINGER_INIT;
                memset(&LINGER_INIT, 0xFF, lingerlen);
                clarinet_linger linger;
                memcpy(&linger, &LINGER_INIT, lingerlen);

                if (proto == CLARINET_PROTO_TCP)
                {
                    // All platforms are expected to open the socket without linger
                    errcode = clarinet_socket_getopt(sp, CLARINET_SO_LINGER, &linger, &lingerlen);
                    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                    REQUIRE(lingerlen == sizeof(clarinet_linger));
                    REQUIRE_FALSE(linger.enabled);

                    errcode = clarinet_socket_getopt(sp, CLARINET_SO_DONTLINGER, &val, &len);
                    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                    REQUIRE(val);
                    REQUIRE(len == sizeof(val));

                    auto target = (int32_t)GENERATE(0, 5, 10, 250, 500, 1000, 5000, 65535);
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
                else
                {
                    errcode = clarinet_socket_getopt(sp, CLARINET_SO_LINGER, &linger, &lingerlen);
                    REQUIRE(Error(errcode) == Error(CLARINET_EPROTONOSUPPORT));
                    REQUIRE(lingerlen == sizeof(clarinet_linger));
                    const bool linger_is_unmodified = (memcmp(&linger, &LINGER_INIT, sizeof(clarinet_linger)) == 0);
                    REQUIRE(linger_is_unmodified);

                    auto target = (int32_t)GENERATE(0, 5, 10, 250, 500, 1000, 5000, 65535);
                    FROM(target);

                    linger.enabled = true;
                    linger.seconds = (uint16_t)target;
                    errcode = clarinet_socket_setopt(sp, CLARINET_SO_LINGER, &linger, sizeof(linger));
                    REQUIRE(Error(errcode) == Error(CLARINET_EPROTONOSUPPORT));
                    REQUIRE(linger.enabled);
                    REQUIRE(linger.seconds == target);
                }
            }

            SECTION("With optname CLARINET_SO_DONTLINGER")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                size_t lingerlen = sizeof(clarinet_linger);
                clarinet_linger LINGER_INIT;
                memset(&LINGER_INIT, 0xFF, lingerlen);
                clarinet_linger linger;
                memcpy(&linger, &LINGER_INIT, lingerlen);

                if (proto == CLARINET_PROTO_TCP)
                {

                    // All platforms are expected to open the socket without linger this means ON
                    errcode = clarinet_socket_getopt(sp, CLARINET_SO_DONTLINGER, &val, &len);
                    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                    REQUIRE(val);
                    REQUIRE(len == sizeof(val));

                    // All platforms are expected to open the socket without linger
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

                    seconds = 5; // arbitrary non-zero value - we just want to check that CLARINET_SO_DONTLINGER does not affect the timeout previously set here
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
                else
                {
                    errcode = clarinet_socket_getopt(sp, CLARINET_SO_DONTLINGER, &val, &len);
                    REQUIRE(Error(errcode) == Error(CLARINET_EPROTONOSUPPORT));
                    REQUIRE(val == VAL_INIT);
                    REQUIRE(len == LEN_INIT);

                    errcode = clarinet_socket_setopt(sp, CLARINET_SO_DONTLINGER, &val, sizeof(val));
                    REQUIRE(Error(errcode) == Error(CLARINET_EPROTONOSUPPORT));
                    REQUIRE(val == VAL_INIT);
                }
            }

            SECTION("With optname CLARINET_IP_V6ONLY")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                #if CLARINET_ENABLE_IPV6

                errcode = clarinet_socket_getopt(sp, CLARINET_IP_V6ONLY, &val, &len);
                if (sp->family == CLARINET_AF_INET6)
                {
                    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                    // Each platform has a specific default initial values assuming unmodified system configurations.
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

            SECTION("With optname CLARINET_IP_TTL")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                // Every platform has a different default TTL but all are > 0
                errcode = clarinet_socket_getopt(sp, CLARINET_IP_TTL, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE(val > 0);
                REQUIRE(len == sizeof(val));

                // Valid values are in the interval [1, 255]
                auto target = (int32_t)GENERATE(1, 2, 4, 8, 16, 32, 64, 128, 255);
                FROM(target);

                errcode = clarinet_socket_setopt(sp, CLARINET_IP_TTL, &target, sizeof(target));
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                errcode = clarinet_socket_getopt(sp, CLARINET_IP_TTL, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE(val == target);
                REQUIRE(len == sizeof(val));
            }

            SECTION("With optname CLARINET_IP_MTU")
            {
                #define CLARINET_IP_MTU_TEST_VALUES 0, 32, 576, 1280, 1500, 4096

                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                SECTION("BEFORE Connect")
                {
                    SECTION("Get option")
                    {
                        // MTU cannot be retrieved until the scoket is connected
                        errcode = clarinet_socket_getopt(sp, CLARINET_IP_MTU, &val, &len);
                        REQUIRE(Error(errcode) == Error(CLARINET_ENOTCONN));
                    }

                    SECTION("Set option")
                    {
                        // All attempts to set CLARINET_IP_MTU should fail since it is a read-only option
                        auto target = (int32_t)GENERATE(CLARINET_IP_MTU_TEST_VALUES);
                        FROM(target);

                        errcode = clarinet_socket_setopt(sp, CLARINET_IP_MTU, &target, sizeof(target));
                        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                    }
                }

                SECTION("AFTER Connect")
                {
                    clarinet_socket server;
                    clarinet_socket* ssp = &server;
                    clarinet_socket_init(ssp);

                    errcode = clarinet_socket_open(ssp, family, proto);
                    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                    const auto onserverexit = finalizer([&ssp]
                    {
                        clarinet_socket_close(ssp);
                    });

                    clarinet_endpoint endpoint;
                    clarinet_addr addr = { 0 };
                    switch (family)
                    {
                        case CLARINET_AF_INET:
                            endpoint = clarinet_make_endpoint(clarinet_addr_any_ipv4, 0);
                            addr = clarinet_addr_loopback_ipv4;
                            break;
                        case CLARINET_AF_INET6:
                            endpoint = clarinet_make_endpoint(clarinet_addr_any_ipv6, 0);
                            addr = clarinet_addr_loopback_ipv6;
                            break;
                        default:
                            FAIL(); // unexpected family
                    }

                    errcode = clarinet_socket_bind(ssp, &endpoint);
                    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                    errcode = clarinet_socket_local_endpoint(ssp, &endpoint);
                    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                    endpoint.addr = addr;

                    // If the server protocol supports listen then make it listen
                    if (proto & CLARINET_TEST_SOCKET_LISTEN_PROTO_BSET)
                    {
                        errcode = clarinet_socket_listen(ssp, 1);
                        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                    }

                    if (proto & CLARINET_TEST_SOCKET_CONNECT_PROTO_BSET)
                    {
                        errcode = clarinet_socket_connect(sp, &endpoint);
                        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                    }

                    SECTION("Get option")
                    {
                        errcode = clarinet_socket_getopt(sp, CLARINET_IP_MTU, &val, &len);
                        // CLARINET_IP_MTU requires the socket to be connected.
                        if (proto & CLARINET_TEST_SOCKET_CONNECT_PROTO_BSET)
                        {
                            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                            // According to RFC791: "Every internet module must be able to forward a datagram of 68
                            // octets without further fragmentation. This is because an internet header may be up to
                            // 60 octets, and the minimum fragment is 8 octets."
                            REQUIRE(val > 68);
                            REQUIRE(len == sizeof(val));
                        }
                        else
                        {
                            REQUIRE(Error(errcode) == Error(CLARINET_ENOTCONN));
                        }
                    }

                    SECTION("Set option")
                    {
                        // All attempts to set CLARINET_IP_MTU should fail since it is a read-only option
                        auto target = (int32_t)GENERATE(CLARINET_IP_MTU_TEST_VALUES);
                        FROM(target);

                        errcode = clarinet_socket_setopt(sp, CLARINET_IP_MTU, &target, sizeof(target));
                        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
                    }
                }

                #undef CLARINET_IP_MTU_TEST_VALUES
            }

            SECTION("With optname CLARINET_IP_MTU_DISCOVER")
            {
                int32_t val = VAL_INIT;
                size_t len = LEN_INIT;

                // All platforms are expected to open the socket with CLARINET_PMTUD_UNSPEC mode
                errcode = clarinet_socket_getopt(sp, CLARINET_IP_MTU_DISCOVER, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE(val == CLARINET_PMTUD_UNSPEC);
                REQUIRE(len == sizeof(val));

                #define TABLE_ITEM(s) { #s, (s) }
                const char* optval;
                int32_t target;
                std::tie(optval, target) = GENERATE(table<const char*, int32_t>({
                    TABLE_ITEM(CLARINET_PMTUD_UNSPEC),
                    TABLE_ITEM(CLARINET_PMTUD_ON),
                    TABLE_ITEM(CLARINET_PMTUD_OFF),
                    TABLE_ITEM(CLARINET_PMTUD_PROBE),
                }));
                #undef TABLE_ITEM

                FROM(optval);

                errcode = clarinet_socket_setopt(sp, CLARINET_IP_MTU_DISCOVER, &target, sizeof(target));
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                errcode = clarinet_socket_getopt(sp, CLARINET_IP_MTU_DISCOVER, &val, &len);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
                REQUIRE(val == target);
                REQUIRE(len == sizeof(val));
            }
        }
    }
}

TEST_CASE("Socket Bind")
{
    SECTION("With NULL socket")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
            #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
            #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        int errcode = clarinet_socket_bind(nullptr, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNOPEN socket")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
            #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
            #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_bind(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With NULL endpoint")
    {
        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));

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

        errcode = clarinet_socket_bind(sp, nullptr);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With INVALID endpoint")
    {
        clarinet_endpoint endpoint = clarinet_make_endpoint(clarinet_make_mac(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF), 0);

        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));

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

        errcode = clarinet_socket_bind(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_EAFNOSUPPORT));
    }

    SECTION("With no conflicts")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
        #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
        #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        SECTION("Bind BEFORE Open should not affect the socket")
        {
            clarinet_proto proto = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
            }));

            FROM(proto);

            clarinet_socket socket;
            clarinet_socket* sp = &socket;
            clarinet_socket_init(sp);

            int errcode = clarinet_socket_bind(sp, &endpoint);
            REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));

            errcode = clarinet_socket_open(sp, endpoint.addr.family, proto);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            const auto onexit = finalizer([&sp]
            {
                clarinet_socket_close(sp);
            });
        }

        SECTION("Bind AFTER Open")
        {
            clarinet_proto proto = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
            }));

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

            SECTION("Bind ONCE")
            {
                errcode = clarinet_socket_bind(sp, &endpoint);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            }

            SECTION("Bind TWICE")
            {
                errcode = clarinet_socket_bind(sp, &endpoint);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

                errcode = clarinet_socket_bind(sp, &endpoint);
                REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
            }

            SECTION("Bind ONCE with dual stack (CLARINET_IP_V6ONLY off)")
            {
                #if CLARINET_ENABLE_IPV6DUAL
                const int expected = (endpoint.addr.family == CLARINET_AF_INET6) ? CLARINET_ENONE : CLARINET_EINVAL;
                #else
                const int expected = CLARINET_EINVAL;
                #endif // CLARINET_ENABLE_IPV6DUAL

                const int32_t off = 1;
                errcode = clarinet_socket_setopt(sp, CLARINET_IP_V6ONLY, &off, sizeof(off));
                REQUIRE(Error(errcode) == Error(expected));

                errcode = clarinet_socket_bind(sp, &endpoint);
                REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            }
        }

        SECTION("Bind TWO sockets with SAME address and port but DIFFERENT protocols")
        {
            clarinet_socket sa;
            clarinet_socket* spa = &sa;
            clarinet_socket_init(spa);

            // Any two protocols should do here. There is no need to test all combinations.
            clarinet_proto proto_a = GENERATE(CLARINET_PROTO_UDP);
            clarinet_proto proto_b = GENERATE(CLARINET_PROTO_TCP);

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

        SECTION("Bind TWO sockets with SAME address and protocol but DIFFERENT ports")
        {
            clarinet_proto proto = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
            }));

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

            const int32_t off = 1;
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
    }

    SECTION("With ALL address combinations (including conflicts)")
    {
        const int NONE = 0;
        const int REUSEADDR = (1 << 0);
        const int IPV6DUAL = (1 << 1);

        // @formatter:off
        // Table: sample, first socket address | first socket flags | second socket address | second socket flags | expected result
        const std::vector<std::tuple<int, clarinet_addr, int, clarinet_addr, int, int>> samples = {
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

        SAMPLES(samples);

        int sample;
        clarinet_addr addra;
        int fa;
        clarinet_addr addrb;
        int fb;
        int expected;
        std::tie(sample, addra, fa, addrb, fb, expected) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        SECTION("Bind TWO sockets with SAME port and protocol")
        {
            clarinet_proto proto = GENERATE(values({
                CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
            }));

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
}

TEST_CASE("Socket Listen")
{
    SECTION("With NULL socket")
    {
        int errcode = clarinet_socket_listen(nullptr, 1);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNOPEN socket")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_listen(sp, 1);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNBOUND socket")
    {
        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));
        FROM(family);

        // All protocols compatible with listen can be opened
        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_LISTEN_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        // Cover the minimum supported backlog values
        int backlog = GENERATE(0, 1, 2, 3, 4, 5);
        FROM(backlog);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        errcode = clarinet_socket_listen(sp, backlog);
        #if defined(_WIN32)
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        #else
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        #endif
    }

    SECTION("With UNSUPPORTED protocols")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
#if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
#endif
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        // All protocols compatible with listen can be opened but not all protocols that can be opened are compatible
        //with listen, so we must filter from the protocols that can be opened which ones are not compatible
        clarinet_proto proto = GENERATE(filter([](int v)
            {
                return !(v & CLARINET_TEST_SOCKET_LISTEN_PROTO_BSET);
            },
            values({ CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST })));
        FROM(proto);

        // Cover the minimum supported backlog values but shouldn't be any different to any other value really
        int backlog = GENERATE(0, 1, 2, 3, 4, 5);
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

    SECTION("With SUPPORTED protocols")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
#if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
#endif
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        // All protocols compatible with listen can be opened
        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_LISTEN_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        // Cover the minimum supported backlog values
        int backlog = GENERATE(0, 1, 2, 3, 4, 5);
        FROM(backlog);

        int calls = GENERATE(1, 2, 3, 4);

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

TEST_CASE("Socket Connect")
{
    SECTION("With NULL socket")
    {
        clarinet_endpoint remote = clarinet_make_endpoint(clarinet_addr_loopback_ipv4, 1313);

        int errcode = clarinet_socket_connect(nullptr, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNOPEN socket")
    {
        clarinet_endpoint endpoint = clarinet_make_endpoint(clarinet_addr_loopback_ipv4, 1313);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_connect(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With NULL endpoint")
    {
        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_CONNECT_SUPPORTED_PROTO_LIST
        }));

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

        errcode = clarinet_socket_connect(sp, nullptr);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With INVALID endpoint")
    {
        clarinet_endpoint endpoint = clarinet_make_endpoint(clarinet_make_mac(0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF), 0);

        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_CONNECT_SUPPORTED_PROTO_LIST
        }));

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

        errcode = clarinet_socket_connect(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_EAFNOSUPPORT));
    }

    SECTION("With UNSUPPORTED protocols")
    {
        // For now, all protocols supported by clarinet_socket_open() are also supported by clarinet_socket_connect()
        //and vice-versa. We'll have something to test here only if they differ
        REQUIRE(Hex(CLARINET_TEST_SOCKET_CONNECT_PROTO_BSET) == Hex(CLARINET_TEST_SOCKET_OPEN_PROTO_BSET));
    }

    SECTION("With SUPPORTED protocols")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint, clarinet_addr>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 }, clarinet_addr_loopback_ipv4 },
            { 1, { clarinet_addr_loopback_ipv4, 0 }, clarinet_addr_loopback_ipv4 },
            #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 }, clarinet_addr_loopback_ipv6 },
            { 3, { clarinet_addr_loopback_ipv6, 0 }, clarinet_addr_loopback_ipv6 },
            #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        clarinet_addr addr;
        std::tie(sample, endpoint, addr) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_CONNECT_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        int errcode = clarinet_socket_open(ssp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onserverexit = finalizer([&ssp]
        {
            clarinet_socket_close(ssp);
        });

        errcode = clarinet_socket_bind(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_local_endpoint(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        endpoint.addr = addr;

        // if the server protocol supports listen then make it listen
        if (proto & CLARINET_TEST_SOCKET_LISTEN_PROTO_BSET)
        {
            errcode = clarinet_socket_listen(ssp, 1);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        }

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        errcode = clarinet_socket_open(sp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        errcode = clarinet_socket_connect(sp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    }
}

TEST_CASE("Socket Accept")
{
    SECTION("NULL server socket")
    {
        clarinet_socket client;
        clarinet_socket* csp = &client;
        clarinet_socket_init(csp);

        clarinet_endpoint remote;

        int errcode = clarinet_socket_accept(nullptr, csp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("NULL client socket")
    {
        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        clarinet_endpoint remote;

        int errcode = clarinet_socket_accept(ssp, nullptr, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("NULL endpoint")
    {
        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        clarinet_socket accepted;
        clarinet_socket* asp = &accepted;
        clarinet_socket_init(asp);

        int errcode = clarinet_socket_accept(ssp, asp, nullptr);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNOPEN server")
    {
        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        clarinet_socket accepted;
        clarinet_socket* asp = &accepted;
        clarinet_socket_init(asp);

        clarinet_endpoint remote;

        int errcode = clarinet_socket_accept(ssp, asp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNBOUND server")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint, clarinet_addr>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 }, clarinet_addr_loopback_ipv4 },
            { 1, { clarinet_addr_loopback_ipv4, 0 }, clarinet_addr_loopback_ipv4 },
            #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 }, clarinet_addr_loopback_ipv6 },
            { 3, { clarinet_addr_loopback_ipv6, 0 }, clarinet_addr_loopback_ipv6 },
            #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        clarinet_addr addr;
        std::tie(sample, endpoint, addr) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_ACCEPT_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        clarinet_socket accepted;
        clarinet_socket* asp = &accepted;
        clarinet_socket_init(asp);

        clarinet_endpoint remote;

        int errcode = clarinet_socket_open(ssp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&ssp]
        {
            clarinet_socket_close(ssp);
        });

        errcode = clarinet_socket_accept(ssp, asp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNLISTENING server")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint, clarinet_addr>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 }, clarinet_addr_loopback_ipv4 },
            { 1, { clarinet_addr_loopback_ipv4, 0 }, clarinet_addr_loopback_ipv4 },
            #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 }, clarinet_addr_loopback_ipv6 },
            { 3, { clarinet_addr_loopback_ipv6, 0 }, clarinet_addr_loopback_ipv6 },
            #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        clarinet_addr addr;
        std::tie(sample, endpoint, addr) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_ACCEPT_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        clarinet_socket accepted;
        clarinet_socket* asp = &accepted;
        clarinet_socket_init(asp);

        clarinet_endpoint remote;

        int errcode = clarinet_socket_open(ssp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&ssp]
        {
            clarinet_socket_close(ssp);
        });

        errcode = clarinet_socket_bind(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_accept(ssp, asp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With LISTENING BLOCKING server")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint, clarinet_addr>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 }, clarinet_addr_loopback_ipv4 },
            { 1, { clarinet_addr_loopback_ipv4, 0 }, clarinet_addr_loopback_ipv4 },
            #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 }, clarinet_addr_loopback_ipv6 },
            { 3, { clarinet_addr_loopback_ipv6, 0 }, clarinet_addr_loopback_ipv6 },
            #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        clarinet_addr addr;
        std::tie(sample, endpoint, addr) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_ACCEPT_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        clarinet_socket accepted;
        clarinet_socket* asp = &accepted;
        clarinet_socket_init(asp);

        clarinet_endpoint remote;

        int errcode = clarinet_socket_open(ssp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onserverexit = finalizer([&ssp]
        {
            clarinet_socket_close(ssp);
        });

        errcode = clarinet_socket_bind(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_local_endpoint(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_listen(ssp, 1);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_socket client;
        clarinet_socket* csp = &client;
        clarinet_socket_init(csp);

        remote = clarinet_make_endpoint(addr, endpoint.port);
        errcode = clarinet_socket_open(csp, addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onclientexit = finalizer([&csp]
        {
            clarinet_socket_close(csp);
        });

        // A client must be trying to connect for accept to not block indefinitely
        errcode = clarinet_socket_connect(csp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        // Trying to accept over the socket that is connecting should invalid
        errcode = clarinet_socket_accept(ssp, csp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));

        // Another socket used to check that we cannot accidentaly overwrite a previously open socket
        clarinet_socket other;
        clarinet_socket* osp = &other;
        clarinet_socket_init(osp);
        errcode = clarinet_socket_open(osp, remote.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onotherexit = finalizer([&osp]
        {
            clarinet_socket_close(osp);
        });

        // Trying to accept over a socket that is already open should be invalid
        errcode = clarinet_socket_accept(ssp, osp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));

        // Trying to accept over a socket that is just initialized is ok
        memset(&remote, 0, sizeof(remote));
        errcode = clarinet_socket_accept(ssp, asp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onacceptedexit = finalizer([&asp]
        {
            clarinet_socket_close(asp);
        });
        REQUIRE(asp->family == ssp->family);
        // Although technically a possibility, there is currently no supported platform that might return an invalid
        //remote address from a successful call to accept
        REQUIRE(remote.addr.family == asp->family);
    }
}

TEST_CASE("Socket Get Local Endpoint")
{
    SECTION("With NULL socket")
    {
        clarinet_endpoint local;

        int errcode = clarinet_socket_local_endpoint(nullptr, &local);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With NULL endpoint")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_local_endpoint(sp, nullptr);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNOPEN socket")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        clarinet_endpoint local;

        int errcode = clarinet_socket_local_endpoint(sp, &local);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNBOUND socket")
    {
        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));

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

        // There is no local endpoint if the  socket is not bound to a local address
        clarinet_endpoint local = { { 0 } };
        errcode = clarinet_socket_local_endpoint(sp, &local);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("AFTER Bind")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 } },
            { 1, { clarinet_addr_loopback_ipv4, 0 } },
        #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 } },
            { 3, { clarinet_addr_loopback_ipv6, 0 } },
        #endif
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        std::tie(sample, endpoint) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));
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

        clarinet_endpoint local = { { 0 } };
        errcode = clarinet_socket_local_endpoint(sp, &local);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        // After a successful bind the local endpoint address should be defined although the port may still vary
        int addr_is_equal = clarinet_addr_is_equal(&local.addr, &endpoint.addr);
        REQUIRE(addr_is_equal);
        if (endpoint.port == 0)
            REQUIRE(local.port > 0);
        else
            REQUIRE(local.port == endpoint.port);
    }

    SECTION("AFTER Connect")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint, clarinet_addr>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 }, clarinet_addr_loopback_ipv4 },
            { 1, { clarinet_addr_loopback_ipv4, 0 }, clarinet_addr_loopback_ipv4 },
            #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 }, clarinet_addr_loopback_ipv6 },
            { 3, { clarinet_addr_loopback_ipv6, 0 }, clarinet_addr_loopback_ipv6 },
            #endif
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        clarinet_addr addr;
        std::tie(sample, endpoint, addr) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_CONNECT_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        int errcode = clarinet_socket_open(ssp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onserverexit = finalizer([&ssp]
        {
            clarinet_socket_close(ssp);
        });

        errcode = clarinet_socket_bind(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_local_endpoint(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        // If the server protocol supports listen then make it listen
        if (proto & CLARINET_TEST_SOCKET_LISTEN_PROTO_BSET)
        {
            errcode = clarinet_socket_listen(ssp, 1);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        }

        endpoint.addr = addr;

        clarinet_socket client;
        clarinet_socket* csp = &client;
        clarinet_socket_init(csp);

        errcode = clarinet_socket_open(csp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onclientexit = finalizer([&csp]
        {
            clarinet_socket_close(csp);
        });

        errcode = clarinet_socket_connect(csp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        memset(&endpoint, 0, sizeof(endpoint));
        errcode = clarinet_socket_local_endpoint(csp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        REQUIRE(endpoint.port > 0);

        const bool addr_is_ip = clarinet_addr_is_ipv4(&endpoint.addr) || clarinet_addr_is_ipv6(&endpoint.addr);
        REQUIRE(addr_is_ip);

        const bool addr_is_any_ip = clarinet_addr_is_any_ip(&endpoint.addr);
        REQUIRE_FALSE(addr_is_any_ip);
    }
}

TEST_CASE("Socket Get Remote Endpoint")
{
    SECTION("With NULL socket")
    {
        clarinet_endpoint remote;

        int errcode = clarinet_socket_remote_endpoint(nullptr, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With NULL endpoint")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_remote_endpoint(sp, nullptr);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNOPEN socket")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        clarinet_endpoint remote;

        int errcode = clarinet_socket_remote_endpoint(sp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNCONNECTED socket")
    {
        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));

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

        clarinet_endpoint remote;
        errcode = clarinet_socket_remote_endpoint(sp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_ENOTCONN));
    }

    SECTION("With CONNECTED socket")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint, clarinet_addr>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 }, clarinet_addr_loopback_ipv4 },
            { 1, { clarinet_addr_loopback_ipv4, 0 }, clarinet_addr_loopback_ipv4 },
            #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 }, clarinet_addr_loopback_ipv6 },
            { 3, { clarinet_addr_loopback_ipv6, 0 }, clarinet_addr_loopback_ipv6 },
            #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        clarinet_addr addr;
        std::tie(sample, endpoint, addr) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        // We're only interested here in protocols that can CONNECT but do not LISTEN/ACCEPT
        clarinet_proto proto = GENERATE(filter([](int v)
            {
                return !(v & CLARINET_TEST_SOCKET_ACCEPT_PROTO_BSET);
            },
            values({ CLARINET_TEST_SOCKET_CONNECT_SUPPORTED_PROTO_LIST })));
        FROM(proto);

        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        clarinet_socket accepted;
        clarinet_socket* asp = &accepted;
        clarinet_socket_init(asp);

        int errcode = clarinet_socket_open(ssp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onserverexit = finalizer([&ssp]
        {
            clarinet_socket_close(ssp);
        });

        errcode = clarinet_socket_bind(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_local_endpoint(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_socket client;
        clarinet_socket* csp = &client;
        clarinet_socket_init(csp);

        errcode = clarinet_socket_open(csp, addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onclientexit = finalizer([&csp]
        {
            clarinet_socket_close(csp);
        });

        const clarinet_endpoint client_remote = clarinet_make_endpoint(addr, endpoint.port);
        errcode = clarinet_socket_connect(csp, &client_remote);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_endpoint remote;

        // The server socket is not connected so the call is invalid
        errcode = clarinet_socket_remote_endpoint(ssp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_ENOTCONN));

        // The remote endpoint of the connected socket should match the remote endpoint used in the call to connect.
        errcode = clarinet_socket_remote_endpoint(csp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const bool endpoint_is_equal = clarinet_endpoint_is_equal(&remote, &client_remote);
        REQUIRE(endpoint_is_equal);
    }

    SECTION("With CONNECTED and ACCEPTED sockets")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint, clarinet_addr>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 }, clarinet_addr_loopback_ipv4 },
            { 1, { clarinet_addr_loopback_ipv4, 0 }, clarinet_addr_loopback_ipv4 },
            #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 }, clarinet_addr_loopback_ipv6 },
            { 3, { clarinet_addr_loopback_ipv6, 0 }, clarinet_addr_loopback_ipv6 },
            #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        clarinet_addr addr;
        std::tie(sample, endpoint, addr) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_ACCEPT_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        clarinet_socket accepted;
        clarinet_socket* asp = &accepted;
        clarinet_socket_init(asp);

        int errcode = clarinet_socket_open(ssp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onserverexit = finalizer([&ssp]
        {
            clarinet_socket_close(ssp);
        });

        errcode = clarinet_socket_bind(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_local_endpoint(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_listen(ssp, 1);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_socket client;
        clarinet_socket* csp = &client;
        clarinet_socket_init(csp);

        errcode = clarinet_socket_open(csp, addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onclientexit = finalizer([&csp]
        {
            clarinet_socket_close(csp);
        });

        const clarinet_endpoint client_remote = clarinet_make_endpoint(addr, endpoint.port);
        errcode = clarinet_socket_connect(csp, &client_remote);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_endpoint accepted_remote;
        errcode = clarinet_socket_accept(ssp, asp, &accepted_remote);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onacceptedexit = finalizer([&asp]
        {
            clarinet_socket_close(asp);
        });

        clarinet_endpoint remote;

        // A listening socket is not connected so the call is invalid
        errcode = clarinet_socket_remote_endpoint(ssp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_ENOTCONN));

        // The remote endpoint of the accepted socket should match the one retrieved in the call to accept
        errcode = clarinet_socket_remote_endpoint(asp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        bool endpoint_is_equal = clarinet_endpoint_is_equal(&remote, &accepted_remote);
        REQUIRE(endpoint_is_equal);

        // The remote endpoint of the accepted socket should also match the local endpoint of the client in this test
        // ALTHOUGH this is not generally guaranteed for any TWO peers over the network because of NAT, proxies etc.
        clarinet_endpoint client_local;
        errcode = clarinet_socket_local_endpoint(csp, &client_local);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        endpoint_is_equal = clarinet_endpoint_is_equal(&remote, &client_local);
        REQUIRE(endpoint_is_equal);

        // The remote endpoint of the connected socket should match the remote endpoint used in the call to connect.
        errcode = clarinet_socket_remote_endpoint(csp, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        endpoint_is_equal = clarinet_endpoint_is_equal(&remote, &client_remote);
        REQUIRE(endpoint_is_equal);
    }
}

TEST_CASE("Socket Send")
{

}

static
void
TEST_UDP_SENDTO(const clarinet_endpoint* local,
                const clarinet_endpoint* remote,
                const int32_t send_buffer_size,
                int send_length,
                int count,
                int expected)
{
    // Sanity: avoid obvious invalid test params
    REQUIRE(count > 0);
    REQUIRE(count <= INT_MAX);
    REQUIRE(expected <= (int)send_length);

    clarinet_socket socket;
    clarinet_socket* sp = &socket;
    clarinet_socket_init(sp);

    int errcode = clarinet_socket_open(sp, local->addr.family, CLARINET_PROTO_UDP);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    const auto onexit = finalizer([&sp]
    {
        clarinet_socket_close(sp);
    });

    // Skip setting the socket buffer size if value is negative
    if (send_buffer_size >= 0)
    {
        errcode = clarinet_socket_setopt(sp, CLARINET_SO_SNDBUF, &send_buffer_size, sizeof(send_buffer_size));
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    }

    const int32_t ttl = 4;
    errcode = clarinet_socket_setopt(sp, CLARINET_IP_TTL, &ttl, sizeof(ttl));
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

    errcode = clarinet_socket_bind(sp, local);
    REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

    void* send_buf = malloc((size_t)send_length);
    REQUIRE(send_buf != nullptr);
    const auto send_buf_dtor = finalizer([&]
    {
        free(send_buf);
    });

    for (int packet = 0; packet < count; ++packet)
    {
        FROM(packet);

        const int n = clarinet_socket_sendto(sp, send_buf, (size_t)send_length, remote);
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

TEST_CASE("Socket Send To")
{
    CLARINET_TEST_CASE_LIMITED_ON_WSL();

    SECTION("With NULL socket")
    {
        const uint8_t buf[] = { 0xAA, 0xBB, 0xCC, 0XDD, 0xEE, 0xFF };
        const clarinet_endpoint remote = clarinet_make_endpoint(clarinet_addr_loopback_ipv4, 9);

        int errcode = clarinet_socket_sendto(nullptr, buf, sizeof(buf), &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNOPEN socket")
    {
        const uint8_t buf[] = { 0xAA, 0xBB, 0xCC, 0XDD, 0xEE, 0xFF };
        const clarinet_endpoint remote = clarinet_make_endpoint(clarinet_addr_loopback_ipv4, 9);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_sendto(sp, buf, sizeof(buf), &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With NULL buffer")
    {
        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));
        FROM(family);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_endpoint remote;
        switch (family)
        {
            case CLARINET_AF_INET:
                remote = clarinet_make_endpoint(clarinet_addr_loopback_ipv4, 9);
                break;
            case CLARINET_AF_INET6:
                remote = clarinet_make_endpoint(clarinet_addr_loopback_ipv6, 9);
                break;
            default:
                FAIL();
        }

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        // A NULL buffer is when buflen=0 so skip 0 here
        int target = GENERATE(range(1, 6));
        errcode = clarinet_socket_sendto(sp, nullptr, (size_t)target, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With INVALID buffer length")
    {
        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));
        FROM(family);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_endpoint remote;
        switch (family)
        {
            case CLARINET_AF_INET:
                remote = clarinet_make_endpoint(clarinet_addr_loopback_ipv4, 9);
                break;
            case CLARINET_AF_INET6:
                remote = clarinet_make_endpoint(clarinet_addr_loopback_ipv6, 9);
                break;
            default:
                FAIL();
        }

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        const uint8_t buf[] = { 0xAA, 0xBB, 0xCC, 0XDD, 0xEE, 0xFF };

        errcode = clarinet_socket_sendto(sp, buf, (size_t)-1, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));

        #if (SIZE_MAX > INT_MAX)
        errcode = clarinet_socket_sendto(sp, buf, (size_t)INT_MAX + 1, &remote);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
        #endif
    }

    SECTION("With NULL endpoint")
    {
        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));
        FROM(family);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        const uint8_t buf[] = { 0xAA, 0xBB, 0xCC, 0XDD, 0xEE, 0xFF };

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        errcode = clarinet_socket_sendto(sp, buf, sizeof(buf), nullptr);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNBOUND socket")
    {
        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));
        FROM(family);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_endpoint remote;
        switch (family)
        {
            case CLARINET_AF_INET:
                remote = clarinet_make_endpoint(clarinet_addr_loopback_ipv4, 9);
                break;
            case CLARINET_AF_INET6:
                remote = clarinet_make_endpoint(clarinet_addr_loopback_ipv6, 9);
                break;
            default:
                FAIL();
        }

        const uint8_t buf[] = { 0xAA, 0xBB, 0xCC, 0XDD, 0xEE, 0xFF };

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onexit = finalizer([&sp]
        {
            clarinet_socket_close(sp);
        });

        errcode = clarinet_socket_sendto(sp, buf, sizeof(buf), &remote);
        if (proto == CLARINET_PROTO_UDP)
        {
            // An unbound UDP socket is implicitly bound to the default bind address which should be a wildcard
            REQUIRE(Error(errcode) == sizeof(buf));

            clarinet_endpoint local;
            errcode = clarinet_socket_local_endpoint(sp, &local);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            const bool addr_is_any_ip = clarinet_addr_is_any_ip(&local.addr);
            REQUIRE(addr_is_any_ip);
            REQUIRE(local.port > 0);
        }
        else
        {
            // An unbound TCP socket always fails with CLARINET_ENOTCONN
            REQUIRE(Error(errcode) == Error(CLARINET_ENOTCONN));
        }
    }

    SECTION("With UDP socket")
    {
        // This is the common ground test. All the samples should work in all platforms. We only send up to 8192 bytes
        // because Darwin (macOS) by default limits datagrams to 9126 bytes.

        // @formatter:off
        // Table: index | local | remote
        const std::vector<std::tuple<int, clarinet_endpoint, clarinet_endpoint>> recipients = {
            { 0, { clarinet_addr_any_ipv4, 0 }, clarinet_make_endpoint(clarinet_make_ipv4(8, 8, 8, 8), 9) }, // google DNS ipv4 with a discard service port
            #if CLARINET_ENABLE_IPV6 && !defined(__wsl__) // WSL cannot interact with external IPV6 only addresses due to Hyper-V limitations
            { 1, { clarinet_addr_any_ipv6, 0 }, clarinet_make_endpoint(clarinet_make_ipv6(0x2001, 0x4860, 0x4860, 0, 0, 0, 0, 0x8888, 0), 9) }, // google DNS ipv6 with a discard service port
            #endif
        };

        // Passing a send-buffer size of -1 here skips setting the buffer size and falls back to using the system
        // default in each platform.

        // Table: index | send buffer size | send length
        const std::vector<std::tuple<int, int, int>> samples = {
            {  0,    -1,     0 },
            {  1,    -1,     1 },
            {  2,    -1,     2 },
            {  3,    -1,     4 },
            {  4,    -1,     8 },
            {  5,    -1,    16 },
            {  6,    -1,    32 },
            {  7,    -1,    64 },
            {  8,    -1,   128 },
            {  9,    -1,   256 },
            { 10,    -1,   512 },
            { 11,    -1,  1024 },
            { 12,    -1,  2048 },
            { 13,    -1,  4096 },
            { 14,    -1,  8192 },

            { 15,   768,     0 },
            { 16,   768,     1 },
            { 17,   768,     2 },
            { 18,   768,     4 },
            { 19,   768,     8 },
            { 20,   768,    16 },
            { 21,   768,    32 },
            { 22,   768,    64 },
            { 23,  1280,   128 },
            { 24,  1280,   256 },
            { 25,  1280,   512 },
            { 26,  2304,  1024 },
            { 27,  4352,  2048 },
            { 28,  8448,  4096 },
            { 29, 16640,  8192 },
        };
        // @formatter:on

        SAMPLES(recipients);
        SAMPLES(samples);

        int sample;
        int send_buffer_size;
        int send_length;

        std::tie(sample, send_buffer_size, send_length) = GENERATE_REF(from_samples(samples));

        FROM(sample);

        REQUIRE(send_length <= 8192);

        int recipient;
        clarinet_endpoint local, remote;
        std::tie(recipient, local, remote) = GENERATE_REF(from_samples(recipients));
        FROM(recipient);

        TEST_UDP_SENDTO(&local, &remote, send_buffer_size, send_length, 1, CLARINET_ENONE);
    }

    SECTION("With UDP socket ON " CONFIG_SYSTEM_NAME)
    {
        // Send may have specific characteristics in different platforms such as distinct buffer size overheads so this
        // test case serves to validate assumptions with a dataset for each platform.

        // We try to avoid platform specific code in tests but in this case certain system settings are critical for the
        // outcome of the test case and will most certainly get testers by surprise. On Linux some test samples require
        // the maximum buffer memory to be increased and on BSD/Darwin some samples require the maximum datagram size to
        // be increased.
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

        // @formatter:off
        // Table: destination | remote
        const std::vector<std::tuple<int, clarinet_endpoint, clarinet_endpoint>> recipients = {
            { 0, { clarinet_addr_any_ipv4, 0 }, clarinet_make_endpoint(clarinet_make_ipv4(8, 8, 8, 8), 9) }, // to google DNS ipv4 with a discard service port
            #if CLARINET_ENABLE_IPV6 && !defined(__wsl__) // WSL cannot interact with external IPV6 only addresses due to Hyper-V limitations
            { 1, { clarinet_addr_any_ipv6, 0 }, clarinet_make_endpoint(clarinet_make_ipv6(0x2001, 0x4860, 0x4860, 0, 0, 0, 0, 0x8888, 0), 9) }, // to google DNS ipv6 with a discard service port
            #endif
        };

        // Passing a send-buffer size of -1 here skips setting the buffer size and falls back to using the system
        // default in each platform.

        // Table: sample | send buffer size | send length | count | expected
        const std::vector<std::tuple<int, int, int, int, int>> samples = {
            #if defined(_WIN32)

            // Setting buffer size to -1 has the effect of using the system's default value which on Windows is 8192.
            // CHECK: Is there a registry key to change the system default value?

            {   0,     -1,               0,    1,       CLARINET_ENONE },
            {   1,     -1,               1,    1,       CLARINET_ENONE },
            {   2,     -1,               2,    1,       CLARINET_ENONE },
            {   3,     -1,               4,    1,       CLARINET_ENONE },
            {   4,     -1,               8,    1,       CLARINET_ENONE },
            {   5,     -1,              16,    1,       CLARINET_ENONE },
            {   6,     -1,              32,    1,       CLARINET_ENONE },
            {   7,     -1,              64,    1,       CLARINET_ENONE },
            {   8,     -1,             128,    1,       CLARINET_ENONE },
            {   9,     -1,             256,    1,       CLARINET_ENONE },
            {  10,     -1,             512,    1,       CLARINET_ENONE },
            {  11,     -1,            1024,    1,       CLARINET_ENONE },
            {  12,     -1,            2048,    1,       CLARINET_ENONE },
            {  13,     -1,            4096,    1,       CLARINET_ENONE },
            {  14,     -1,            8192,    1,       CLARINET_ENONE },
            {  15,     -1,           16384,    1,       CLARINET_ENONE },
            {  16,     -1,           32768,    1,       CLARINET_ENONE },
            {  17,     -1,           65507,    1,       CLARINET_ENONE },
            {  18,     -1,           65508,    1,    CLARINET_EMSGSIZE },
            {  19,     -1,           65535,    1,    CLARINET_EMSGSIZE },

            // Windows does not impose a minimum send buffer size so 256 is actually 256 but the first message always
            // bypasses the buffer regardless of the buffer size. It is not clear if this is due to a separate internal
            // buffer or if this is because the message is dispatched directly to the NIC tx queue.
            // CHECK: Is this always really the case or does it depend on a specific feature of the NIC ?
            // Windows mentions SO_MAX_MSG_SIZE should be used to determine the maximum message size supported by the
            // protocol but this is known to be 65507 for udp/ipv4 and 65527 for udp/ipv6.

            {  20,    256,               0,    1,       CLARINET_ENONE },
            {  21,    256,               1,    1,       CLARINET_ENONE },
            {  22,    256,               2,    1,       CLARINET_ENONE },
            {  23,    256,               4,    1,       CLARINET_ENONE },
            {  24,    256,               8,    1,       CLARINET_ENONE },
            {  25,    256,              16,    1,       CLARINET_ENONE },
            {  26,    256,              32,    1,       CLARINET_ENONE },
            {  27,    256,              64,    1,       CLARINET_ENONE },
            {  28,    256,             128,    1,       CLARINET_ENONE },
            {  29,    256,             256,    1,       CLARINET_ENONE },
            {  30,    256,             512,    1,       CLARINET_ENONE },
            {  31,    256,            1024,    1,       CLARINET_ENONE },
            {  32,    256,            2048,    1,       CLARINET_ENONE },
            {  33,    256,            4096,    1,       CLARINET_ENONE },
            {  34,    256,            8192,    1,       CLARINET_ENONE },
            {  35,    256,           16384,    1,       CLARINET_ENONE },
            {  36,    256,           32768,    1,       CLARINET_ENONE },
            {  37,    256,           65507,    1,       CLARINET_ENONE },
            {  38,    256,           65508,    1,    CLARINET_EMSGSIZE },
            {  39,    256,           65535,    1,    CLARINET_EMSGSIZE },
            {  40,   4096,               0,    1,       CLARINET_ENONE },
            {  41,   4096,               1,    1,       CLARINET_ENONE },
            {  42,   4096,               2,    1,       CLARINET_ENONE },
            {  43,   4096,               4,    1,       CLARINET_ENONE },
            {  44,   4096,               8,    1,       CLARINET_ENONE },
            {  45,   4096,              16,    1,       CLARINET_ENONE },
            {  46,   4096,              32,    1,       CLARINET_ENONE },
            {  47,   4096,              64,    1,       CLARINET_ENONE },
            {  48,   4096,             128,    1,       CLARINET_ENONE },
            {  49,   4096,             256,    1,       CLARINET_ENONE },
            {  50,   4096,             512,    1,       CLARINET_ENONE },
            {  51,   4096,            1024,    1,       CLARINET_ENONE },
            {  52,   4096,            2048,    1,       CLARINET_ENONE },
            {  53,   4096,            4096,    1,       CLARINET_ENONE },
            {  54,   4096,            8192,    1,       CLARINET_ENONE },
            {  55,   4096,           16384,    1,       CLARINET_ENONE },
            {  56,   4096,           32768,    1,       CLARINET_ENONE },
            {  57,   4096,           65507,    1,       CLARINET_ENONE },
            {  58,   4096,           65508,    1,    CLARINET_EMSGSIZE },
            {  59,   4096,           65535,    1,    CLARINET_EMSGSIZE },
            {  60,   4096,               0,    2,       CLARINET_ENONE },
            {  61,   4096,               1,    2,       CLARINET_ENONE },
            {  62,   4096,               2,    2,       CLARINET_ENONE },
            {  63,   4096,               4,    2,       CLARINET_ENONE },
            {  64,   4096,               8,    2,       CLARINET_ENONE },
            {  65,   4096,              16,    2,       CLARINET_ENONE },
            {  66,   4096,              32,    2,       CLARINET_ENONE },
            {  67,   4096,              64,    2,       CLARINET_ENONE },
            {  68,   4096,             128,    2,       CLARINET_ENONE },
            {  69,   4096,             256,    2,       CLARINET_ENONE },
            {  70,   4096,             512,    2,       CLARINET_ENONE },
            {  71,   4096,            1024,    2,       CLARINET_ENONE },
            {  72,   4096,            2048,    2,       CLARINET_ENONE },

            // Messages do not require extra space in the buffer for protocol/system overhead so the MTU is irrelevant
            // (whether fragmented or not the message is going to take the exact same space). However, the last byte of
            // the send-buffer is reserved so a message can only be buffered if size < available space (not <=). In the
            // following samples the first message is expected to bypass the buffer but the remaining ones are stored.

            {  73,   4096,            4096,    2,      CLARINET_EAGAIN },
            {  74,   4096 * 1 + 1,    4096,    2,       CLARINET_ENONE },
            {  75,   4096,            4096,    3,      CLARINET_EAGAIN },
            {  76,   4096 * 3,        4096,    3,       CLARINET_ENONE },
            {  77,   4096 * 2,        4096,    3,      CLARINET_EAGAIN },
            {  78,   4096 * 2 + 1,    4096,    3,       CLARINET_ENONE },
            {  79,   4096 * 9 + 1,    4096,   10,       CLARINET_ENONE },
            {  80,   4096,            2048,    3,      CLARINET_EAGAIN },
            {  81,   4096 + 1,        2048,    3,       CLARINET_ENONE },
            {  82,     16 * 199,        16,  200,      CLARINET_EAGAIN },
            {  83,     16 * 199 + 1,    16,  200,       CLARINET_ENONE },
            {  84,     48 * 4999,       48, 5000,      CLARINET_EAGAIN },
            {  85,     48 * 4999 + 1,   48, 5000,       CLARINET_ENONE },
            {  86,     49 * 4999,       49, 5000,      CLARINET_EAGAIN },
            {  87,     49 * 4999 + 1,   49, 5000,       CLARINET_ENONE },

            // Setting the buffer size to 0 effectively disables buffering on Windows so every second packet should fail
            // except when the message size is zero in which case it is always successful.

            {  88,      0,               0,   99,       CLARINET_ENONE },
            {  89,      0,               1,    2,      CLARINET_EAGAIN },
            {  90,      0,               2,    2,      CLARINET_EAGAIN },
            {  91,      0,               4,    2,      CLARINET_EAGAIN },
            {  92,      0,               8,    2,      CLARINET_EAGAIN },
            {  93,      0,              16,    2,      CLARINET_EAGAIN },
            {  94,      0,              32,    2,      CLARINET_EAGAIN },
            {  95,      0,              64,    2,      CLARINET_EAGAIN },
            {  96,      0,             128,    2,      CLARINET_EAGAIN },
            {  97,      0,             256,    2,      CLARINET_EAGAIN },
            {  98,      0,             512,    2,      CLARINET_EAGAIN },
            {  99,      0,            1024,    2,      CLARINET_EAGAIN },
            { 100,      0,            2048,    2,      CLARINET_EAGAIN },
            { 101,      0,            4096,    2,      CLARINET_EAGAIN },
            { 102,      0,            8192,    2,      CLARINET_EAGAIN },
            { 103,      0,           16384,    2,      CLARINET_EAGAIN },
            { 104,      0,           32768,    2,      CLARINET_EAGAIN },
            { 105,      0,           65507,    2,      CLARINET_EAGAIN },
            { 106,      0,           65508,    2,    CLARINET_EMSGSIZE },
            { 107,      0,           65535,    2,    CLARINET_EMSGSIZE },

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

            {   0,     -1,               0,    1,       CLARINET_ENONE },
            {   1,     -1,               1,    1,       CLARINET_ENONE },
            {   2,     -1,               2,    1,       CLARINET_ENONE },
            {   3,     -1,               4,    1,       CLARINET_ENONE },
            {   4,     -1,               8,    1,       CLARINET_ENONE },
            {   5,     -1,              16,    1,       CLARINET_ENONE },
            {   6,     -1,              32,    1,       CLARINET_ENONE },
            {   7,     -1,              64,    1,       CLARINET_ENONE },
            {   8,     -1,             128,    1,       CLARINET_ENONE },
            {   9,     -1,             256,    1,       CLARINET_ENONE },
            {  10,     -1,             512,    1,       CLARINET_ENONE },
            {  11,     -1,            1024,    1,       CLARINET_ENONE },
            {  12,     -1,            2048,    1,       CLARINET_ENONE },
            {  13,     -1,            4096,    1,       CLARINET_ENONE },
            {  14,     -1,            8192,    1,       CLARINET_ENONE },
            {  15,     -1,           16384,    1,       CLARINET_ENONE },
            {  16,     -1,           32768,    1,       CLARINET_ENONE },
            {  17,     -1,           65507,    1,       CLARINET_ENONE },
            {  18,     -1,           65508,    1,    CLARINET_EMSGSIZE },
            {  19,     -1,           65535,    1,    CLARINET_EMSGSIZE },

            // Setting the buffer size to 0 has the effect of using the system's minimum value which on Linux x64 is 4608.

            {  20,      0,               0,    1,       CLARINET_ENONE },
            {  21,      0,               1,    1,       CLARINET_ENONE },
            {  22,      0,               2,    1,       CLARINET_ENONE },
            {  23,      0,               4,    1,       CLARINET_ENONE },
            {  24,      0,               8,    1,       CLARINET_ENONE },
            {  25,      0,              16,    1,       CLARINET_ENONE },
            {  26,      0,              32,    1,       CLARINET_ENONE },
            {  27,      0,              64,    1,       CLARINET_ENONE },
            {  28,      0,             128,    1,       CLARINET_ENONE },
            {  29,      0,             256,    1,       CLARINET_ENONE },
            {  30,      0,             512,    1,       CLARINET_ENONE },
            {  31,      0,            1024,    1,       CLARINET_ENONE },
            {  32,      0,            2048,    1,       CLARINET_ENONE },

            // With MTU=1500, a message of 4096 bytes will produce 3 ipv4 fragments with payloads of 1472, 1480 and 1142
            // bytes respectively. The first fragment bypasses the buffer but the other 2 are buffered and require 2304
            // bytes each so the total buffer space used is 2304 * 2 = 4608 bytes. Since we're setting the buffer size to
            // 256, which is below minimum, the system will round it up to 4608 on x64 (the minimum) which is exactly the
            // size we need.

            // CHECK: 33 and 34 may fail on x86 because the minimum send buffer size might be lower than 4608 as it depends
            // on the size of struct sk_buff aligned to 32. Should we have an ifdef for x86 and expect CL_ENOBUFS instead ?

            {  33,      0,            4096,    1,       CLARINET_ENONE },

            // With MTU=1500, a message of 4432 bytes fragments into payloads of 1472, 1480 and 1480 bytes respectively. So
            // this is the maximum message size the minimum buffers can transmit.

            {  34,      0,            4432,    1,       CLARINET_ENONE },

            // With MTU=1500, a message of 4433 bytes will produce 4 fragments: 2 with 1480 bytes of payload, 1 with 1473
            // bytes of payload and the last one with 0 payload bytes just carrying the udp header.

            {  35,      0,            4433,    1,     CLARINET_ENOBUFS },
            {  34,      0,            8192,    1,     CLARINET_ENOBUFS },
            {  35,      0,           16384,    1,     CLARINET_ENOBUFS },
            {  36,      0,           32768,    1,     CLARINET_ENOBUFS },
            {  37,      0,           65507,    1,     CLARINET_ENOBUFS },
            {  38,      0,           65508,    1,    CLARINET_EMSGSIZE },
            {  39,      0,           65535,    1,    CLARINET_EMSGSIZE },

            // The packet overhead in Linux is directly related to the payload size. A payload size of 16 bytes requires 768
            // bytes of buffer space (to send).

            {  40,    768 *  99,         1,  100,      CLARINET_EAGAIN },
            {  41,    768 *  99 + 1,     1,  100,      CLARINET_EAGAIN },
            {  42,    768 *  99 + 2,     1,  100,       CLARINET_ENONE },
            {  43,    768 *  99 + 2,     5,  100,       CLARINET_ENONE },
            {  44,    768 *  99 + 2,     6,  100,      CLARINET_EAGAIN },
            {  45,   1280 *  99,         6,  100,      CLARINET_EAGAIN },
            {  46,   1280 *  99 + 1,     6,  100,      CLARINET_EAGAIN },
            {  47,   1280 *  99 + 2,     6,  100,       CLARINET_ENONE },
            {  48,   1280 *  99 + 2,   517,  100,       CLARINET_ENONE },
            {  49,   1280 *  99 + 2,   518,  100,      CLARINET_EAGAIN },
            {  50,   2304 *  99,       518,  100,      CLARINET_EAGAIN },
            {  51,   2304 *  99 + 1,   518,  100,      CLARINET_EAGAIN },
            {  52,   2304 *  99 + 2,   518,  100,       CLARINET_ENONE },
            {  53,   2304 *  99 + 2,  1472,  100,       CLARINET_ENONE },
            {  54,   2304 * 299,      4096,  100,      CLARINET_EAGAIN },
            {  55,   2304 * 299 + 1,  4096,  100,      CLARINET_EAGAIN },

            // The following samples require net.core.wmem_max=6398722

            {  56,   2304 *  299 + 2, 4096,  100,       CLARINET_ENONE },
            {  57,   1280 *  199,       16,  200,      CLARINET_EAGAIN },
            {  58,   1280 *  199 + 2,   16,  200,       CLARINET_ENONE },
            {  59,   1280 * 4999,       48, 5000,      CLARINET_EAGAIN },
            {  60,   1280 * 4999 + 2,   48, 5000,       CLARINET_ENONE },
            {  61,   1280 * 4999,       49, 5000,      CLARINET_EAGAIN },
            {  62,   1280 * 4999 + 2,   49, 5000,       CLARINET_ENONE },

            #elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__ )

            // BSD/Drawin does not have a proper send buffer for UDP sockets. Instead the value of SO_SNDBUF is only used to
            // limit the size of a the message that can be transmitted by the socket.

            // Setting the buffer size to -1 has the effect of using the system's default value which on Darwin is the
            // value of sysctl:net.inet.udp.maxdgram.

            {   0,     -1,               0,    1,       CLARINET_ENONE },
            {   1,     -1,               1,    1,       CLARINET_ENONE },
            {   2,     -1,               2,    1,       CLARINET_ENONE },
            {   3,     -1,               4,    1,       CLARINET_ENONE },
            {   4,     -1,               8,    1,       CLARINET_ENONE },
            {   5,     -1,              16,    1,       CLARINET_ENONE },
            {   6,     -1,              32,    1,       CLARINET_ENONE },
            {   7,     -1,              64,    1,       CLARINET_ENONE },
            {   8,     -1,             128,    1,       CLARINET_ENONE },
            {   9,     -1,             256,    1,       CLARINET_ENONE },
            {  10,     -1,             512,    1,       CLARINET_ENONE },
            {  11,     -1,            1024,    1,       CLARINET_ENONE },
            {  12,     -1,            2048,    1,       CLARINET_ENONE },
            {  13,     -1,            4096,    1,       CLARINET_ENONE },
            {  14,     -1,            8192,    1,       CLARINET_ENONE },

            // Setting the buffer size to 0 also falls back to using the system's default in our implementation because
            // BSD/Darwin would otherwise reject it with EINVAL.

            {  15,      0,               0,    1,       CLARINET_ENONE },
            {  16,      0,               1,    1,       CLARINET_ENONE },
            {  17,      0,               2,    1,       CLARINET_ENONE },
            {  18,      0,               4,    1,       CLARINET_ENONE },
            {  19,      0,               8,    1,       CLARINET_ENONE },
            {  20,      0,              16,    1,       CLARINET_ENONE },
            {  21,      0,              32,    1,       CLARINET_ENONE },
            {  22,      0,              64,    1,       CLARINET_ENONE },
            {  23,      0,             128,    1,       CLARINET_ENONE },
            {  24,      0,             256,    1,       CLARINET_ENONE },
            {  25,      0,             512,    1,       CLARINET_ENONE },
            {  26,      0,            1024,    1,       CLARINET_ENONE },
            {  27,      0,            2048,    1,       CLARINET_ENONE },
            {  28,      0,            4096,    1,       CLARINET_ENONE },
            {  29,      0,            8192,    1,       CLARINET_ENONE },

            // There is no overhead per message in the send buffer and there is no system minimum.

            {  30,      1,               0,    1,       CLARINET_ENONE },
            {  31,      1,               1,    1,       CLARINET_ENONE },
            {  32,      1,               2,    1,    CLARINET_EMSGSIZE },
            {  33,   8192,               4,    1,       CLARINET_ENONE },
            {  34,   8192,               8,    1,       CLARINET_ENONE },
            {  35,   8192,              16,    1,       CLARINET_ENONE },
            {  36,   8192,              32,    1,       CLARINET_ENONE },
            {  37,   8192,              64,    1,       CLARINET_ENONE },
            {  38,   8192,             128,    1,       CLARINET_ENONE },
            {  39,   8192,             256,    1,       CLARINET_ENONE },
            {  40,   8192,             512,    1,       CLARINET_ENONE },
            {  41,   8192,            1024,    1,       CLARINET_ENONE },
            {  42,   8192,            2048,    1,       CLARINET_ENONE },
            {  43,   8192,            4096,    1,       CLARINET_ENONE },
            {  44,   8192,            8192,    1,       CLARINET_ENONE },
            {  45,   8191,            8192,    2,    CLARINET_EMSGSIZE },
            {  46,   8192,            8192,    2,       CLARINET_ENONE },

            // With MTU=1500, a message of 4096 bytes will produce 3 ipv4 fragments with payloads of 1472, 1480 and 1142
            // bytes respectively but since there is no actual send buffer we should be able to send as many fragments as
            // the network layer can queue. The problem is that if the queue is full packets are silently dropped and there
            // is no way around it. The call to send(2) doesn't fail and the user program never gets to know those packets
            // were never transmitted.

            {  47,   4096,            4096,    1,       CLARINET_ENONE },
            {  48,   4096,            4096,    2,       CLARINET_ENONE },
            {  49,   4096,            4096,   10,       CLARINET_ENONE },
            {  50,   4096,            4096,  100,       CLARINET_ENONE },
            {  51,   4096,            4096,  500,       CLARINET_ENONE },
            {  52,   4096,              48, 5000,       CLARINET_ENONE },
            {  53,   4096,              49, 5000,       CLARINET_ENONE },
            {  54,   4096,            4097, 5000,    CLARINET_EMSGSIZE },

            // The following samples require net.inet.udp.maxdgram=65535

            {  55,     -1,           16384,    1,     CLARINET_ENOBUFS },
            {  56,     -1,           32768,    1,     CLARINET_ENOBUFS },
            {  57,     -1,           65507,    1,     CLARINET_ENOBUFS },
            {  58,     -1,           65508,    1,    CLARINET_EMSGSIZE },
            {  59,     -1,           65535,    1,    CLARINET_EMSGSIZE },

            {  60,      0,           16384,    1,     CLARINET_ENOBUFS },
            {  61,      0,           32768,    1,     CLARINET_ENOBUFS },
            {  62,      0,           65507,    1,     CLARINET_ENOBUFS },
            {  63,      0,           65508,    1,    CLARINET_EMSGSIZE },
            {  64,      0,           65535,    1,    CLARINET_EMSGSIZE },

            {  65,  65535,           16384,    1,     CLARINET_ENOBUFS },
            {  66,  65535,           32768,    1,     CLARINET_ENOBUFS },
            {  67,  65535,           65507,    1,     CLARINET_ENOBUFS },
            {  68,  65535,           65508,    1,    CLARINET_EMSGSIZE },
            {  69,  65535,           65535,    1,    CLARINET_EMSGSIZE },

                #endif
            };
        // @formatter:on

        SAMPLES(samples);
        SAMPLES(recipients);

        int sample;
        int send_buffer_size;
        int send_length;
        int count;
        int expected;

        std::tie(sample, send_buffer_size, send_length, count, expected) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        int recipient;
        clarinet_endpoint local;
        clarinet_endpoint remote;
        std::tie(recipient, local, remote) = GENERATE_REF(from_samples(recipients));
        FROM(recipient);

        TEST_UDP_SENDTO(&local, &remote, (int32_t)send_buffer_size, send_length, count, expected);
    }
}

TEST_CASE("Socket Recv")
{

}

TEST_CASE("Socket Recv From")
{
    CLARINET_TEST_CASE_LIMITED_ON_WSL();

    SECTION("With UDP socket ON " CONFIG_SYSTEM_NAME)
    {

    }
}

TEST_CASE("Socket Shutdown")
{
    SECTION("With NULL socket")
    {
        int errcode = clarinet_socket_shutdown(nullptr, CLARINET_SHUTDOWN_BOTH);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNOPEN socket")
    {
        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_shutdown(nullptr, CLARINET_SHUTDOWN_BOTH);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With UNCONNECTED socket")
    {
        clarinet_family family = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_AF_LIST
        }));

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_OPEN_SUPPORTED_PROTO_LIST
        }));

        FROM(family);
        FROM(proto);

        clarinet_socket socket;
        clarinet_socket* sp = &socket;
        clarinet_socket_init(sp);

        int errcode = clarinet_socket_open(sp, family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_shutdown(sp, CLARINET_SHUTDOWN_BOTH);
        #if defined(_WIN32)
        // On Windows, unconnected UDP sockets can be shutdown
        if (proto == CLARINET_PROTO_UDP)
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        else
            REQUIRE(Error(errcode) == Error(CLARINET_ENOTCONN));
        #else
        REQUIRE(Error(errcode) == Error(CLARINET_ENOTCONN));
        #endif
    }

    SECTION("With INVALID flags")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint, clarinet_addr>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 }, clarinet_addr_loopback_ipv4 },
            { 1, { clarinet_addr_loopback_ipv4, 0 }, clarinet_addr_loopback_ipv4 },
            #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 }, clarinet_addr_loopback_ipv6 },
            { 3, { clarinet_addr_loopback_ipv6, 0 }, clarinet_addr_loopback_ipv6 },
            #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        clarinet_addr addr;
        std::tie(sample, endpoint, addr) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_CONNECT_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        int errcode = clarinet_socket_open(ssp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onserverexit = finalizer([&ssp]
        {
            clarinet_socket_close(ssp);
        });

        errcode = clarinet_socket_bind(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_local_endpoint(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        // Set the endpoint for the clients to connect
        endpoint.addr = addr;

        // If the server protocol supports listen then make it listen
        if (proto & CLARINET_TEST_SOCKET_LISTEN_PROTO_BSET)
        {
            errcode = clarinet_socket_listen(ssp, 1);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        }

        clarinet_socket client;
        clarinet_socket* csp = &client;

        clarinet_socket_init(csp);
        errcode = clarinet_socket_open(csp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onclientexit = finalizer([&csp]
        {
            clarinet_socket_close(csp);
        });

        errcode = clarinet_socket_connect(csp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_shutdown(csp, INT_MIN);
        REQUIRE(Error(errcode) == Error(CLARINET_EINVAL));
    }

    SECTION("With CONNECTED socket")
    {
        // @formatter:off
        const std::vector<std::tuple<int, clarinet_endpoint, clarinet_addr>> samples = {
            { 0, { clarinet_addr_any_ipv4,      0 }, clarinet_addr_loopback_ipv4 },
            { 1, { clarinet_addr_loopback_ipv4, 0 }, clarinet_addr_loopback_ipv4 },
            #if CLARINET_ENABLE_IPV6
            { 2, { clarinet_addr_any_ipv6,      0 }, clarinet_addr_loopback_ipv6 },
            { 3, { clarinet_addr_loopback_ipv6, 0 }, clarinet_addr_loopback_ipv6 },
            #endif // CLARINET_ENABLE_IPV6
        };
        // @formatter:on

        SAMPLES(samples);

        int sample;
        clarinet_endpoint endpoint;
        clarinet_addr addr;
        std::tie(sample, endpoint, addr) = GENERATE_REF(from_samples(samples));
        FROM(sample);

        clarinet_proto proto = GENERATE(values({
            CLARINET_TEST_SOCKET_CONNECT_SUPPORTED_PROTO_LIST
        }));
        FROM(proto);

        clarinet_socket server;
        clarinet_socket* ssp = &server;
        clarinet_socket_init(ssp);

        int errcode = clarinet_socket_open(ssp, endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onserverexit = finalizer([&ssp]
        {
            clarinet_socket_close(ssp);
        });

        errcode = clarinet_socket_bind(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        errcode = clarinet_socket_local_endpoint(ssp, &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        // Set the endpoint for the clients to connect
        endpoint.addr = addr;

        // If the server protocol supports listen then make it listen
        if (proto & CLARINET_TEST_SOCKET_LISTEN_PROTO_BSET)
        {
            errcode = clarinet_socket_listen(ssp, 2);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        }

        clarinet_socket client[2];
        clarinet_socket* csp[2] = { &client[0], &client[1] };

        clarinet_socket_init(csp[0]);
        errcode = clarinet_socket_open(csp[0], endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onclient0exit = finalizer([&csp]
        {
            clarinet_socket_close(csp[0]);
        });

        errcode = clarinet_socket_connect(csp[0], &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        clarinet_socket_init(csp[1]);
        errcode = clarinet_socket_open(csp[1], endpoint.addr.family, proto);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        const auto onclient1exit = finalizer([&csp]
        {
            clarinet_socket_close(csp[1]);
        });
        errcode = clarinet_socket_connect(csp[1], &endpoint);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        // Shutdown socket before being accepted (for protocols that can accept)
        errcode = clarinet_socket_shutdown(csp[0], CLARINET_SHUTDOWN_BOTH);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));

        if (proto & CLARINET_TEST_SOCKET_ACCEPT_PROTO_BSET)
        {
            clarinet_socket accepted;
            clarinet_socket* asp = &accepted;
            clarinet_socket_init(asp);

            errcode = clarinet_socket_accept(ssp, asp, &endpoint);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
            // Note that the finalizer will close the socket when the if statement is finished
            const auto onacceptedexit = finalizer([&asp]
            {
                clarinet_socket_close(asp);
            });

            errcode = clarinet_socket_shutdown(asp, CLARINET_SHUTDOWN_BOTH);
            REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
        }

        // Shutdown socket after being accepted (for protocols that can accept)
        errcode = clarinet_socket_shutdown(csp[1], CLARINET_SHUTDOWN_BOTH);
        REQUIRE(Error(errcode) == Error(CLARINET_ENONE));
    }
}
