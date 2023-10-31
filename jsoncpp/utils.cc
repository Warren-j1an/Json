#include <cmath>
#include <cstdio>
#include "except.h"
#include "utils.h"

namespace JsonCPP {
static void uintToString(uint64_t value, char*& current) {
    *--current = 0;
    do {
        *--current = static_cast<char>(value % 10U + static_cast<unsigned>('0'));
        value /= 10;
    } while (value != 0);
}

std::string valueToString(int64_t value) {
    char buffer[3 * 64 / 8 + 1];
    char* current = buffer + sizeof(buffer);
    if (value == Value::minInt64) {
        uintToString(uint64_t(Value::maxInt64) + 1, current);
        *--current = '-';
    } else if (value < 0) {
        uintToString(uint64_t(-value), current);
        *--current = '-';
    } else {
        uintToString(uint64_t(value), current);
    }
    JSON_ASSERT(current >= buffer);
    return current;
}

std::string valueToString(uint64_t value) {
    char buffer[3 * 64 / 8 + 1];
    char* current = buffer + sizeof(buffer);
    uintToString(value, current);
    JSON_ASSERT(current >= buffer);
    return current;
}

std::string valueToString(int32_t value) {
    return valueToString(int64_t(value));
}

std::string valueToString(uint32_t value) {
    return valueToString(uint64_t(value));
}

std::string valueToString(bool value) {
    return value ? "true" : "false";
}

std::string valueToString(double value, unsigned int precision, PrecisionType precisionType) {
    return valueToString(value, false, precision, precisionType);
}

std::string valueToString(double value, bool useSpecialFloats,
    unsigned int precision, PrecisionType precisionType) {
    // Print into the buffer. We need not request the alternative representation
    // that always has a decimal point because JSON doesn't distinguish the
    // concepts of reals and integers.
    if (!std::isfinite(value)) {
        static const char* const reps[2][3] = {{"NaN", "-Infinity", "Infinity"},
                                               {"null", "-1e+9999", "1e+9999"}};
        return reps[useSpecialFloats ? 0 : 1][std::isnan(value) ? 0 : (value < 0) ? 1 : 2];
    }
    std::string buffer(size_t(36), '\0');
    while (true) {
        int len = std::snprintf(&*buffer.begin(), buffer.size(),
            (precisionType == PrecisionType::significantDigits) ? "%.*g" : "%.*f",
            precision, value);
        JSON_ASSERT(len >= 0);
        size_t wouldPrint = static_cast<size_t>(len);
        if (wouldPrint >= buffer.size()) {
            buffer.resize(wouldPrint + 1);
            continue;
        }
        buffer.resize(wouldPrint);
        break;
    }
    buffer.erase(fixNumericLocale(buffer.begin(), buffer.end()), buffer.end());
    // try to ensure we preserve the fact that this was given to us as a double on input
    if (buffer.find('.') == buffer.npos && buffer.find('e') == buffer.npos) {
        buffer += ".0";
    }

    // strip the zero padding from the right
    if (precisionType == PrecisionType::decimalPlaces) {
        buffer.erase(fixZerosInTheEnd(buffer.begin(), buffer.end(), precision),
                    buffer.end());
    }

    return buffer;
}
}