#pragma once

constexpr int CTRL_KEY(char c) {
    return c & 0x1f;
}

inline constexpr const char* RAFI_VERSION = "0.0.1";
constexpr int RAFI_TAB_STOP = 8;
constexpr int RAFI_QUIT_TIMES = 3;

enum editorKey {
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};
