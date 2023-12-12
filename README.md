# ZoomMe
Application for zooming and noting desktop (like ZoomIt under windows).

## Functions
|               Key               | Function                                                           |
|:-------------------------------:|--------------------------------------------------------------------|
| **`R` `G` `B` `C` `M` `Y` `W`** | To change color                                                    |
|            **`1-9`**            | To change width in pixels                                          |
|     **`Z` `X` `A` `E` `T`**     | To change draw mode (line\| rectangle \| arrow \| ellipse \| text) |
|             **`Q`**             | To clear all                                                       |
|             **`U`**             | To delete the last drawing of the current draw mode (undo)         |
|            **`ESC`**            | To quit, restore zoom or finish writing text                       |
|           **`Enter`**           | To finish writing text                                             |
|        **`Shift+Enter`**        | To make a new line when writing text                               |

## Dependencies
- `build-essential`
- `qt5-qmake`
- `qtbase5-dev`

## Instalation

### Dependencies
- Debian-based: `sudo apt install build-essential qt5-qmake qtbase5-dev`
- Arch-based: `sudo pacman -S base-devel qt5-base`

### Compilation
```bash
qmake -makefile zoomme.pro
make
```

### Running
```bash
./zoomme
```
