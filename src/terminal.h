#pragma once
#include <termios.h>

// Manages terminal raw mode (RAII)
class RawMode {
public:
    RawMode();
    ~RawMode();

    RawMode(const RawMode&) = delete;
    RawMode& operator=(const RawMode&) = delete;

private:
    termios orig_{};
    bool enabled_ = false;
};
