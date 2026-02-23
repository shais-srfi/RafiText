#pragma once

#include <string>
#include <string_view>

// Append buffer: accumulates output into a single string for one write() call,
// avoiding the flicker caused by many small write() calls.
class AppendBuffer {
public:
    AppendBuffer() = default;

    void append(std::string_view s);
    const std::string& str() const;

private:
    std::string buf_;
};
