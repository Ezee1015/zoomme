<div align="center">
  <img src="./resources/Icon.png" height="100" />
  <h1>ZoomMe</h1>
</div>

Application for zooming/magnifying and annotating the desktop (like ZoomIt under windows).

You can...
- Draw lines, rectangles, arrows, ellipses and hand draws (free forms)!
- Insert text
- Highlight parts of the screen with colors (a highlighter)
- Use a "Flashlight" function to concentrate attention on the cursor position
- Zoom in and out of the desktop.
- Switch between your desktop and a blackboard
- Save it to your Desktop folder
- Record what you do
- Save the current work in a `.zoomme` file and [recover from it later](#restore-from-file)

Made with Qt6 (it works with Qt5 too. There's an [older branch for that](https://github.com/Ezee1015/zoomme/tree/Qt5)) and OpenGL.

### Demonstration
![Demonstration of the functionality](resources/demonstration.gif)

> *The idea of the Flashlight effect was taken from [tsoding/boomer](https://github.com/tsoding/boomer)*

## Functions & Mappings

### General functions

|         Key/Event         | Function                                                                                                                                                                                                                                                                                                                                                                                                     |
|:-------------------------:|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|     **`Mouse Wheel`**     | To zoom in and zoom out                                                                                                                                                                                                                                                                                                                                                                                      |
|        **`Shift`**        | When it's zoomed in, if you press and hold the key, the screen will not follow the mouse, so you can freely move it around in that part of the screen (for drawing, for example). When you release it, it will continue following the mouse                                                                                                                                                                  |
|      **`.`** (period)     | To toggle the "Flashlight effect" on the cursor.                                                                                                                                                                                                                                                                                                                                                             |
| **`Shift + Mouse Wheel`** | **When the flashlight effect is turn on**, this will increase/decrease the size of the "light"                                                                                                                                                                                                                                                                                                               |
|          **`S`**          | Save the current work to an image, which will be stored in the Desktop folder (or the current path if not found). The computer will *beep* if the image was correctly saved                                                                                                                                                                                                                                  |
|      **`Shift + S`**      | Save the current work to the clipboard. The computer will *beep* once the mapping is pressed                                                                                                                                                                                                                                                                                                                 |
|      **`Shift + E`**      | Save the current work a '.zoomme' file, so you can later restore the state of the program. It is going to be save in the same path and with the same name that the image of the screenshot                                                                                                                                                                                                                   |
|       **`-`** (dash)      | Start/stop recording. A video will be produced, but it may take some time for rendering (You'll know when it finished rendering when the recording indicator of the status bar goes away and you listen a Beep). The video is going to be save in the same path and with the same name that the image of the screenshot. **Requirements**: have `ffmpeg` installed, and use a Unix based system              |
|         **`Tab`**         | To toggle between your screen and a blackboard mode. By the way, your drawings will persist between the modes                                                                                                                                                                                                                                                                                                |
|        **`Space`**        | To toggle the visibility of the elements of the screen: hide the status bar, hide all or show all                                                                                                                                                                                                                                                                                                            |
|         **`ESC`**         | To **finish writing text** (if you're writing), **exit the "delete a drawing" mode** (if it's on), **exit the "pick a color" mode**, **disable the 'hide all' mode** (if it's on), **disable the mouse flashlight** (if it's on), **restore zoom** (if you are zoomed in), **stop recording**, or **quit/exit** (If you are not zoomed in, not writing text, not deleting a drawing, nor in flashlight mode) |

### Available Drawing Modes

|            Key            | Function                                                                                                                               |
|:-------------------------:|----------------------------------------------------------------------------------------------------------------------------------------|
|          **`Z`**          | To change the mode to drawing Lines (mode by default)                                                                                  |
|          **`X`**          | To change the mode to drawing Rectangles                                                                                               |
|          **`A`**          | To change the mode to drawing Arrows                                                                                                   |
|          **`E`**          | To change the mode to drawing Ellipses                                                                                                 |
|          **`F`**          | To change the mode to drawing Free forms / Hand draws                                                                                  |
|          **`T`**          | To change the mode to insert Text                                                                                                      |
|          **`H`**          | To change the mode to a Highlighter (rectangular)                                                                                      |

#### Text Mode Mappings

|              Key             | Function                                                                                                                                                  |
|:----------------------------:|-----------------------------------------------------------------------------------------------------------------------------------------------------------|
|          **`Enter`**         | To finish writing text                                                                                                                                    |
|       **`Shift+Enter`**      | To make a new line when writing text                                                                                                                      |
|     **`Left/Right keys`**    | To go to the previous/next character when writing text                                                                                                    |
|      **`Up/Down keys`**      | To go to the previous/next line break when writing text                                                                                                   |
| **`Shift and click a text`** | You can access and modify the text by clicking on it when you're in insert text mode. However, you cannot be writing or drawing a new text while clicking |

### Available Actions for the drawing modes

|   Key   | Function                                                                                                                               |
|:-------:|----------------------------------------------------------------------------------------------------------------------------------------|
| **`U`** | To delete the last drawing of the current draw mode (undo)                                                                             |
| **`,`** | To delete a drawing (from the current mode) with the mouse. You can exit by deleting a form, with the Escape key or pressing `,` again |
| **`Q`** | To clear all the drawings                                                                                                              |


### Change the color of the lines

|   Key   | Function                        |
|:-------:|---------------------------------|
| **`R`** | To change color to Red          |
| **`G`** | To change color to Green        |
| **`B`** | To change color to Blue         |
| **`C`** | To change color to Cyan         |
| **`M`** | To change color to Magenta      |
| **`Y`** | To change color to Yellow       |
| **`O`** | To change color to Orange       |
| **`W`** | To change color to White        |
| **`D`** | To change color to Black        |
| **`P`** | To pick a color from the screen |

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

<!-- Start 1 -->
<details>
<summary><b>Running the application as normal</b></summary><p>

```bash
./zoomme
```

</p></details>
<!-- End 1 -->

<!-- Start 2 -->
<details>
<summary><b>Show the help message</b></summary><p>

```bash
./zoomme --help
```

</p></details>
<!-- End 2 -->

<!-- Start 3 -->
<details>
<summary><b>Set the path where is going to be saved the exported image (when pressing the 's' key)</b></summary><p>

> It can be an absolute or relative path

```bash
./zoomme -p ./path/to/folder
```

> By default, ZoomMe saves it to the `Desktop` folder

</p></details>
<!-- End 3 -->

<!-- Start 4 -->
<details>
<summary><b>Set the name of the exported image (when pressing the 's' key)</b></summary><p>

```bash
./zoomme -n name_of_the_image
```

> By default, the name will be: `Zoomme dd-mm-yyyy hh.mm.ss`
>
> The format of the date can be customized in the `zoomwidget.hpp` file

</p></details>
<!-- End 4 -->

<!-- Start 5 -->
<details>
<summary><b>Set the extension of the exported image (when pressing the 's' key)</b></summary><p>

```bash
./zoomme -e:i jpg
```

> By default, the extension will be: `png`

</p></details>
<!-- End 5 -->

<!-- Start 6 -->
<details>
<summary><b>Set the extension of the recorded video (when pressing the '-' key)</b></summary><p>

```bash
./zoomme -e:v mkv
```

> By default, the extension will be: `mp4`

</p></details>
<!-- End 6 -->

<!-- Start 7 -->
<details>
<summary><b>***Experimental*** -- Use a transparent background. No zooming allowed, only drawing</b></summary><p>

```bash
./zoomme -l
```

</p></details>
<!-- End 7 -->

<!-- Start 8 -->
<details>
<summary><b>Use an image as the background (instead of the desktop)</b></summary><p>

 Intended for previously saved images of the desktop from ZoomMe (so you can add more drawings after been saved), but you can use it with any image

```bash
./zoomme -i path/to/image
```

</p></details>
<!-- End 8 -->

<!-- Start 9 -->
<details>
<summary id="restore-from-file"><b>Restore the state of the program from a `.zoomme` file</b></summary><p>

Load/Restore the state of the program saved in that file. It should be a '.zoomme' file.

Ensure that you execute the file on the same monitor where it was previously run to avoid resolution issues, or at least, make sure that the two monitors have the same resolution.

```bash
./zoomme -r path/to/file.zoomme
```

</p></details>
<!-- End 9 -->

#### Additional arguments:
- You can force the image to fit the screen's width or hight with `-w` or `-h` after providing the image path, like this: `./zoomme -i path/to/image -w`, if you do not providing anything, it automatically detect the best option.

- You can overwrite the image provided when saving by doing this: `./zoomme -i path/to/image --replace-on-save`. This will autocomplete the `-p`, `-n` and `-e:i` arguments for you. How kind :)

</p></details>
<!-- End 7 -->

## :suspect: Secret :suspect:
> Shhh! Don't tell anybody... :zipper_mouth_face:
>
> You can customize certain features in the app :art: by editing the header file (`zoomwidget.hpp`). Within this file, there are macros that determine various program behaviors. You can modify the content of these macros located inside the "Customization" comment section. To apply the changes you've made, simply recompile the program.
>
> Some of the customizable elements include:
>
> - FPS of the recording
> - The date format when saving screenshots and recordings
> - Color codes for the color of the drawings
> - Icons in the status bar


## Credit
- Magnifying glass icon: Research or Magnifying Glass Flat Icon Vector.svg from Wikimedia Commons by Videoplasty.com, CC-BY-SA 4.0
- Pencil: The "Pencil" icon used by MobiText from Wikimedia Commons by Gavinstubbs09, CC-BY-SA-3.0
- Color pallet/scheme: [yeun/open-color](https://github.com/yeun/open-color)
- [Color picker mouse icon](https://github.com/ful1e5/BreezeX_Cursor/blob/main/bitmaps/BreezeX-Dark/color-picker.png)
