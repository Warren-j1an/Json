#include "except.h"

namespace JsonCPP {
Exception::Exception(const std::string& msg) : m_msg(msg) {}

Exception::~Exception() noexcept = default;

char const* Exception::what() const noexcept {
    return m_msg.c_str();
}
void throwRuntimeError(const std::string& msg) {
    throw RuntimeError(msg);
}

void throwLogicError(const std::string& msg) {
    throw LogicError(msg);
}
}