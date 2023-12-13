<div align="center">
  <img src="./resources/Icon.png" height="100" />
  <h1>ZoomMe</h1>
</div>

Application for zooming and noting the desktop (like ZoomIt under windows).

You can...
- Draw lines, rectangles, arrows and ellipses.
- Insert text.
- Zoom in and out of the desktop.

Made with Qt6 (it works with Qt5 too. There's an [older branch for that](https://github.com/Ezee1015/zoomme/tree/Qt5)) and OpenGL.

![Demonstration of the functionality](resources/demonstration.gif)
> The dots in the GIF are artifacts from ffpemg

## Functions

### General
|    Key    | Function                                                   |
|:---------:|------------------------------------------------------------|
|  **`Q`**  | To clear all the drawings                                  |
|  **`U`**  | To delete the last drawing of the current draw mode (undo) |
| **`ESC`** | To finish writing text, restore zoom or quit/exit          |

### Change color

|   Key   | Function                   |
|:-------:|----------------------------|
| **`R`** | To change color to Red     |
| **`G`** | To change color to Green   |
| **`B`** | To change color to Blue    |
| **`C`** | To change color to Cyan    |
| **`M`** | To change color to Magenta |
| **`Y`** | To change color to Yellow  |
| **`W`** | To change color to White   |
| **`O`** | To change color to Orange  |

### Change size
|   Key   | Function                        |
|:-------:|---------------------------------|
| **`1`** | To change the width to 1 pixel  |
| **`2`** | To change the width to 2 pixels |
| **`3`** | To change the width to 3 pixels |
| **`4`** | To change the width to 4 pixels |
| **`5`** | To change the width to 5 pixels |
| **`6`** | To change the width to 6 pixels |
| **`7`** | To change the width to 7 pixels |
| **`8`** | To change the width to 8 pixels |
| **`9`** | To change the width to 9 pixels |

### Modes
|        Key        | Function                                              |
|:-----------------:|-------------------------------------------------------|
|      **`Z`**      | To change the mode to drawing Lines (mode by default) |
|      **`X`**      | To change the mode to drawing Rectangles              |
|      **`A`**      | To change the mode to drawing Arrows                  |
|      **`E`**      | To change the mode to drawing Ellipses                |
|      **`T`**      | To change the mode to insert Text                     |
|    **`Enter`**    | To finish writing text                                |
| **`Shift+Enter`** | To make a new line when writing text                  |

## Dependencies
- `build-essential`
- `qt6`

## Instalation

### Dependencies
- Debian-based: `sudo apt install build-essential qt6-base`
- Arch-based: `sudo pacman -S base-devel qt6-base`

### Compilation
```bash
qmake -makefile zoomme.pro
make
```

### Running
```bash
./zoomme
```

## Credit
- Magnifying glass icon: Research or Magnifying Glass Flat Icon Vector.svg from Wikimedia Commons by Videoplasty.com, CC-BY-SA 4.0
- Pencil: The "Pencil" icon used by MobiText from Wikimedia Commons by Gavinstubbs09, CC-BY-SA-3.0
