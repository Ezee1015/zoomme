#include "zoomwidget.hpp"
#include <QtWidgets/QApplication>
#include <QCursor>
#include <QScreen>
#include <QImage>
#include <string.h>
#include <QString>
#include <QSystemTrayIcon>
#include <stdio.h>
#include <stdlib.h>
#include <QFileInfo>

void help(const char *errorMsg)
{
  FILE *output;
  int exitStatus;

  if (strlen(errorMsg) == 0) {
    output = stdout;
    exitStatus = EXIT_SUCCESS;
    fprintf(output, "ZoomMe is an application for zooming/magnifying and noting the desktop.\n");
  } else {
    output = stderr;
    exitStatus = EXIT_FAILURE;
    fprintf(output, "[ERROR] %s\n", errorMsg);
  }

  fprintf(output, "\nUsage: zoomme [configurations] [mode]\n");

  fprintf(output, "\nHelp page:\n");
  fprintf(output, "  --help                    Display this help message\n");

  fprintf(output, "\nConfigurations:\n");
  fprintf(output, "  -p [path/to/folder]       Set the path where to save the exported files (default: Desktop folder)\n");
  fprintf(output, "  -n [file_name]            Specify the name of the exported files (default: Zoomme {date})\n");
  fprintf(output, "  -e:i [extension]          Specify the extension of the exported (saved) image (default: png)\n");
  fprintf(output, "  -e:v [extension]          Specify the extension of the exported (saved) video file (default: mp4)\n");

  fprintf(output, "\nModes:\n");
  fprintf(output, "  -l                        Not use a background (transparent). In this mode zooming is disabled\n");
  fprintf(output, "  -i <image_path> [opts]    Specify the path to an image as the background, instead of the desktop.\n");
  fprintf(output, "       --copy                    This will copy the source image path (autocompletes -p, -e and -n flags) -it will NOT replace the original image-.\n");
  fprintf(output, "  -r [path/to/file]         Load/Restore the state of the program saved in that file. It should be a '.zoomme' file\n");
  fprintf(output, "  -c                        Load an image from the clipboard as the background, instead of the desktop.\n");
  fprintf(output, "  --empty [width] [height]  Create an empty blackboard with the given size\n");

  fprintf(output, "\nExperimental:\n");
  fprintf(output, "  --floating                This option bypasses the window manager hint and creates its own window\n");

  fprintf(output, "\n  For more information, visit https://github.com/Ezee1015/zoomme\n");

  exit(exitStatus);
}

QString nextToken(const int argc, char *argv[], int *pos, const QString type)
{
  (*pos)++;

  if (*pos == argc) {
    QString error(type + " not provided");
    help(QSTRING_TO_STRING(error));
  }

  return argv[*pos];
}

enum Mode {
  DESKTOP,    // Grab the desktop (default)
  LIVE_MODE,
  IMAGE,      // Grab an image
  CLIPBOARD,  // Grab an image from the clipboard
  BACKUP,     // Recover from a backup file (.zoomme)
  BLACKBOARD  // Empty pixmap
};

void setMode(Mode *mode, const Mode newMode)
{
  switch (*mode) {
    case BACKUP:
      help("Mode already provided (Backup file provided)");
      break;

    case IMAGE:
      help("Mode already provided (image provided)");
      break;

    case LIVE_MODE:
      help("Mode already provided (live mode)");
      break;

    case CLIPBOARD:
      help("Mode already provided (load from clipboard)");
      break;

    case BLACKBOARD:
      help("Mode already provided (empty blackboard)");
      break;

    case DESKTOP: // Default value
      *mode = newMode;
      break;
  }
}

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  QSystemTrayIcon tray = QSystemTrayIcon(QIcon(":/resources/icon/Icon.png"));
  tray.setVisible(true);
  tray.show();

  // Configurations
  QString savePath;
  QString saveName;
  QString saveImgExt; // Extension
  QString saveVidExt; // Extension
  bool floating = false;

  // Modes
  Mode mode = DESKTOP;
  QString imgPath;
  QString backupPath;
  QSize blackboardSize;

  // Parsing arguments
  for (int i=1; i<argc ; ++i) {
    if (strcmp(argv[i], "--help") == 0) {
      help("");

    } else if (strcmp(argv[i], "--floating") == 0) {
      if (floating) {
        help("Floating option already set");
      }
      floating = true;

    } else if (strcmp(argv[i], "-l") == 0) {
      setMode(&mode, LIVE_MODE);

    } else if (strcmp(argv[i], "-i") == 0) {
      setMode(&mode, IMAGE);
      imgPath = nextToken(argc, argv, &i, "Image path");

    } else if (strcmp(argv[i], "--copy") == 0) {
      if (imgPath == "") {
        help("Copy the image path was indicated, but the source image is not provided");
      }
      if (savePath != "")   help("Saving path already provided");
      if (saveName != "")   help("Saving name already provided");
      if (saveImgExt != "") help("Saving extension already provided");

      QFileInfo imgInfo = QFileInfo(imgPath);
      savePath      = imgInfo.path();
      saveName      = imgInfo.completeBaseName();
      saveImgExt    = imgInfo.suffix();

    } else if (strcmp(argv[i], "-p") == 0) {
      if (savePath != "") {
        help("Saving path already provided");
      }

      savePath = nextToken(argc, argv, &i, "Save path");

    } else if (strcmp(argv[i], "-n") == 0) {
      if (saveName != "") {
        help("Saving name already provided");
      }

      saveName = nextToken(argc, argv, &i, "Save name");

    } else if (strcmp(argv[i], "-e:i") == 0) {
      if (saveImgExt != "") {
        help("Saving image extension already provided");
      }

      saveImgExt = nextToken(argc, argv, &i, "Image extension");

    } else if (strcmp(argv[i], "-e:v") == 0) {
      if (saveVidExt != "") {
        help("Saving video extension already provided");
      }

      saveVidExt = nextToken(argc, argv, &i, "Video extension");

    } else if (strcmp(argv[i], "-r") == 0) {
      setMode(&mode, BACKUP);

      backupPath = nextToken(argc, argv, &i, "Backup file path");
      if (QFileInfo(backupPath).suffix() != "zoomme") {
        QString errorMsg("It's not a '.zoomme' file: " + backupPath);
        help(QSTRING_TO_STRING(errorMsg));
      }

    } else if (strcmp(argv[i], "-c") == 0) {
      setMode(&mode, CLIPBOARD);

    } else if (strcmp(argv[i], "--empty") == 0) {
      setMode(&mode, BLACKBOARD);

      bool widthCorrect = false, heightCorrect=false;
      blackboardSize.setWidth(nextToken(argc, argv, &i, "Width for the blackboard").toInt(&widthCorrect));
      blackboardSize.setHeight(nextToken(argc, argv, &i, "Height for the blackboard").toInt(&heightCorrect));

      if (!widthCorrect)  help("The given width is not a number");
      if (!heightCorrect) help("The given height is not a number");

      if (blackboardSize.width() < 1)  help("The given width is not a positive number");
      if (blackboardSize.height() < 1) help("The given height is not a positive number");
    }

    else {
      QString textError("Unknown flag: ");
      textError.append(argv[i]);
      help(QSTRING_TO_STRING(textError));
    }
  }

  ZoomWidget w;
  if (floating) {
    w.setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
    w.activateWindow();
  } else {
    w.setWindowFlags(Qt::WindowMinimizeButtonHint);
  }
  w.resize(QApplication::screenAt(QCursor::pos())->geometry().size());
  w.move(QApplication::screenAt(QCursor::pos())->geometry().topLeft());
  w.setCursor(QCursor(Qt::CrossCursor));

  // Set the path, name and extension for saving the file
  w.initFileConfig(savePath, saveName, saveImgExt, saveVidExt);

  // Configure the app mode
  switch (mode) {
    case BACKUP:
      w.restoreStateFromFile(backupPath);
      break;
    case IMAGE:
      w.grabImage(QPixmap(imgPath));
      break;
    case BLACKBOARD:
      w.createBlackboard(blackboardSize);
      break;
    case CLIPBOARD:
      w.grabFromClipboard();
      break;
    case LIVE_MODE:
      // Set transparency for the window
      w.setAttribute(Qt::WA_TranslucentBackground, true);
      w.setLiveMode();
      break;
    case DESKTOP:
      w.grabDesktop();
      break;
  }

  QApplication::beep();
  w.show();
  return a.exec();
}
