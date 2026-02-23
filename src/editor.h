#pragma once
#include "abuf.h"

class EditorConfig {
public:
    int screenrows = 0;
    int screencols = 0;

    void initEditor();
    int getCursorPosition(int *rows, int *cols);
    bool processKeypress();
    void refreshScreen();

private:
    void MoveCursor(int key);
    int readKey();
    static int getWindowSize(EditorConfig* editor, int* rows, int* cols);
    void drawRows(AppendBuffer& ab);
    int cx = 0;
    int cy = 0;
};
