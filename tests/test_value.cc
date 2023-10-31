#include <cassert>
#include <iostream>
#include <string>

static int64_t minLargestInt = int64_t(~(uint64_t(-1) / 2));
static int64_t maxLargestInt = int64_t(uint64_t(-1) / 2);

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
    if (value == minLargestInt) {
        uintToString(uint64_t(maxLargestInt) + 1, current);
        *--current = '-';
    } else if (value < 0) {
        uintToString(uint64_t(-value), current);
        *--current = '-';
    } else {
        uintToString(uint64_t(value), current);
    }
    assert(current >= buffer);
    return current;
}

int main() {
    int64_t a = 99999;
    std::cout << valueToString(a) << std::endl;
    return 0;
}