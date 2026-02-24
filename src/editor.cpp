#include "editor.h"
#include "terminal.h"
#include "defines.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <ctime>
#include <optional>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <cerrno>
#include <system_error>
#include <fstream>
#include <string>
#include <string_view>

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

EditorConfig::EditorConfig() {
    cx = 0;
    cy = 0;
    rx = 0;
    rowoff = 0;
    coloff = 0;
    screenrows = 0;
    screencols = 0;
    dirty = 0;
    statusmsg[0] = '\0';
    statusmsg_time = 0;

    if (getWindowSize(this, &screenrows, &screencols) == -1)
        throw std::system_error(errno, std::generic_category(), "getWindowSize");

    screenrows -= 2;  // reserve rows for status bar and message bar
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
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

/*** Row operations ***/

int EditorConfig::rowCxToRx(const EditorRow& row, int cx) {
    int rx_val = 0;
    for (int j = 0; j < cx && j < (int)row.chars.size(); ++j) {
        if (row.chars[j] == '\t')
            rx_val += (RAFI_TAB_STOP - 1) - (rx_val % RAFI_TAB_STOP);
        ++rx_val;
    }
    return rx_val;
}

int EditorConfig::rowRxToCx(const EditorRow& row, int rx) {
    int cur_rx = 0;
    int cx;
    for (cx = 0; cx < (int)row.chars.size(); cx++) {
        if (row.chars[cx] == '\t')
            cur_rx += (RAFI_TAB_STOP - 1) - (cur_rx % RAFI_TAB_STOP);
        cur_rx++;
        if (cur_rx > rx) return cx;
    }
    return cx;
}

void EditorConfig::updateRow(EditorRow& row) {
    row.render.clear();
    for (char c : row.chars) {
        if (c == '\t') {
            row.render += ' ';
            while ((int)row.render.size() % RAFI_TAB_STOP != 0)
                row.render += ' ';
        } else {
            row.render += c;
        }
    }
}

void EditorConfig::insertRow(int at, std::string_view s) {
    if (at < 0 || at > (int)rows.size()) return;
    rows.insert(rows.begin() + at, EditorRow{});
    rows[at].chars = std::string(s);
    updateRow(rows[at]);
    dirty++;
}

void EditorConfig::rowInsertChar(EditorRow& row, int at, int c) {
    if (at < 0 || at > (int)row.chars.size()) at = (int)row.chars.size();
    row.chars.insert(at, 1, static_cast<char>(c));
    updateRow(row);
    dirty++;
}

void EditorConfig::rowDelChar(EditorRow& row, int at) {
    if (at < 0 || at >= (int)row.chars.size()) return;
    row.chars.erase(at, 1);
    updateRow(row);
    dirty++;
}

void EditorConfig::rowAppendString(EditorRow& row, std::string_view s) {
    row.chars += s;
    updateRow(row);
    dirty++;
}

void EditorConfig::delRow(int at) {
    if (at < 0 || at >= (int)rows.size()) return;
    rows.erase(rows.begin() + at);
    dirty++;
}

/*** File I/O ***/

void EditorConfig::editorOpen(const std::string& fname) {
    filename = fname;

    std::ifstream file(fname);
    if (!file.is_open())
        throw std::system_error(errno, std::generic_category(), "editorOpen: " + fname);

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        insertRow((int)rows.size(), line);
    }
    dirty = 0;
}

std::string EditorConfig::rowsToString() {
    std::string result;
    for (const auto& row : rows) {
        result += row.chars;
        result += '\n';
    }
    return result;
}

void EditorConfig::save() {
    if (filename.empty()) {
        auto name = prompt("Save as: %s (ESC to cancel)");
        if (!name) {
            setStatusMessage("Save aborted");
            return;
        }
        filename = *name;
    }

    std::string buf = rowsToString();
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        if (ftruncate(fd, static_cast<off_t>(buf.size())) != -1) {
            if (write(fd, buf.data(), buf.size()) == (ssize_t)buf.size()) {
                close(fd);
                dirty = 0;
                setStatusMessage("%d bytes written to disk", (int)buf.size());
                return;
            }
        }
        close(fd);
    }
    setStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

/*** Find ***/

void EditorConfig::findCallback(const std::string& query, int key) {
    static int last_match = -1;
    static int direction = 1;

    if (key == '\r' || key == '\x1b') {
        last_match = -1;
        direction = 1;
        return;
    } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1;
    } else if (key == ARROW_LEFT || key == ARROW_UP) {
        direction = -1;
    } else {
        last_match = -1;
        direction = 1;
    }

    if (last_match == -1) direction = 1;
    int current = last_match;

    for (int i = 0; i < (int)rows.size(); i++) {
        current += direction;
        if (current == -1) current = (int)rows.size() - 1;
        else if (current == (int)rows.size()) current = 0;

        const EditorRow& row = rows[current];
        auto pos = row.render.find(query);
        if (pos != std::string::npos) {
            last_match = current;
            cy = current;
            cx = rowRxToCx(row, (int)pos);
            rowoff = (int)rows.size();
            break;
        }
    }
}

void EditorConfig::find() {
    int saved_cx = cx;
    int saved_cy = cy;
    int saved_coloff = coloff;
    int saved_rowoff = rowoff;

    auto result = prompt("Search: %s (Use ESC/Arrows/Enter)",
        [this](const std::string& query, int key) {
            findCallback(query, key);
        });

    if (!result) {
        cx = saved_cx;
        cy = saved_cy;
        coloff = saved_coloff;
        rowoff = saved_rowoff;
    }
}

/*** Editor operations ***/

void EditorConfig::insertChar(int c) {
    if (cy == (int)rows.size()) {
        insertRow((int)rows.size(), "");
    }
    rowInsertChar(rows[cy], cx, c);
    cx++;
}

void EditorConfig::delChar() {
    if (cy == (int)rows.size()) return;
    if (cx == 0 && cy == 0) return;

    if (cx > 0) {
        rowDelChar(rows[cy], cx - 1);
        cx--;
    } else {
        cx = (int)rows[cy - 1].chars.size();
        rowAppendString(rows[cy - 1], rows[cy].chars);
        delRow(cy);
        cy--;
    }
}

void EditorConfig::insertNewline() {
    if (cx == 0) {
        insertRow(cy, "");
    } else {
        // Save right-hand portion before modifying the vector
        std::string right = rows[cy].chars.substr(cx);
        insertRow(cy + 1, right);
        // Truncate current row (vector may have reallocated above)
        rows[cy].chars.resize(cx);
        updateRow(rows[cy]);
    }
    cy++;
    cx = 0;
}

/*** Input ***/

int EditorConfig::readKey() {
    char c;
    int nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            throw std::system_error(errno, std::generic_category(), "read");
    }

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

std::optional<std::string> EditorConfig::prompt(const std::string& promptStr,
        std::function<void(const std::string&, int)> callback) {
    std::string buf;
    while (true) {
        setStatusMessage(promptStr.c_str(), buf.c_str());
        refreshScreen();
        int c = readKey();

        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (!buf.empty()) buf.pop_back();
        } else if (c == '\x1b') {
            setStatusMessage("");
            if (callback) callback(buf, c);
            return std::nullopt;
        } else if (c == '\r') {
            if (!buf.empty()) {
                setStatusMessage("");
                if (callback) callback(buf, c);
                return buf;
            }
        } else if (!std::iscntrl(c) && c < 128) {
            buf += static_cast<char>(c);
        }
        if (callback) callback(buf, c);
    }
}

void EditorConfig::MoveCursor(int key) {
    const EditorRow* row = (cy >= (int)rows.size()) ? nullptr : &rows[cy];

    switch (key) {
        case ARROW_LEFT:
            if (cx != 0) {
                cx--;
            } else if (cy > 0) {
                cy--;
                cx = (int)rows[cy].chars.size();
            }
            break;
        case ARROW_RIGHT:
            if (row && cx < (int)row->chars.size()) {
                cx++;
            } else if (row && cx == (int)row->chars.size()) {
                cy++;
                cx = 0;
            }
            break;
        case ARROW_UP:
            if (cy != 0) cy--;
            break;
        case ARROW_DOWN:
            if (cy < (int)rows.size()) cy++;
            break;
    }

    // Snap cursor to end of line if past it
    row = (cy >= (int)rows.size()) ? nullptr : &rows[cy];
    int rowlen = row ? (int)row->chars.size() : 0;
    if (cx > rowlen) cx = rowlen;
}

bool EditorConfig::processKeypress() {
    static int quit_times = RAFI_QUIT_TIMES;
    int c = readKey();

    switch (c) {
        case '\r':
            insertNewline();
            break;

        case CTRL_KEY('q'):
            if (dirty && quit_times > 0) {
                setStatusMessage("WARNING!!! File has unsaved changes. "
                    "Press Ctrl-Q %d more times to quit.", quit_times);
                quit_times--;
                return true;
            }
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            return false;

        case CTRL_KEY('s'):
            save();
            break;

        case CTRL_KEY('f'):
            find();
            break;

        case HOME_KEY:
            cx = 0;
            break;

        case END_KEY:
            if (cy < (int)rows.size())
                cx = (int)rows[cy].chars.size();
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) MoveCursor(ARROW_RIGHT);
            delChar();
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                if (c == PAGE_UP) {
                    cy = rowoff;
                } else {
                    cy = rowoff + screenrows - 1;
                    if (cy > (int)rows.size()) cy = (int)rows.size();
                }
                int times = screenrows;
                while (times--)
                    MoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            MoveCursor(c);
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            break;

        default:
            insertChar(c);
            break;
    }

    quit_times = RAFI_QUIT_TIMES;
    return true;
}

/*** Output ***/

void EditorConfig::scroll() {
    rx = 0;
    if (cy < (int)rows.size()) {
        rx = rowCxToRx(rows[cy], cx);
    }

    if (cy < rowoff) rowoff = cy;
    if (cy >= rowoff + screenrows) rowoff = cy - screenrows + 1;
    if (rx < coloff) coloff = rx;
    if (rx >= coloff + screencols) coloff = rx - screencols + 1;
}

void EditorConfig::drawRows(AppendBuffer& ab) {
    for (int y = 0; y < screenrows; ++y) {
        int filerow = y + rowoff;
        if (filerow >= (int)rows.size()) {
            if (rows.empty() && y == screenrows / 3) {
                char welcome[80];
                int welcomelen = std::snprintf(welcome, sizeof(welcome),
                    "Rafi Text -- Light Terminal Based Editor");
                if (welcomelen > screencols) welcomelen = screencols;
                int padding = (screencols - welcomelen) / 2;
                if (padding) {
                    ab.append("~");
                    --padding;
                }
                while (padding--) ab.append(" ");
                ab.append(std::string_view(welcome, welcomelen));
            } else {
                ab.append("~");
            }
        } else {
            int len = (int)rows[filerow].render.size() - coloff;
            if (len < 0) len = 0;
            if (len > screencols) len = screencols;
            ab.append(std::string_view(rows[filerow].render.data() + coloff, len));
        }

        ab.append("\x1b[K");
        ab.append("\r\n");
    }
}

void EditorConfig::drawStatusBar(AppendBuffer& ab) {
    ab.append("\x1b[7m");

    char status[80], rstatus[80];
    int len = std::snprintf(status, sizeof(status), "%.20s - %d lines %s",
        filename.empty() ? "[No Name]" : filename.c_str(), (int)rows.size(),
        dirty ? "(modified)" : "");
    int rlen = std::snprintf(rstatus, sizeof(rstatus), "%d/%d",
        cy + 1, (int)rows.size());

    if (len > screencols) len = screencols;
    ab.append(std::string_view(status, len));

    while (len < screencols) {
        if (screencols - len == rlen) {
            ab.append(std::string_view(rstatus, rlen));
            break;
        } else {
            ab.append(" ");
            ++len;
        }
    }

    ab.append("\x1b[m");
    ab.append("\r\n");
}

void EditorConfig::drawMessageBar(AppendBuffer& ab) {
    ab.append("\x1b[K");
    int msglen = (int)std::strlen(statusmsg);
    if (msglen > screencols) msglen = screencols;
    if (msglen && time(nullptr) - statusmsg_time < 5)
        ab.append(std::string_view(statusmsg, msglen));
}

void EditorConfig::refreshScreen() {
    scroll();

    AppendBuffer ab;
    ab.append("\x1b[?25l");
    ab.append("\x1b[H");

    drawRows(ab);
    drawStatusBar(ab);
    drawMessageBar(ab);

    char buf[32];
    std::snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
        (cy - rowoff) + 1, (rx - coloff) + 1);
    ab.append(buf);

    ab.append("\x1b[?25h");

    const std::string& out = ab.str();
    write(STDOUT_FILENO, out.data(), out.size());
}

void EditorConfig::setStatusMessage(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(statusmsg, sizeof(statusmsg), fmt, ap);
    va_end(ap);
    statusmsg_time = time(nullptr);
}
