#include "editor.h"
#include "terminal.h"
#include <unistd.h>
#include <iostream>
#include <system_error>

int main(int argc, char *argv[]) {
    EditorConfig E;

    try {
        RawMode raw;
        if (argc >= 2) {
            E.editorOpen(argv[1]);
        }
        E.setStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");
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
