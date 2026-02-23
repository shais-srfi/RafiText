#include "abuf.h"

void AppendBuffer::append(std::string_view s) {
    buf_.append(s);
}

const std::string& AppendBuffer::str() const {
    return buf_;
}
