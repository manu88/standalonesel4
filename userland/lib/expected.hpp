#pragma once

template<typename ValueType, typename ErrorType>
struct Expected
{
    Expected(ValueType value, ErrorType error):
    value(value),
    error(error)
    {}

    ValueType value;
    ErrorType error;
};

template<typename ValueType, typename ErrorType>
static inline Expected<ValueType, ErrorType> success(ValueType val, ErrorType err = (ErrorType) 0)
{
    return Expected<ValueType, ErrorType>(val, err);
}

template<typename ValueType, typename ErrorType>
static inline Expected<ValueType, ErrorType> unexpected(ErrorType err, ValueType val = (ValueType) 0)
{
    return Expected<ValueType, ErrorType>(val, err);
}