#pragma once

#include <cstddef>
#include <string>

namespace JsonCPP {
class Features {
public:
    static Features All();
    static Features StrictMode();

    Features();

private:
    bool m_allowComments;
    bool m_strictRoot;
    bool m_allowDroppedNullPlaceholders;
    bool m_allowNumericKeys;
};

class Reader {
public:
    struct StructedError {
        ptrdiff_t offset_start;
        ptrdiff_t offset_limit;
        std::string message;
    };

    Reader();
    Reader(const Features& features);
    // bool parse(const std::string& document, Value& root, bool collectComments = true);
};
}