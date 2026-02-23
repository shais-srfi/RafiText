#include "terminal.h"
#include <unistd.h>
#include <cerrno>
#include <system_error>

// Enable raw mode on construction
RawMode::RawMode() {
    if (tcgetattr(STDIN_FILENO, &orig_) == -1)
        throw std::system_error(errno, std::generic_category(), "tcgetattr");

    termios raw = orig_;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        throw std::system_error(errno, std::generic_category(), "tcsetattr");

    enabled_ = true;
}

// Restore original terminal settings on destruction
RawMode::~RawMode() {
    if (enabled_) tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_);
}