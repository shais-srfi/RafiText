#include "editor.h"
#include "terminal.h"
#include "defines.h"
#include <cstdio>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <system_error>

/*** Init ***/

int EditorConfig::getWindowSize(EditorConfig* editor, int* rows, int* cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return editor->getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void EditorConfig::initEditor() {
    if (getWindowSize(this, &screenrows, &screencols) == -1)
        throw std::system_error(errno, std::generic_category(), "getWindowSize");
}

int EditorConfig::getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        ++i;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;  // fixed: ';' not ':'

    return 0;
}

/*** Input ***/

int EditorConfig::readKey() {
    char c = '\0';
    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
        throw std::system_error(errno, std::generic_category(), "read");

    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    } else {
        return c;
    }
}

void EditorConfig::MoveCursor(int key) {
    switch (key) {
        case ARROW_LEFT:
            if (cx != 0) cx--;
            break;
        case ARROW_RIGHT:
            if (cx != screencols - 1) cx++;
            break;
        case ARROW_UP:
            if (cy != 0) cy--;
            break;
        case ARROW_DOWN:
            if (cy != screenrows - 1) cy++;
            break;
    }
}

bool EditorConfig::processKeypress() {
    int c = readKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            return false;

        case HOME_KEY:
            cx = 0;
            return true;
        case END_KEY:
            cx = screencols - 1;
            return true;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = screenrows;
                while (times--)
                MoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            return true;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case ARROW_LEFT:
            MoveCursor(c);
            return true;
    }

    return true;
}

/*** Output ***/

void EditorConfig::drawRows(AppendBuffer& ab) {
    for (int y = 0; y < screenrows; ++y) {
        if (y == screenrows/3) {
            char welcome[80];
            int welcomelen = std::snprintf(welcome, sizeof(welcome), "Rafi Text -- Light Terminal Based Editor");
            if (welcomelen > screencols) welcomelen = screencols;
            int padding = (screencols - welcomelen)/2;
            if (padding) {
                ab.append("~");
                --padding;
            }
            while (padding--) ab.append(" ");
            ab.append(std::string_view(welcome, welcomelen));
        } else {
            ab.append("~");
        }

        ab.append("\x1b[K");
        if (y < screenrows - 1) {
            ab.append("\r\n");
        }
    }
}

void EditorConfig::refreshScreen() {
    AppendBuffer ab;

    ab.append("\x1b[?25l");  // hide cursor
    ab.append("\x1b[H");     // move cursor to top-left

    drawRows(ab);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cy + 1, cx + 1);
    ab.append(buf);           // position cursor at cx, cy

    ab.append("\x1b[?25h");  // show cursor

    const std::string& out = ab.str();
    write(STDOUT_FILENO, out.data(), out.size());
}
