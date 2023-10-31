#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace JsonCPP {
/// @brief Type of the value held by a Value object.
enum ValueType {
    nullValue = 0,  // "null" value
    intValue,       // signed integer value
    uintValue,      // unsigned integer value
    realValue,      // double value
    stringValue,    // UTF-8 string value
    boolValue,      // bool value
    arrayValue,     // array value
    objectValue     // object value (collection of name/value pairs)
};

enum CommentPlacement {
    commentBefore = 0,      // a comment placed on the line before a value
    commentAfterOnSameLine, // a comment just after a value on the same line
    commentAfter,           // a comment on the line after a value (only make sense for root value)
    numberOfCommentPlacement
};

/// @brief Type of precision for formatting of real values.
enum PrecisionType {
    significantDigits = 0,  // we set max number of significant digits in string
    decimalPlaces           // we set max number of digits after "." in string
};

class StaticString {
public:
    explicit StaticString(const char* string) : m_str(string) {}

    const char* operator()() const { return m_str; }
    const char* c_str() const { return m_str; }

private:
    const char* m_str;
};

class ValueConstIterator;
class ValueIterator;

class Value {
public:
    static Value const& nullSingleton();
    static constexpr int64_t minInt64 = int64_t(~(uint64_t(-1) / 2));
    static constexpr int64_t maxInt64 = int64_t(uint64_t(-1) / 2);
    static constexpr int64_t maxUInt64 = uint64_t(-1);

    static constexpr int32_t minInt = int32_t(~(uint32_t(-1) / 2));
    static constexpr int32_t maxInt = int32_t(uint32_t(-1) / 2);
    static constexpr int32_t maxUInt = uint32_t(-1);

    static constexpr uint32_t defaultRealPrecision = 17;
    static constexpr double maxUInt64AsDouble = 18446744073709551615.0;

private:
    class CZString {
    public:
        enum DuplicationPolicy { noDuplication = 0, duplicate, duplicateOnCopy };
        CZString(uint32_t index);
        CZString(char const* str, unsigned int length, DuplicationPolicy allocate);
        CZString(CZString const& other);
        CZString(CZString&& other) noexcept;
        ~CZString();
        CZString& operator=(const CZString& other);
        CZString& operator=(CZString&& other) noexcept;
        bool operator<(CZString const& other) const;
        bool operator==(CZString const& other) const;
        uint32_t index() const;
        char const* data() const;
        unsigned int length() const;
        bool isStaticString() const;

    private:
        void swap(CZString& other);

        struct StringStorage {
            unsigned int policy : 2;
            unsigned int length : 30; // 1GB max
        };

        char const* m_cstr; // actually, a prefixed string, unless policy is noDup
        union {
            uint32_t m_index;
            StringStorage m_storage;
        };
    };

public:
    typedef std::map<CZString, Value> ObjectValues;

    Value(ValueType type = nullValue);
    Value(int32_t value);
    Value(uint32_t value);
    Value(int64_t value);
    Value(uint64_t value);
    Value(double value);
    Value(const char* value); // Copy til first 0. (NULL causes to seg-fault.)
    Value(const char* begin, const char* end); // Copy all, includ zeroes.
    Value(const StaticString& value);
    Value(const std::string& value);
    Value(bool value);
    Value(const Value& other);
    Value(Value&& other) noexcept;
    ~Value();

    Value& operator=(const Value& other);
    Value& operator=(Value&& other) noexcept;

    /// @brief Swap everything.
    void swap(Value& other);
    /// @brief Swap values but leave comments and source offsets in place.
    void swapPayload(Value& other);
    /// @brief copy everything.
    void copy(const Value& other);
    /// @brief copy values but leave comments and source offsets in place.
    void copyPayload(const Value& other);

    ValueType type() const;

    /// Compare payload only, not comments etc.
    bool operator<(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>=(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;
    int compare(const Value& other) const;

    const char* asCString() const; // Embedded zeroes could cause you trouble!
    std::string asString() const; // Embedded zeroes are possible.

    unsigned getCStringLength() const;
    bool getString(const char** begin, const char** end) const;

    int32_t asInt() const;
    uint32_t asUInt() const;
    int64_t asInt64() const;
    uint64_t asUInt64() const;
    int64_t asLargestInt() const;
    uint64_t asLargestUInt() const;
    float asFloat() const;
    double asDouble() const;
    bool asBool() const;
    bool isNull() const;
    bool isBool() const;
    bool isInt() const;
    bool isInt64() const;
    bool isUInt() const;
    bool isUInt64() const;
    bool isIntegral() const;
    bool isDouble() const;
    bool isNumeric() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    bool isConvertibleTo(ValueType other) const;
    uint32_t size() const;
    bool empty() const;
    explicit operator bool() const;
    void clear();
    void resize(uint32_t newSize);
    Value& operator[](uint32_t index);
    Value& operator[](int index);
    const Value& operator[](uint32_t index) const;
    const Value& operator[](int index) const;
    Value get(uint32_t index, const Value& defaultValue) const;
    bool isValidIndex(uint32_t index) const;
    Value& append(const Value& value);
    Value& append(Value&& value);
    bool insert(uint32_t index, const Value& newValue);
    bool insert(uint32_t index, Value&& newValue);
    Value& operator[](const char* key);
    const Value& operator[](const char* key) const;
    Value& operator[](const std::string& key);
    const Value& operator[](const std::string& key) const;
    Value& operator[](const StaticString& key);
    Value get(const char* key, const Value& defaultValue) const;
    Value get(const char* begin, const char* end, const Value& defaultValue) const;
    Value get(const std::string& key, const Value& defaultValue) const;
    Value const* find(char const* begin, char const* end) const;
    Value* demand(char const* begin, char const* end);
    void removeMember(const char* key);
    void removeMember(const std::string& key);
    bool removeMember(const char* key, Value* removed);
    bool removeMember(std::string const& key, Value* removed);
    bool removeMember(const char* begin, const char* end, Value* removed);
    bool removeIndex(uint32_t index, Value* removed);
    bool isMember(const char* key) const;
    bool isMember(const std::string& key) const;
    bool isMember(const char* begin, const char* end) const;
    std::vector<std::string> getMemberNames() const;

    void setComment(std::string comment, CommentPlacement placement);
    void setComment(const char* comment, size_t len, CommentPlacement placement) {
        setComment(std::string(comment, len), placement);
    }

    bool hasComment(CommentPlacement placement) const;
    std::string getComment(CommentPlacement placement) const;
    std::string toStyledString() const;
    ValueConstIterator begin() const;
    ValueConstIterator end() const;
    ValueIterator begin();
    ValueIterator end();
    const Value& front() const;
    Value& front();
    const Value& back() const;
    Value& back();
    void setOffsetStart(ptrdiff_t start);
    void setOffsetLimit(ptrdiff_t limit);
    ptrdiff_t getOffsetStart() const;
    ptrdiff_t getOffsetLimit() const;

private:
    void setType(ValueType v) {
        m_bits.value_type = static_cast<unsigned char>(v);
    }
    bool isAllocated() const { return m_bits.allocated; }
    void setIsAllocated(bool v) { m_bits.allocated = v; }

    void initBasic(ValueType type, bool allocated = false);
    void dupPayload(const Value& other);
    void releasePayload();
    void dupMeta(const Value& other);

    Value& resolveReference(const char* key);
    Value& resolveReference(const char* key, const char* end);

    union ValueHolder {
        int64_t v_int;
        uint64_t v_uint;
        double v_real;
        bool v_bool;
        char* v_string; // if allocated_, ptr to { unsigned, char[] }.
        ObjectValues* v_map;
    } m_value;

    struct {
        // Really a ValueType, but types should agree for bitfield packing.
        unsigned int value_type : 8;
        // Unless allocated_, string_ must be null-terminated.
        unsigned int allocated : 1;
    } m_bits;

    class Comments {
    public:
        Comments() = default;
        Comments(const Comments& that);
        Comments(Comments&& that) noexcept;
        Comments& operator=(const Comments& that);
        Comments& operator=(Comments&& that) noexcept;
        bool has(CommentPlacement slot) const;
        std::string get(CommentPlacement slot) const;
        void set(CommentPlacement slot, std::string comment);

    private:
        using Array = std::array<std::string, numberOfCommentPlacement>;
        std::unique_ptr<Array> m_ptr;
    };

    Comments m_comments;
    // [start, limit) byte offsets in the source JSON text from which this Value was extracted.
    ptrdiff_t m_start;
    ptrdiff_t m_limit;
};

class PathArgument {
friend class Path;
public:
    PathArgument();
    PathArgument(uint32_t index);
    PathArgument(const char* key);
    PathArgument(std::string key);

private:
    enum Kind { kindNone = 0, kindIndex, kindKey };
    std::string m_key;
    uint32_t m_index{};
    Kind kind_{kindNone};
};

class Path {
public:
    Path(const std::string& path, const PathArgument& a1 = PathArgument(),
         const PathArgument& a2 = PathArgument(),
         const PathArgument& a3 = PathArgument(),
         const PathArgument& a4 = PathArgument(),
         const PathArgument& a5 = PathArgument());

    const Value& resolve(const Value& root) const;
    Value resolve(const Value& root, const Value& defaultValue) const;
    /// Creates the "path" to access the specified node and returns a reference on
    /// the node.
    Value& make(Value& root) const;

private:
    using InArgs = std::vector<const PathArgument*>;
    using Args = std::vector<PathArgument>;

    void makePath(const std::string& path, const InArgs& in);
    void addPathInArg(const std::string& path, const InArgs& in,
                        InArgs::const_iterator& itInArg, PathArgument::Kind kind);
    static void invalidPath(const std::string& path, int location);

    Args m_args;
};

class ValueIteratorBase {
public:
    ValueIteratorBase();
    explicit ValueIteratorBase(const Value::ObjectValues::iterator& current);

    bool operator==(const ValueIteratorBase& other) const { return isEqual(other); }
    bool operator!=(const ValueIteratorBase& other) const { return !isEqual(other); }
    int operator-(const ValueIteratorBase& other) const {
        return other.computeDistance(*this);
    }

    Value key() const;
    uint32_t index() const;
    std::string name() const;
    char const* memberName(char const** end) const;

protected:
    const Value& deref() const;
    Value& deref();
    void increment();
    void decrement();
    int computeDistance(const ValueIteratorBase& other) const;
    bool isEqual(const ValueIteratorBase& other) const;
    void copy(const ValueIteratorBase& other);

private:
    Value::ObjectValues::iterator m_current;
    bool m_isNull{true};
};

class ValueConstIterator : public ValueIteratorBase {
public:
    ValueConstIterator();
    ValueConstIterator(ValueIterator const& other);
    ValueConstIterator& operator=(const ValueIteratorBase& other);
    ValueConstIterator operator++(int) {
        ValueConstIterator temp(*this);
        ++*this;
        return temp;
    }

    ValueConstIterator operator--(int) {
        ValueConstIterator temp(*this);
        --*this;
        return temp;
    }

    ValueConstIterator& operator--() {
        decrement();
        return *this;
    }

    ValueConstIterator& operator++() {
        increment();
        return *this;
    }

    const Value& operator*() const { return deref(); }
    const Value* operator->() const { return &deref(); }

private:
    explicit ValueConstIterator(const Value::ObjectValues::iterator& current);
};

class ValueIterator : public ValueIteratorBase {
public:
    ValueIterator();
    explicit ValueIterator(const ValueConstIterator& other);
    ValueIterator(const ValueIterator& other);
    ValueIterator& operator=(const ValueIterator& other);

    ValueIterator operator++(int) {
        ValueIterator temp(*this);
        ++*this;
        return temp;
    }

    ValueIterator operator--(int) {
        ValueIterator temp(*this);
        --*this;
        return temp;
    }

    ValueIterator& operator--() {
        decrement();
        return *this;
    }

    ValueIterator& operator++() {
        increment();
        return *this;
    }

    /*! The return value of non-const iterators can be changed, so the these functions are not
    *  const because the returned references/pointers can be used to change state of the base
    *  class.
    */
    Value& operator*() const { return const_cast<Value&>(deref()); }
    Value* operator->() const { return const_cast<Value*>(&deref()); }

private:
    explicit ValueIterator(const Value::ObjectValues::iterator& current);
};

inline void swap(Value& a, Value& b) { a.swap(b); }

inline const Value& Value::front() const { return *begin(); }

inline Value& Value::front() { return *begin(); }

inline const Value& Value::back() const { return *(--end()); }

inline Value& Value::back() { return *(--end()); }
}