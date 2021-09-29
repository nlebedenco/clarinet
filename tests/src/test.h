#pragma once
#ifndef CLARINET_TESTS_TEST_H
#define CLARINET_TESTS_TEST_H

#include "clarinet/clarinet.h"

#if defined(HAVE_CONFIG_H)
    #include "config.h"
#endif

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cctype>
#include <climits>
#include <cstring>
#include <string>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <ctime>

#include <functional>
#include <utility>

#include "catch2/catch_all.hpp"

#define FROM(i)                 INFO(format("FROM %s: %d", (#i), (i)))
#define EXPLAIN(fmt, ...)       INFO(format(fmt, __VA_ARGS__))

#include <chrono>
#include <thread>

#define suspend(x) std::this_thread::sleep_for(std::chrono::milliseconds(x))

void
memnoise(void* dst,
         size_t n);

static inline
std::string
format(const char* fmt,
       ...)
{
    size_t size = 64;
    va_list ap;
    while (true) // maximum two passes on a POSIX system...
    {
        auto buf = std::make_unique<char[]>(size);
        va_start(ap, fmt);
        int n = vsnprintf(buf.get(), size, fmt, ap);
        va_end(ap);
        if (n >= 0)
        {
            if ((size_t)n < size)
                return { buf.get(), buf.get() + n };

            size = (size_t)n + 1;   // +1 for '\0'
            continue;
        }

        throw std::runtime_error("Invalid format string");
    }

    // NEVER REACHED
}

struct not_copyable
{
    not_copyable&
    operator=(const not_copyable&) = delete;

    not_copyable(const not_copyable&) = delete;

    not_copyable&
    operator=(not_copyable&&) = default;

    not_copyable() = default;

    not_copyable(not_copyable&&) = default;
};

class finalizer: public not_copyable
{
private:
    std::function<void()> cleanup;

public:
    friend
    void
    dismiss(finalizer& g);

    template <class Func>
    finalizer(Func const& _cleanup)
        : cleanup(_cleanup)
    {
    }

    finalizer(finalizer&& other) noexcept;

    ~finalizer();
};

template <typename T>
struct segment
{
    T*
    begin() const
    {
        return data;
    }

    T*
    end() const
    {
        return data + len;
    }

    segment(T* _data,
            size_t _len):
        data(_data),
        len(_len)
    {
    }

private:
    T* data;
    size_t len;

};

/* This is so that printing ranges out of segment<T> when REQUIRE_THAT fails can print something other than '?' */
template <typename T,
    typename = typename std::enable_if<!std::is_same<T, char>::type>>
std::ostream&
operator<<(std::ostream& os,
           const T* value)
{
    return value ? os << *value : os << "(nullptr)";
}

template <typename T>
T*
begin(segment<T>& segment)
{
    return segment.begin();
}

template <typename T>
T*
end(segment<T>& segment)
{
    return segment.end();
}

template <typename T = uint8_t>
segment<T>
memory(void* buf,
       size_t len)
{
    return segment<T>((T*)buf, len);
}


using namespace Catch::Matchers;

template <typename T>
struct EqualsRangeMatcher: Catch::Matchers::MatcherGenericBase
{
    explicit EqualsRangeMatcher(T const& _range):
        range(_range)
    {
    }

    template <typename OtherRange>
    bool
    match(OtherRange const& other) const
    {
        using std::begin;
        using std::end;

        return std::equal(begin(range), end(range), begin(other), end(other));
    }

    std::string
    describe() const override
    {
        return "Equals: " + Catch::rangeToString(range);
    }

private:
    T const& range;
};

template <typename T>
EqualsRangeMatcher<T>
Equals(const T& range)
{
    return EqualsRangeMatcher<T>{ range };
}

static inline
StringEqualsMatcher
Equals(const char* range)
{
    return Equals(std::string(range));
}

template <typename T>
struct is_ordinal: std::integral_constant<bool, std::is_integral<T>::value || std::is_enum<T>::value>
{
};

template <typename T, typename = typename std::enable_if<is_ordinal<T>::value>::type>
struct HexadecimalFormatter
{
    T value;

    explicit HexadecimalFormatter(const T& _value): value(_value)
    {
    }
};

template <typename T>
HexadecimalFormatter<T>
Hex(const T& value)
{
    return HexadecimalFormatter<T>(value);
}

template <typename T, typename U>
inline bool
operator==(const HexadecimalFormatter<T>& lhs,
           const HexadecimalFormatter<U>& rhs)
{
    return (uint64_t)lhs.value == (uint64_t)(rhs.value);
}

template <typename T, typename U>
inline bool
operator<(const HexadecimalFormatter<T>& lhs,
          const HexadecimalFormatter<U>& rhs)
{
    return (uint64_t)lhs.value < (uint64_t)(rhs.value);
}

template <typename T>
std::ostream&
operator<<(std::ostream& os,
           HexadecimalFormatter<T> const& value)
{
    return os << "0x" << std::hex << std::setw((sizeof(T) * 2)) << std::setfill('0') << value.value;
}

template <typename T, typename = typename std::enable_if<is_ordinal<T>::value>::type>
struct ErrorFormatter
{
    enum clarinet_error value;

    explicit ErrorFormatter(const T& _value): value((clarinet_error)_value)
    {
    }
};

template <typename T>
ErrorFormatter<T>
Error(const T& value)
{
    return ErrorFormatter<T>(value);
}

template <typename T, typename U>
inline bool
operator==(const ErrorFormatter<T>& lhs,
           const ErrorFormatter<U>& rhs)
{
    return (int64_t)lhs.value == (int64_t)(rhs.value);
}

template <typename T, typename U>
inline bool
operator==(const T& lhs,
           const ErrorFormatter<U>& rhs)
{
    return ErrorFormatter<T>(lhs) == rhs;
}

template <typename T, typename U>
inline bool
operator==(const ErrorFormatter<T>& lhs,
           const U& rhs)
{
    return lhs == ErrorFormatter<U>(rhs);
}

template <typename T, typename U>
inline bool
operator!=(const ErrorFormatter<T>& lhs,
           const ErrorFormatter<U>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename U>
inline bool
operator!=(const T& lhs,
           const ErrorFormatter<U>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename U>
inline bool
operator!=(const ErrorFormatter<T>& lhs,
           const U& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename U>
inline bool
operator<(const ErrorFormatter<T>& lhs,
          const ErrorFormatter<U>& rhs)
{
    return (int64_t)lhs.value < (int64_t)(rhs.value);
}

template <typename T, typename U>
inline bool
operator<(const T& lhs,
          const ErrorFormatter<U>& rhs)
{
    return ErrorFormatter<T>(lhs) < rhs;
}

template <typename T, typename U>
inline bool
operator<(const ErrorFormatter<T>& lhs,
          const U& rhs)
{
    return lhs < ErrorFormatter<U>(rhs);
}

template <typename T, typename U>
inline bool
operator<=(const ErrorFormatter<T>& lhs,
           const ErrorFormatter<U>& rhs)
{
    return lhs == rhs || lhs < rhs;
}

template <typename T, typename U>
inline bool
operator<=(const T& lhs,
           const ErrorFormatter<U>& rhs)
{
    return lhs == rhs || lhs < rhs;
}

template <typename T, typename U>
inline bool
operator<=(const ErrorFormatter<T>& lhs,
           const U& rhs)
{
    return lhs == rhs || lhs < rhs;
}


template <typename T>
std::ostream&
operator<<(std::ostream& os,
           const ErrorFormatter<T>& value)
{
    return value.value <= 0 ? os << clarinet_error_name(value.value) : os << value.value;
}


#endif // CLARINET_TESTS_TEST_H
