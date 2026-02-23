#pragma once
#include <termios.h>

// class to manage terminal raw mode
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