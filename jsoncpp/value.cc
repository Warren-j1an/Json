#include <cmath>
#include <cstddef>
#include <cstring>
#include "except.h"
#include "utils.h"
#include "value.h"

namespace JsonCPP {
template <typename T>
static std::unique_ptr<T> cloneUnique(const std::unique_ptr<T>& p) {
  std::unique_ptr<T> r;
  if (p) {
    r = std::unique_ptr<T>(new T(*p));
  }
  return r;
}

Value const& Value::nullSingleton() {
    static const Value nullStatic;
    return nullStatic;
}

static inline double integerToDouble(uint64_t value) {
    return static_cast<double>(int64_t(value / 2)) * 2.0 + static_cast<double>(int64_t(value & 1));
}

template <typename T>
static inline double integerToDouble(T value) {
    return static_cast<double>(value);
}

template <typename T, typename U>
static inline bool InRange(double d, T min, U max) {
    return d >= integerToDouble(min) && d <= integerToDouble(max);
}

static bool IsIntegral(double d) {
    double integral_part;
    return modf(d, &integral_part) == 0.0;
}

static inline char* duplicateStringValue(const char* value, size_t length) {
    if (length >= static_cast<size_t>(Value::maxInt)) {
        length = Value::maxInt - 1;
    }

    char* newString = new char[length + 1];
    if (newString == nullptr) {
        throwRuntimeError("in Json::Value::duplicateStringValue(): "
                          "Failed to allocate string value buffer");
    }
    memcpy(newString, value, length);
    newString[length] = 0;
    return newString;
}

static inline char* duplicateAndPrefixStringValue(const char* value, unsigned int length) {
    JSON_ASSERT_MESSAGE(length <= static_cast<unsigned>(Value::maxInt) - sizeof(unsigned) - 1U,
        "in Json::Value::duplicateAndPrefixStringValue(): length too big for prefixing");
    size_t actualLength = sizeof(length) + length + 1;
    char* newString = new char[actualLength];
    if (newString == nullptr) {
        throwRuntimeError("in Json::Value::duplicateAndPrefixStringValue(): "
                          "Failed to allocate string value buffer");
    }
    *reinterpret_cast<unsigned*>(newString) = length;
    memcpy(newString + sizeof(unsigned), value, length);
    newString[actualLength - 1U] = 0; // to avoid buffer over-run accidents by users later
    return newString;
}

inline static void decodePrefixedString(bool isPrefixed, const char* prefixed,
                                        unsigned* length, const char** value) {
    if (!isPrefixed) {
        *length = static_cast<unsigned>(strlen(prefixed));
        *value = prefixed;
    } else {
        *length = *reinterpret_cast<const unsigned*>(prefixed);
        *value = prefixed + sizeof(unsigned);
    }
}

Value::CZString::CZString(uint32_t index) : m_cstr(nullptr), m_index(index) {}

Value::CZString::CZString(char const* str, unsigned int length, DuplicationPolicy allocate)
    : m_cstr(str) {
    m_storage.policy = allocate & 0x3;
    m_storage.length = length & 0x3fffffff;
}

Value::CZString::CZString(CZString const& other) {
    if (other.m_storage.policy != noDuplication && other.m_cstr != nullptr) {
        m_cstr = duplicateStringValue(other.m_cstr, other.m_storage.length);
    } else {
        m_cstr = other.m_cstr;
    }
    if (other.m_cstr) {
        if (static_cast<DuplicationPolicy>(other.m_storage.policy) == noDuplication) {
            m_storage.policy = static_cast<unsigned int>(noDuplication) & 3U;
        } else {
            m_storage.policy = static_cast<unsigned int>(duplicate) & 3U;
        }
    } else {
        m_storage.policy = static_cast<unsigned int>(other.m_storage.policy) & 3U;
    }
    m_storage.length = other.m_storage.length;
}

Value::CZString::CZString(CZString&& other) noexcept
    : m_cstr(other.m_cstr), m_index(other.m_index) {
    other.m_cstr = nullptr;
}

Value::CZString::~CZString() {
    if (m_cstr && m_storage.policy == duplicate) {
        delete[] m_cstr;
        m_cstr = nullptr;
    }
}

void Value::CZString::swap(CZString& other) {
    std::swap(m_cstr, other.m_cstr);
    std::swap(m_index, other.m_index);
}

Value::CZString& Value::CZString::operator=(const Value::CZString& other) {
    m_cstr = other.m_cstr;
    m_index = other.m_index;
    return *this;
}

Value::CZString& Value::CZString::operator=(Value::CZString&& other) noexcept {
    m_cstr = other.m_cstr;
    m_index = other.m_index;
    other.m_cstr = nullptr;
    return *this;
}

bool Value::CZString::operator<(Value::CZString const& other) const {
    if (m_cstr == nullptr) {
        return m_index < other.m_index;
    }
    unsigned min_len = std::min<unsigned>(m_storage.length, other.m_storage.length);
    JSON_ASSERT(m_cstr && other.m_cstr);
    int ret = memcmp(m_cstr, other.m_cstr, min_len);
    if (ret < 0) {
        return true;
    } else {
        return false;
    }
    return m_storage.length < other.m_storage.length;
}

bool Value::CZString::operator==(Value::CZString const& other) const {
    if (m_cstr == nullptr) {
        return m_index == other.m_index;
    }
    if (m_storage.length != other.m_storage.length) {
        return false;
    }
    JSON_ASSERT(m_cstr && other.m_cstr);
    int ret = memcmp(m_cstr, other.m_cstr, m_storage.length);
    return ret == 0;
}

uint32_t Value::CZString::index() const {
    return m_index;
}

char const* Value::CZString::data() const {
    return m_cstr;
}
unsigned int Value::CZString::length() const {
    return m_storage.length;
}

bool Value::CZString::isStaticString() const {
    return m_storage.policy == noDuplication;
}

Value::Value(ValueType type) {
    static const char emptyString[] = "";
    initBasic(type);
    switch (type) {
    case nullValue:
        break;

    case intValue:
    case uintValue:
        m_value.v_int = 0;
        break;

    case realValue:
        m_value.v_real = 0.0;
        break;

    case stringValue:
        m_value.v_string = const_cast<char*>(static_cast<const char*>(emptyString));
        break;

    case arrayValue:
    case objectValue:
        m_value.v_map = new ObjectValues();
        break;

    case boolValue:
        m_value.v_bool = false;
        break;

    default:
        JSON_ASSERT_MESSAGE(false, "Value::Value(ValueType) unreachable");
    }
}

Value::Value(int32_t value) {
    initBasic(intValue);
    m_value.v_int = value;
}

Value::Value(uint32_t value) {
    initBasic(uintValue);
    m_value.v_uint = value;
}

Value::Value(int64_t value) {
    initBasic(intValue);
    m_value.v_int = value;
}

Value::Value(uint64_t value) {
    initBasic(uintValue);
    m_value.v_uint = value;
}

Value::Value(double value) {
    initBasic(realValue);
    m_value.v_real = value;
}

Value::Value(const char* value) {
    initBasic(stringValue, true);
    JSON_ASSERT_MESSAGE(value != nullptr, "Null Value Passed to Value Constructor");
    m_value.v_string = duplicateAndPrefixStringValue(value, static_cast<unsigned>(strlen(value)));
}

Value::Value(const char* begin, const char* end) {
    initBasic(stringValue, true);
    m_value.v_string = duplicateAndPrefixStringValue(begin, static_cast<unsigned>(end - begin));
}

Value::Value(const StaticString& value) {
    initBasic(stringValue);
    m_value.v_string = const_cast<char*>(value.c_str());
}

Value::Value(const std::string& value) {
    initBasic(stringValue, true);
    m_value.v_string = duplicateAndPrefixStringValue(value.data(),
        static_cast<unsigned>(value.length()));
}

Value::Value(bool value) {
    initBasic(boolValue);
    m_value.v_bool = value;
}

Value::Value(const Value& other) {
    dupPayload(other);
    dupMeta(other);
}

Value::Value(Value&& other) noexcept {
    initBasic(nullValue);
    swap(other);
}

Value::~Value() {
    releasePayload();
    m_value.v_uint = 0;
}

Value& Value::operator=(const Value& other) {
    Value(other).swap(*this);
    return *this;
}

Value& Value::operator=(Value&& other) noexcept {
    other.swap(*this);
    return *this;
}

void Value::swapPayload(Value& other) {
    std::swap(m_bits, other.m_bits);
    std::swap(m_value, other.m_value);
}

void Value::swap(Value& other) {
    swapPayload(other);
    std::swap(m_comments, other.m_comments);
    std::swap(m_start, other.m_start);
    std::swap(m_limit, other.m_limit);
}

void Value::copyPayload(const Value& other) {
    releasePayload();
    dupPayload(other);
}

void Value::copy(const Value& other) {
    copyPayload(other);
    dupMeta(other);
}

ValueType Value::type() const {
    return static_cast<ValueType>(m_bits.value_type);
}

bool Value::operator<(const Value& other) const {
    int typeDelta = type() - other.type();
    if (typeDelta) {
        return typeDelta < 0;
    }
    switch (type()) {
    case nullValue:
        return false;

    case intValue:
        return m_value.v_int < other.m_value.v_int;

    case uintValue:
        return m_value.v_uint < other.m_value.v_uint;

    case realValue:
        return m_value.v_real < other.m_value.v_real;

    case boolValue:
        return m_value.v_bool < other.m_value.v_bool;

    case stringValue: {
        if ((m_value.v_string == nullptr) || (other.m_value.v_string == nullptr)) {
            return other.m_value.v_string != nullptr;
        }
        unsigned this_len;
        unsigned other_len;
        char const* this_str;
        char const* other_str;
        decodePrefixedString(isAllocated(), m_value.v_string, &this_len, &this_str);
        decodePrefixedString(other.isAllocated(), other.m_value.v_string, &other_len, &other_str);
        unsigned min_len = std::min(this_len, other_len);
        JSON_ASSERT(this_str && other_str);
        int comp = memcmp(this_str, other_str, min_len);
        if (comp < 0) {
            return true;
        }
        if (comp > 0) {
            return false;
        }
        return this_len < other_len;
        }

    case arrayValue:
    case objectValue: {
        size_t this_size = m_value.v_map->size();
        size_t other_size = other.m_value.v_map->size();
        if (this_size != other_size) {
            return this_size < other_size;
        }
        return *m_value.v_map < *other.m_value.v_map;
        }

    default:
        JSON_ASSERT_MESSAGE(false, "Value::operator<(const Value& other) const unreachable");
    }
    return false;
}

bool Value::operator<=(const Value& other) const {
    return !(other < *this);
}

bool Value::operator>=(const Value& other) const {
    return !(*this < other);
}

bool Value::operator>(const Value& other) const {
    return other < *this;
}

bool Value::operator==(const Value& other) const {
    if (type() != other.type()) {
        return false;
    }
    switch (type()) {
    case nullValue:
        return true;

    case intValue:
        return m_value.v_int < other.m_value.v_int;

    case uintValue:
        return m_value.v_uint < other.m_value.v_uint;

    case realValue:
        return m_value.v_real < other.m_value.v_real;

    case boolValue:
        return m_value.v_bool < other.m_value.v_bool;

    case stringValue: {
        if ((m_value.v_string == nullptr) || (other.m_value.v_string == nullptr)) {
            return other.m_value.v_string != nullptr;
        }
        unsigned this_len;
        unsigned other_len;
        char const* this_str;
        char const* other_str;
        decodePrefixedString(isAllocated(), m_value.v_string, &this_len, &this_str);
        decodePrefixedString(other.isAllocated(), other.m_value.v_string, &other_len, &other_str);
        unsigned min_len = std::min(this_len, other_len);
        JSON_ASSERT(this_str && other_str);
        int comp = memcmp(this_str, other_str, min_len);
        if (comp < 0) {
            return true;
        }
        if (comp > 0) {
            return false;
        }
        return this_len < other_len;
        }

    case arrayValue:
    case objectValue: {
        size_t this_size = m_value.v_map->size();
        size_t other_size = other.m_value.v_map->size();
        if (this_size != other_size) {
            return this_size < other_size;
        }
        return *m_value.v_map < *other.m_value.v_map;
        }

    default:
        JSON_ASSERT_MESSAGE(false, "Value::operator<(const Value& other) const unreachable");
    }
    return false;
}

bool Value::operator!=(const Value& other) const {
    return !(*this == other);
}

int Value::compare(const Value& other) const {
    if (*this < other) {
        return -1;
    } else if (*this > other) {
        return 1;
    } else {
        return 0;
    }
}

const char* Value::asCString() const {
    JSON_ASSERT_MESSAGE(type() == stringValue,
        "in Json::Value::asCString(): requires stringValue");
    if (m_value.v_string == nullptr) {
        return nullptr;
    }
    unsigned this_len;
    const char* this_str;
    decodePrefixedString(isAllocated(), m_value.v_string, &this_len, &this_str);
    return this_str;
}

std::string Value::asString() const {
    switch (type()) {
    case nullValue:
        return "";

    case stringValue: {
        if (m_value.v_string == nullptr) {
            return nullptr;
        }
        unsigned this_len;
        const char* this_str;
        decodePrefixedString(isAllocated(), m_value.v_string, &this_len, &this_str);
        return std::string(this_str, this_len);
        }

    case boolValue:
        return m_value.v_bool ? "true" : "false";

    case intValue:
        return valueToString(m_value.v_int);

    case uintValue:
        return valueToString(m_value.v_uint);

    case realValue:
        return valueToString(m_value.v_real);

    default:
        JSON_ASSERT_MESSAGE(false, "Type is not convertible to string");
    }
}

unsigned Value::getCStringLength() const {
    JSON_ASSERT_MESSAGE(type() == stringValue,
        "in Json::Value::asCString(): requires stringValue");
    if (m_value.v_string == nullptr) {
        return 0;
    }
    unsigned this_len;
    const char* this_str;
    decodePrefixedString(isAllocated(), m_value.v_string, &this_len, &this_str);
    return this_len;
}

bool Value::getString(const char** begin, const char** end) const {
    if (type() != stringValue || m_value.v_string == nullptr) {
        return false;
    }
    unsigned length;
    decodePrefixedString(isAllocated(), m_value.v_string, &length, begin);
    *end = *begin + length;
    return true;
}

int32_t Value::asInt() const {
    switch (type()) {
    case intValue:
        JSON_ASSERT_MESSAGE(isInt(), "LargestInt out of Int range");
        return int32_t(m_value.v_int);

    case uintValue:
        JSON_ASSERT_MESSAGE(isInt(), "LargestInt out of Int range");
        return int32_t(m_value.v_uint);

    case realValue:
        JSON_ASSERT_MESSAGE(InRange(m_value.v_real, minInt, maxInt), "double out of Int range");
        return int32_t(m_value.v_real);

    case nullValue:
        return 0;

    case boolValue:
        return m_value.v_bool ? 1 : 0;

    default:
        JSON_ASSERT_MESSAGE(false, "Type is not convertible to Int");
    }
}

uint32_t Value::asUInt() const {
    switch (type()) {
    case intValue:
        JSON_ASSERT_MESSAGE(isUInt(), "LargestInt out of UInt range");
        return uint32_t(m_value.v_int);

    case uintValue:
        JSON_ASSERT_MESSAGE(isUInt(), "LargestInt out of UInt range");
        return uint32_t(m_value.v_uint);

    case realValue:
        JSON_ASSERT_MESSAGE(InRange(m_value.v_real, 0, maxUInt), "double out of Int range");
        return uint32_t(m_value.v_real);

    case nullValue:
        return 0;

    case boolValue:
        return m_value.v_bool ? 1 : 0;

    default:
        JSON_ASSERT_MESSAGE(false, "Type is not convertible to UInt");
    }
}

int64_t Value::asInt64() const {
    switch (type()) {
    case intValue:
        return int64_t(m_value.v_int);

    case uintValue:
        JSON_ASSERT_MESSAGE(isInt64(), "LargestInt out of Int64 range");
        return int64_t(m_value.v_uint);

    case realValue:
        JSON_ASSERT_MESSAGE(InRange(m_value.v_real, minInt64, maxInt64),
                            "double out of Int range");
        return int64_t(m_value.v_real);

    case nullValue:
        return 0;

    case boolValue:
        return m_value.v_bool ? 1 : 0;

    default:
        JSON_ASSERT_MESSAGE(false, "Type is not convertible to Int64");
    }
}

uint64_t Value::asUInt64() const {
    switch (type()) {
    case intValue:
        JSON_ASSERT_MESSAGE(isUInt64(), "LargestInt out of UInt64 range");
        return uint64_t(m_value.v_int);

    case uintValue:
        return uint64_t(m_value.v_uint);

    case realValue:
        JSON_ASSERT_MESSAGE(InRange(m_value.v_real, 0, maxUInt64), "double out of Int range");
        return uint64_t(m_value.v_real);

    case nullValue:
        return 0;

    case boolValue:
        return m_value.v_bool ? 1 : 0;

    default:
        JSON_ASSERT_MESSAGE(false, "Type is not convertible to UInt64");
    }
}

int64_t Value::asLargestInt() const {
    return asInt64();
}

uint64_t Value::asLargestUInt() const {
    return asUInt64();
}

float Value::asFloat() const {
    switch (type()) {
    case intValue:
        return static_cast<float>(m_value.v_int);

    case uintValue:
        return static_cast<float>(integerToDouble(m_value.v_uint));

    case realValue:
        return static_cast<float>(m_value.v_real);

    case nullValue:
        return 0.0;

    case boolValue:
        return m_value.v_bool ? 1.0f : 0.0f;

    default:
        JSON_ASSERT_MESSAGE(false, "Type is not convertible to Float");
    }
}

double Value::asDouble() const {
    switch (type()) {
    case intValue:
        return static_cast<double>(m_value.v_int);

    case uintValue:
        return static_cast<float>(integerToDouble(m_value.v_uint));

    case realValue:
        return m_value.v_real;

    case nullValue:
        return 0.0;

    case boolValue:
        return m_value.v_bool ? 1.0 : 0.0;

    default:
        JSON_ASSERT_MESSAGE(false, "Type is not convertible to Double");
    }
}

bool Value::asBool() const {
    switch (type()) {
    case intValue:
        return m_value.v_int != 0;

    case uintValue:
        return m_value.v_uint != 0;

    case realValue:
        const int value_classification = std::fpclassify(m_value.v_real);
        return value_classification != FP_ZERO && value_classification != FP_NAN;

    case nullValue:
        return false;

    case boolValue:
        return m_value.v_bool;

    default:
        JSON_ASSERT_MESSAGE(false, "Type is not convertible to Bool");
    }
}

bool Value::isNull() const {
    return type() == nullValue;
}

bool Value::isBool() const {
    return type() == boolValue;
}

bool Value::isInt() const {
    switch (type()) {
    case intValue:
        return m_value.v_int >= minInt && m_value.v_int <= maxInt;

    case uintValue:
        return m_value.v_uint <= uint32_t(maxInt);

    case realValue:
        return m_value.v_real >= minInt && m_value.v_real <= maxInt && IsIntegral(m_value.v_real);

    default:
        return false;
    }
}

bool Value::isInt64() const {
    switch (type()) {
    case intValue:
        return true;

    case uintValue:
        return m_value.v_int <= uint64_t(maxInt64);

    case realValue:
        // Note that maxInt64 (= 2^63 - 1) is not exactly representable as a
        // double, so double(maxInt64) will be rounded up to 2^63. Therefore we
        // require the value to be strictly less than the limit.
        return m_value.v_real >= double(minInt64) && m_value.v_real < double(maxInt64) &&
            IsIntegral(m_value.v_real);

    default:
        return false;
    }
}

bool Value::isUInt() const {
    switch (type()) {
    case intValue:
        return m_value.v_int >= 0 && uint64_t(m_value.v_int) <= uint64_t(maxUInt);

    case uintValue:
        return m_value.v_uint <= maxUInt;

    case realValue:
        return m_value.v_real >= 0 && m_value.v_real <= maxUInt && IsIntegral(m_value.v_real);

    default:
        return false;
    }
}

bool Value::isUInt64() const {
    switch (type()) {
    case intValue:
        return m_value.v_int >= 0;

    case uintValue:
        return true;

    case realValue:
        // Note that maxUInt64 (= 2^64 - 1) is not exactly representable as a
        // double, so double(maxUInt64) will be rounded up to 2^64. Therefore we
        // require the value to be strictly less than the limit.
        return m_value.v_real >= 0 && m_value.v_real <= maxUInt64AsDouble &&
            IsIntegral(m_value.v_real);

    default:
        return false;
    }
}

bool Value::isIntegral() const {
    switch (type()) {
    case intValue:
    case uintValue:
        return true;

    case realValue:
        // Note that maxUInt64 (= 2^64 - 1) is not exactly representable as a
        // double, so double(maxUInt64) will be rounded up to 2^64. Therefore we
        // require the value to be strictly less than the limit.
        return m_value.v_real >= 0 && m_value.v_real <= maxUInt64AsDouble &&
            IsIntegral(m_value.v_real);

    default:
        return false;
    }
}

bool Value::isDouble() const {
    return type() == intValue || type() == uintValue || type() == realValue;
}

bool Value::isNumeric() const {
    return isDouble();
}

bool Value::isString() const {
    return type() == stringValue;
}

bool Value::isArray() const {
    return type() == arrayValue;
}

bool Value::isObject() const {
    return type() == objectValue;
}

bool Value::isConvertibleTo(ValueType other) const {
    switch (other) {
    case nullValue:
        return (isNumeric() && asDouble() == 0.0) ||
            (type() == boolValue && !m_value.v_bool) ||
            (type() == stringValue && asString().empty()) ||
            (type() == arrayValue && m_value.v_map->empty()) ||
            (type() == objectValue && m_value.v_map->empty()) ||
            type() == nullValue;

    case intValue:
        return isInt() ||
            (type() == realValue && InRange(m_value.v_real, minInt, maxInt)) ||
            type() == boolValue || type() == nullValue;

    case uintValue:
        return isUInt() ||
            (type() == realValue && InRange(m_value.v_real, 0, maxUInt)) ||
            type() == boolValue || type() == nullValue;

    case realValue:
        return isNumeric() || type() == boolValue || type() == nullValue;

    case boolValue:
        return isNumeric() || type() == boolValue || type() == nullValue;

    case stringValue:
        return isNumeric() || type() == boolValue || type() == stringValue || type() == nullValue;

    case arrayValue:
        return type() == arrayValue || type() == nullValue;

    case objectValue:
        return type() == objectValue || type() == nullValue;

    default:
        JSON_ASSERT(false);
        return false;
    }
}

uint32_t Value::size() const {
    switch (type()) {
    case nullValue:
    case intValue:
    case uintValue:
    case realValue:
    case boolValue:
    case stringValue:
        return 0;

    case arrayValue: // size of the array is highest index + 1
        if (!m_value.v_map->empty()) {
            ObjectValues::const_iterator itLast = m_value.v_map->end();
            --itLast;
            return (*itLast).first.index() + 1;
        }
        return 0;

    case objectValue:
        return uint32_t(m_value.v_map->size());

    default:
        JSON_ASSERT(false);
        return 0;
    }
}

bool Value::empty() const {
    if (isNull() || isArray() || isObject()) {
        return size() == 0U;
    }
    return false;
}

Value::operator bool() const { return !isNull(); }



Value::Comments::Comments(const Comments& that) : m_ptr(cloneUnique(that.m_ptr)) {}

Value::Comments::Comments(Comments&& that) noexcept : m_ptr(std::move(that.m_ptr)) {}

Value::Comments& Value::Comments::operator=(const Value::Comments& that) {
    m_ptr = cloneUnique(that.m_ptr);
    return *this;
}

Value::Comments& Value::Comments::operator=(Value::Comments&& that) noexcept {
    m_ptr = std::move(that.m_ptr);
    return *this;
}

bool Value::Comments::has(CommentPlacement slot) const {
    return m_ptr && !(*m_ptr)[slot].empty();
}

std::string Value::Comments::get(CommentPlacement slot) const {
    if (!m_ptr) {
        return {};
    }
    return (*m_ptr)[slot];
}

void Value::Comments::set(CommentPlacement slot, std::string comment) {
    if (slot >= CommentPlacement::numberOfCommentPlacement) {
        return;
    }
    if (!m_ptr) {
        m_ptr = std::unique_ptr<Array>(new Array());
    }
    (*m_ptr)[slot] = std::move(comment);
}


}
