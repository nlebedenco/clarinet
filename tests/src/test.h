#pragma once
#ifndef CLARINET_TESTS_TEST_H
#define CLARINET_TESTS_TEST_H

#include "clarinet/clarinet.h"
#if defined(HAVE_CONFIG_H)
#include "config.h" 
#endif

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cctype>
#include <climits>
#include <cstring>
#include <string>

#if defined(_MSC_VER)
#pragma warning(push)
#if (_MSC_VER >= 1800) /* Disable warnings we can't fix. */
#pragma warning(disable : 26812)    /* disable warning for unscoped enum warning (/wd26812 is not recognized) */
#pragma warning(disable : 26495)    /* disable warning for member variable not initialized */
#pragma warning(disable : 4388)     /* disable warning about comparing signed/unsigned int */
#pragma warning(disable : 5204)     /* disable warning for abstract/virtual class not having a virtual dtor */
#endif
#endif

#include "catch2/catch_all.hpp"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#define FROM(i)                 INFO(format("GENERATE INDEX %d", (i)))

#define EXPLAIN(fmt, ...)       INFO(format(fmt, __VA_ARGS__))

template<typename T>
T* 
coalesce(T* p, T* fallback) { return (p == NULL) ? fallback : p; }

static inline 
std::string 
format(const char* fmt, ...) {
    size_t size = 64;    
    va_list ap;
    while (1) 
    {     // Maximum two passes on a POSIX system...
        auto buf = std::make_unique<char[]>(size);
        va_start(ap, fmt);
        int n = vsnprintf(buf.get(), size, fmt, ap);
        va_end(ap);
        if (n >= 0)
        {
            if ((size_t)n < size)
                return std::string(buf.get(), buf.get() + n);

            size = (size_t)n + 1;   // +1 for '\0'
            continue;
        }
        
        throw std::runtime_error("Invalid format string");
    }

    /* NEVER REACHED */
}

using namespace Catch::Matchers;

template<typename ElementType>
struct Segment
{
    ElementType* begin() const { return data; }
    ElementType* end() const { return data + len; }

    Segment(ElementType* _data, size_t _len) :
        data(_data),
        len(_len)
    {}

private:
    ElementType* data;
    size_t len;

};

template<typename ElementType>
ElementType* 
begin(Segment<ElementType>& segment) { return segment.begin();  }

template<typename ElementType>
ElementType* 
end(Segment<ElementType>& segment) { return segment.end(); }

template<typename ElementType = uint8_t>
Segment<ElementType> 
memory(void* buf, size_t len) { return Segment<ElementType>((ElementType*)buf, len); }


template<typename Range>
struct EqualsRangeMatcher : Catch::Matchers::MatcherGenericBase 
{
    EqualsRangeMatcher(Range const& _range):
        range(_range)
    {}

    template<typename OtherRange>
    bool match(OtherRange const& other) const 
    {
        using std::begin; 
        using std::end;

        return std::equal(begin(range), end(range), begin(other), end(other));
    }

    std::string describe() const override 
    {
        return "Equals: " + Catch::rangeToString(range);
    }

private:
    Range const& range;
};

template<typename Range>
auto 
Equals(const Range& range) -> EqualsRangeMatcher<Range> { return EqualsRangeMatcher<Range>{range}; }

static inline 
auto 
Equals(const char* range) -> StringEqualsMatcher { return Equals(std::string(range)); }

#endif /* CLARINET_TESTS_TEST_H */