#include "terminal.h"
#include <unistd.h>
#include <cerrno>
#include <cctype>
#include <iostream>
#include <system_error>

int main() {
    try {
        RawMode raw;

        while (1) {
            char c = '\0';
            if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
                throw std::system_error(errno, std::generic_category(), "read");
            if (std::iscntrl(c)) {
                std::cout << (int)c << "\r\n";
            } else {
                std::cout << (int)c << " ('" << c << "')\r\n";
            }
            if (c == 'q') break;
        }
    } catch (const std::system_error& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}