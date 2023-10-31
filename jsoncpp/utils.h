#pragma once

#include <string>
#include "value.h"

namespace JsonCPP {
std::string valueToString(int64_t value);
std::string valueToString(uint64_t value);
std::string valueToString(int32_t value);
std::string valueToString(uint32_t value);
std::string valueToString(bool value);
std::string valueToString(double value, unsigned int precision = Value::defaultRealPrecision,
    PrecisionType precisionType = PrecisionType::significantDigits);
std::string valueToString(double value, bool useSpecialFloats,
    unsigned int precision, PrecisionType precisionType);

/**
 * Change ',' to '.' everywhere in buffer.
 */
template <typename Iter>
Iter fixNumericLocale(Iter begin, Iter end) {
    for (; begin != end; ++begin) {
        if (*begin == ',') {
            *begin = '.';
        }
    }
    return begin;
}

/**
 * Return iterator that would be the new end of the range [begin,end), if we
 * were to delete zeros in the end of string, but not the last zero before '.'.
 */
template <typename Iter>
Iter fixZerosInTheEnd(Iter begin, Iter end, unsigned int precision) {
    for (; begin != end; --end) {
        if (*(end - 1) != '0') {
            return end;
        }
        // Don't delete the last zero before the decimal point.
        if (begin != (end - 1) && begin != (end - 2) && *(end - 2) == '.') {
            if (precision) {
                return end;
            }
            return end - 2;
        }
    }
    return end;
}
}