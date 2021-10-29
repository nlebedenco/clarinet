#include "test.h"

std::string
to_string(enum clarinet_error value)
{
    return clarinet_error_name((int)value);
}

std::string
to_string(enum clarinet_family value)
{
    return clarinet_family_name((int)value);
}

std::string
to_string(enum clarinet_proto value)
{
    return clarinet_proto_name((int)value);
}

std::string
to_string(const char* s)
{
    return s;
}

autoload::
autoload() noexcept
{
    clarinet_initialize();
}

autoload::
~autoload()
{
    clarinet_finalize();
}

starter::
starter(std::function<void()> const& callback) noexcept
{
    callback();
}

void
memnoise(void* dst,
         size_t n)
{
    static auto noise = (uint8_t)clock();

    for (size_t i = 0; i < n; ++i)
    {
        ((uint8_t*)dst)[i] = noise++;
    }
}

finalizer::
finalizer(finalizer&& other) noexcept
    : cleanup(std::move(other.cleanup))
{
    dismiss(other);
}

finalizer::
~finalizer()
{
    cleanup();
}

void
dismiss(finalizer& g)
{
    g.cleanup = []
    {
    };
}
