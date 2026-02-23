#include "editor.h"
#include "terminal.h"
#include <unistd.h>
#include <iostream>
#include <system_error>

int main() {
    EditorConfig E;

    try {
        RawMode raw;
        E.initEditor();
        E.refreshScreen();
        while (E.processKeypress()) {
            E.refreshScreen();
        }
    } catch (const std::system_error& e) {
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
