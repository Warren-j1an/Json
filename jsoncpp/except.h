#pragma once

#include <exception>
#include <sstream>
#include <string>

namespace JsonCPP {
class Exception : public std::exception {
public:
    Exception(const std::string& msg);
    ~Exception() noexcept override;
    const char* what() const noexcept override;

private:
    std::string m_msg;
};

class RuntimeError : public Exception {
public:
    RuntimeError(const std::string& msg) : Exception(msg) {}
};

class LogicError : public Exception {
public:
    LogicError(const std::string& msg) : Exception(msg) {}
};

void throwRuntimeError(const std::string& msg);
void throwLogicError(const std::string& msg);
}

#define JSON_ASSERT(condition)                          \
    if (!(condition)) {                                 \
        JsonCPP::throwLogicError("assert json failed"); \
        abort();                                        \
    }

#define JSON_ASSERT_MESSAGE(condition, message) \
    if (!(condition)) {                         \
        std::stringstream ss;                   \
        ss << message;                          \
        JsonCPP::throwLogicError(ss.str());     \
        abort();                                \
    }
