# ZoomMe
Application for zooming and noting desktop (like ZoomIt under windows).

## Functions
| Key                             | Function                          |
|---------------------------------|-----------------------------------|
| **`R` `G` `B` `C` `M` `Y` `W`** | To change color                   |
| **`1-9`**                       | To change width in pixels         |
| **`Z` `X`**                     | To change draw mode (line\| rect) |
| **`Q`**                         | To clear all                      |
| **`ESC`**                       | To quit press                     |

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
