#pragma once
#include "abuf.h"
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>
#include <ctime>
#include <cstdarg>

class EditorConfig {
public:
    EditorConfig();

    int getCursorPosition(int *rows, int *cols);
    bool processKeypress();
    void refreshScreen();
    void editorOpen(const std::string& filename);
    void setStatusMessage(const char* fmt, ...);

private:
    struct EditorRow {
        std::string chars;
        std::string render;
    };

    /*** Row operations ***/
    void insertRow(int at, std::string_view s);
    void updateRow(EditorRow& row);
    int rowCxToRx(const EditorRow& row, int cx);
    int rowRxToCx(const EditorRow& row, int rx);
    void rowInsertChar(EditorRow& row, int at, int c);
    void rowDelChar(EditorRow& row, int at);
    void rowAppendString(EditorRow& row, std::string_view s);
    void delRow(int at);

    /*** Editor operations ***/
    void insertChar(int c);
    void delChar();
    void insertNewline();

    /*** File I/O ***/
    std::string rowsToString();
    void save();

    /*** Find ***/
    void findCallback(const std::string& query, int key);
    void find();

    /*** Input ***/
    std::optional<std::string> prompt(const std::string& promptStr,
        std::function<void(const std::string&, int)> callback = nullptr);
    void scroll();
    void MoveCursor(int key);
    int readKey();
    static int getWindowSize(EditorConfig* editor, int* rows, int* cols);

    /*** Output ***/
    void drawRows(AppendBuffer& ab);
    void drawStatusBar(AppendBuffer& ab);
    void drawMessageBar(AppendBuffer& ab);

    int cx;
    int cy;
    int rx;
    int rowoff;
    int coloff;
    int screenrows;
    int screencols;
    int dirty;
    std::vector<EditorRow> rows;
    std::string filename;
    char statusmsg[80];
    time_t statusmsg_time;
};
