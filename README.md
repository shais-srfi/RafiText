# RafiText

A small terminal text editor built in C++, based on the [kilo](https://viewsourcecode.org/snaptoken/kilo/) tutorial. Runs in the terminal on Linux and WSL2.

## Requirements

- Linux or WSL2 (Ubuntu)
- g++ with C++20 support
- cmake
- ninja (optional, can use the default make generator instead)

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

The binary is placed at `build/RafiText`.

## Usage

Open an existing file:
```bash
./build/RafiText path/to/file.txt
```

Start with a blank buffer (you'll be prompted for a filename on first save):
```bash
./build/RafiText
```

## Keybindings

### Navigation

| Key | Action |
|-----|--------|
| Arrow keys | Move cursor up / down / left / right |
| `Home` | Jump to start of line |
| `End` | Jump to end of line |
| `Page Up` | Scroll up one screen |
| `Page Down` | Scroll down one screen |

Cursor movement wraps between lines: pressing `←` at the start of a line moves to the end of the previous line, and `→` at the end of a line moves to the beginning of the next.

### Editing

| Key | Action |
|-----|--------|
| Any printable character | Insert character at cursor |
| `Enter` | Insert newline / split line |
| `Backspace` / `Ctrl-H` | Delete character to the left |
| `Delete` | Delete character to the right |

### File operations

| Key | Action |
|-----|--------|
| `Ctrl-S` | Save. Prompts for a filename if the buffer has never been saved. |
| `Ctrl-Q` | Quit. If there are unsaved changes, press `Ctrl-Q` three times to confirm. |

### Search

| Key | Action |
|-----|--------|
| `Ctrl-F` | Open search prompt |
| (while searching) Type | Filter incrementally as you type |
| (while searching) `↑` / `←` | Jump to previous match |
| (while searching) `↓` / `→` | Jump to next match |
| (while searching) `Enter` | Confirm and stay at current match |
| (while searching) `Escape` | Cancel and return cursor to where it was |

Search wraps around the end/beginning of the file.

## Status bar

The bottom of the screen shows two lines:

- **Status bar** (inverted colours): filename (or `[No Name]`), line count, and `(modified)` if there are unsaved changes. The current line / total lines is shown on the right.
- **Message bar**: hints and feedback messages (e.g. save confirmation, warnings, search prompt). Messages disappear after 5 seconds.

## Tab display

Tabs are rendered as spaces and aligned to the next multiple of 8 columns (standard tab stop width).

## Source layout

| File | Purpose |
|------|---------|
| `src/main.cpp` | Entry point, RAII raw-mode setup, main loop |
| `src/editor.h/cpp` | All editor state and logic (rows, input, output, find, file I/O) |
| `src/terminal.h/cpp` | RAII wrapper that enables/restores terminal raw mode |
| `src/abuf.h/cpp` | Append buffer — batches all screen writes into one `write()` call to avoid flicker |
| `src/defines.h` | Constants (`RAFI_TAB_STOP`, `RAFI_QUIT_TIMES`), `CTRL_KEY()` helper, key enum |
