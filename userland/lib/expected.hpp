/*
Loosely inspired by http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0323r10.html
*/

#pragma once

template<typename ValueType, typename ErrorType>
struct Expected
{
    Expected(ValueType value, ErrorType error, int valid):
    value(value),
    error(error),
    valid(valid)
    {}

    Expected(const Expected& other):
    value(other.value),
    error(other.value),
    valid(other._valid)
    {}

    Expected& operator=(const Expected& rhs)
    {
        value = rhs.value;
        error = rhs.error;
        valid = rhs._valid;
        return *this;
    }

    operator bool() const noexcept
    {
        return valid;
    }

    ValueType value;
    ErrorType error;
    bool valid = false;
};

template<typename ValueType, typename ErrorType>
static inline Expected<ValueType, ErrorType> success(ValueType val, ErrorType err = (ErrorType) 0)
{
    return Expected<ValueType, ErrorType>(val, err, true);
}

template<typename ValueType, typename ErrorType>
static inline Expected<ValueType, ErrorType> unexpected(ErrorType err, ValueType val = (ValueType) 0)
{
    return Expected<ValueType, ErrorType>(val, err, false);
}