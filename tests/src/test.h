#pragma once
#ifndef TESTS_TEST_H
#define TESTS_TEST_H

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
#include <chrono>
#include <thread>
#include <numeric>

#include "catch2/catch_all.hpp"

#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#endif

#define SAMPLES(v)              do { if ((v).empty()) { WARN("No data to test."); return; } } while(0)
#define EXPLAIN(fmt, ...)       INFO(format(fmt, __VA_ARGS__))
#define FROM(i)                 EXPLAIN("FROM %s: %s", (#i), to_string(i).c_str())

#define suspend(x) std::this_thread::sleep_for(std::chrono::milliseconds(x))

#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic pop
#endif

std::string
to_string(enum clarinet_error value);

std::string
to_string(enum clarinet_family value);

std::string
to_string(enum clarinet_proto value);

std::string
to_string(const char* s);

using namespace Catch::Generators;
using std::to_string;

template <typename Container>
GeneratorWrapper<typename Container::value_type>
from_samples(Container const& cnt,
             std::function<bool(const typename Container::value_type&)> predicate)
{
    WARN("Using filtered data.");
    return filter(predicate, from_range(cnt));
}

template <typename Container>
GeneratorWrapper<typename Container::value_type>
from_samples(Container const& cnt,
             int sample)
{
    WARN("Using filtered data.");
    return filter([sample](const typename Container::value_type& item)
        {
            return std::get<0>(item) == sample;
        },
        from_range(cnt));
}

template <typename Container>
GeneratorWrapper<typename Container::value_type>
from_samples(Container const& cnt,
             std::vector<int> const& samples)
{
    WARN("Using filtered data.");
    return filter([&samples](const typename Container::value_type& item)
        {
            return std::find(samples.cbegin(), samples.cend(), std::get<0>(item)) != samples.cend();
        },
        from_range(cnt));
}

template <typename Container>
GeneratorWrapper<typename Container::value_type>
from_samples(Container const& cnt)
{
    return from_range(cnt);
}

void
memnoise(void* dst,
         size_t n);

// clang can't see format() is used inside macros and warns
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

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
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

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

struct autoload: public not_copyable
{
    autoload() noexcept;

    ~autoload();
};

struct starter: public not_copyable
{
    explicit
    starter(std::function<void()> const& callback) noexcept;
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
    explicit
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

// This is so that printing ranges out of segment<T> when REQUIRE_THAT fails can print something other than '?'
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

// clang can't see that this function is only used for implicit casting */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

static inline
StringEqualsMatcher
Equals(const char* range)
{
    return Equals(std::string(range));
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

// CLion can't see that is_ordinal is only used as a metaprogramming template type */
#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedStructInspection"
#endif

template <typename T>
struct is_ordinal: std::integral_constant<bool, std::is_integral<T>::value || std::is_enum<T>::value>
{
};

#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic pop
#endif

/* region Formatter */

template <typename ArgType, typename ValueType,
    typename = typename std::enable_if<is_ordinal<ArgType>::value>::type>
struct Formatter
{
    ValueType value;

    explicit Formatter(const ArgType& _value)
        : value((ValueType)_value)
    {
    }

protected:

    typedef Formatter<ValueType, ArgType> base;
};

template <typename V, typename T>
std::string
to_string(const Formatter<V, T>& f)
{
    return to_string(f.value);
}

/* endregion */

/* region HexadecimalFormatter */

template <typename T>
struct HexadecimalFormatter: public Formatter<T, T>
{
    typedef Formatter<T, T> base;

    explicit
    HexadecimalFormatter(const T& _value): base(_value)
    {
    }
};

#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#endif

template <typename T>
HexadecimalFormatter<T>
Hex(const T& value)
{
    return HexadecimalFormatter<T>(value);
}

#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic pop
#endif


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

/* endregion */

/* region Error Formatter */

template <typename T, typename V = clarinet_error>
struct ErrorFormatter: public Formatter<T, V>
{
    using Formatter<T, V>::Formatter;
};

#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#endif

template <typename T>
ErrorFormatter<T>
Error(const T& value)
{
    return ErrorFormatter<T>(value);
}

#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic pop
#endif

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
           const ErrorFormatter<T>& f)
{
    return f.value <= 0 ? os << to_string(f.value) : os << f.value;
}


/* endregion */

/* region Protocol Formatter */

template <typename T, typename V = clarinet_proto>
struct ProtocolFormatter: public Formatter<T, V>
{
    using Formatter<T, V>::Formatter;
};

#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#endif

template <typename T>
ProtocolFormatter<T>
Protocol(const T& value)
{
    return ProtocolFormatter<T>(value);
}

#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic pop
#endif

template <typename T, typename U>
inline bool
operator==(const ProtocolFormatter<T>& lhs,
           const ProtocolFormatter<U>& rhs)
{
    return (int64_t)lhs.value == (int64_t)(rhs.value);
}

template <typename T, typename U>
inline bool
operator==(const T& lhs,
           const ProtocolFormatter<U>& rhs)
{
    return ProtocolFormatter<T>(lhs) == rhs;
}

template <typename T, typename U>
inline bool
operator==(const ProtocolFormatter<T>& lhs,
           const U& rhs)
{
    return lhs == ProtocolFormatter<U>(rhs);
}

template <typename T, typename U>
inline bool
operator!=(const ProtocolFormatter<T>& lhs,
           const ProtocolFormatter<U>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename U>
inline bool
operator!=(const T& lhs,
           const ProtocolFormatter<U>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename U>
inline bool
operator!=(const ProtocolFormatter<T>& lhs,
           const U& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename U>
inline bool
operator<(const ProtocolFormatter<T>& lhs,
          const ProtocolFormatter<U>& rhs)
{
    return (int64_t)lhs.value < (int64_t)(rhs.value);
}

template <typename T, typename U>
inline bool
operator<(const T& lhs,
          const ProtocolFormatter<U>& rhs)
{
    return ProtocolFormatter<T>(lhs) < rhs;
}

template <typename T, typename U>
inline bool
operator<(const ProtocolFormatter<T>& lhs,
          const U& rhs)
{
    return lhs < ProtocolFormatter<U>(rhs);
}

template <typename T, typename U>
inline bool
operator<=(const ProtocolFormatter<T>& lhs,
           const ProtocolFormatter<U>& rhs)
{
    return lhs == rhs || lhs < rhs;
}

template <typename T, typename U>
inline bool
operator<=(const T& lhs,
           const ProtocolFormatter<U>& rhs)
{
    return lhs == rhs || lhs < rhs;
}

template <typename T, typename U>
inline bool
operator<=(const ProtocolFormatter<T>& lhs,
           const U& rhs)
{
    return lhs == rhs || lhs < rhs;
}


template <typename T>
std::ostream&
operator<<(std::ostream& os,
           const ProtocolFormatter<T>& f)
{
    return f.value <= 0 ? os << to_string(f.value) : os << f.value;
}

/*endregion */

/* region Famliy Formatter */

template <typename T, typename V = clarinet_family>
struct FamilyFormatter: public Formatter<T, V>
{
    using Formatter<T, V>::Formatter;
};


#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#endif

template <typename T>
FamilyFormatter<T>
Family(const T& value)
{
    return FamilyFormatter<T>(value);
}

#if defined(__JETBRAINS_IDE__)
#pragma clang diagnostic pop
#endif

template <typename T, typename U>
inline bool
operator==(const FamilyFormatter<T>& lhs,
           const FamilyFormatter<U>& rhs)
{
    return (int64_t)lhs.value == (int64_t)(rhs.value);
}

template <typename T, typename U>
inline bool
operator==(const T& lhs,
           const FamilyFormatter<U>& rhs)
{
    return FamilyFormatter<T>(lhs) == rhs;
}

template <typename T, typename U>
inline bool
operator==(const FamilyFormatter<T>& lhs,
           const U& rhs)
{
    return lhs == FamilyFormatter<U>(rhs);
}

template <typename T, typename U>
inline bool
operator!=(const FamilyFormatter<T>& lhs,
           const FamilyFormatter<U>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename U>
inline bool
operator!=(const T& lhs,
           const FamilyFormatter<U>& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename U>
inline bool
operator!=(const FamilyFormatter<T>& lhs,
           const U& rhs)
{
    return !(lhs == rhs);
}

template <typename T, typename U>
inline bool
operator<(const FamilyFormatter<T>& lhs,
          const FamilyFormatter<U>& rhs)
{
    return (int64_t)lhs.value < (int64_t)(rhs.value);
}

template <typename T, typename U>
inline bool
operator<(const T& lhs,
          const FamilyFormatter<U>& rhs)
{
    return FamilyFormatter<T>(lhs) < rhs;
}

template <typename T, typename U>
inline bool
operator<(const FamilyFormatter<T>& lhs,
          const U& rhs)
{
    return lhs < FamilyFormatter<U>(rhs);
}

template <typename T, typename U>
inline bool
operator<=(const FamilyFormatter<T>& lhs,
           const FamilyFormatter<U>& rhs)
{
    return lhs == rhs || lhs < rhs;
}

template <typename T, typename U>
inline bool
operator<=(const T& lhs,
           const FamilyFormatter<U>& rhs)
{
    return lhs == rhs || lhs < rhs;
}

template <typename T, typename U>
inline bool
operator<=(const FamilyFormatter<T>& lhs,
           const U& rhs)
{
    return lhs == rhs || lhs < rhs;
}


template <typename T>
std::ostream&
operator<<(std::ostream& os,
           const FamilyFormatter<T>& f)
{
    return f.value <= 0 ? os << to_string(f.value) : os << f.value;
}

/* endregion */

#endif // TESTS_TEST_H
