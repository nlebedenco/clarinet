#include "test.h"

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
