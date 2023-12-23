<div align="center">
  <img src="./resources/Icon.png" height="100" />
  <h1>ZoomMe</h1>
</div>

Application for zooming/magnifying and noting the desktop (like ZoomIt under windows).

You can...
- Draw lines, rectangles, arrows, ellipses and hand draws (free forms)!
- Insert text
- Highlight parts of the screen with colors (a highlighter)
- Use a "Flashlight" function to concentrate attention on the cursor
- Zoom in and out of the desktop.
- Switch between your desktop and a blackboard
- Take a screenshot and save it to your Pictures folder

Made with Qt6 (it works with Qt5 too. There's an [older branch for that](https://github.com/Ezee1015/zoomme/tree/Qt5)) and OpenGL.

### Demonstration
![Demonstration of the functionality](resources/demonstration.gif)

> *The idea of the Flashlight effect was taken from [tsoding/boomer](https://github.com/tsoding/boomer)*

## Functions & Mappings

### General mappings
|         Key/Event         | Function                                                                                                                                                                                                                                                                                            |
|:-------------------------:|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|          **`S`**          | To take a Screenshot in the current position and Save it to the standard Pictures folder (or the current path if it isn't found)                                                                                                                                                                    |
|          **`U`**          | To delete the last drawing of the current draw mode (undo)                                                                                                                                                                                                                                          |
|          **`,`**          | To delete a drawing (from the current mode) with the mouse. You can exit by deleting a form, with the Escape key or pressing ',' again                                                                                                                                                            |
|        **`Shift`**        | When it's zoomed in, if you press and hold the key, the screen will not follow the mouse, so you can freely move it around that part of the screen (for drawing, for example). When you release it, it will continue following the mouse                                                            |
|     **`Mouse Wheel`**     | To zoom in and zoom out                                                                                                                                                                                                                                                                             |
|      **`.`** (period)     | To toggle the "Flashlight effect" on the cursor.                                                                                                                                                                                                                                                    |
| **`Shift + Mouse Wheel`** | When the shadow/flashlight effect is turn on, this will increase/decrease the size of the "light"                                                                                                                                                                                                   |
|          **`Q`**          | To clear all the drawings                                                                                                                                                                                                                                                                           |
|          **`P`**          | To toggle between your screen and a blackboard mode. By the way, your drawings will persist between the modes                                                                                                                                                                                       |
|         **`ESC`**         | To finish writing text (if you're writing), restore zoom (if you are zoomed in), exit the "deleting a drawing" mode (if it's on), disable the mouse shadow/flashlight (if it's on) or quit/exit (If you are not zoomed in, not writing text, not deleting a drawing, nor in shadow/flashlight mode) |

### Available Modes
|              Key             | Function                                                                                                                                                  |
|:----------------------------:|-----------------------------------------------------------------------------------------------------------------------------------------------------------|
|            **`Z`**           | To change the mode to drawing Lines (mode by default)                                                                                                     |
|            **`X`**           | To change the mode to drawing Rectangles                                                                                                                  |
|            **`A`**           | To change the mode to drawing Arrows                                                                                                                      |
|            **`E`**           | To change the mode to drawing Ellipses                                                                                                                    |
|            **`F`**           | To change the mode to drawing Free forms / Hand draws                                                                                                     |
|            **`T`**           | To change the mode to insert Text                                                                                                                         |
|            **`H`**           | To change the mode to a Highlighter (rectangular)                                                                                                         |
|          **`Enter`**         | To finish writing text                                                                                                                                    |
|       **`Shift+Enter`**      | To make a new line when writing text                                                                                                                      |
|     **`Left/Right keys`**    | To go to the previous/next character when writing text                                                                                                    |
|      **`Up/Down keys`**      | To go to the previous/next line break when writing text                                                                                                   |
| **`Shift and click a text`** | You can access and modify the text by clicking on it when you're in insert text mode. However, you cannot be writing or drawing a new text while clicking |

### Change the color of the lines

|   Key   | Function                   |
|:-------:|----------------------------|
| **`R`** | To change color to Red     |
| **`G`** | To change color to Green   |
| **`B`** | To change color to Blue    |
| **`C`** | To change color to Cyan    |
| **`M`** | To change color to Magenta |
| **`Y`** | To change color to Yellow  |
| **`O`** | To change color to Orange  |
| **`W`** | To change color to White   |
| **`D`** | To change color to Black   |

### Change the size of the lines
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

## Dependencies
- `build-essential`
- `qt6`
- `libopengl-dev`

## Instalation

### Dependencies
- Debian-based: `sudo apt install build-essential qt6-base-dev libqt6opengl6-dev libqt5opengl5-dev`
- Arch-based: `sudo pacman -S base-devel qt6-base`

### Compilation
```bash
qmake6 -makefile zoomme.pro
make
```

### Running
- Running the application as normal:
```bash
./zoomme
```

- Use an image as the background (instead of the desktop):
```bash
./zoomme -i path/to/image
```

- ***Experimental*** -- Use a transparent background. No zooming allowed, only drawing
```bash
./zoomme -l
```

- Show the help message:
```bash
./zoomme -h
```

## Credit
- Magnifying glass icon: Research or Magnifying Glass Flat Icon Vector.svg from Wikimedia Commons by Videoplasty.com, CC-BY-SA 4.0
- Pencil: The "Pencil" icon used by MobiText from Wikimedia Commons by Gavinstubbs09, CC-BY-SA-3.0
