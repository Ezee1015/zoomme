#include <QtWidgets/QApplication>
#include "zoomwidget.hpp"
#include <QCursor>
#include <QScreen>
#include <QImage>
#include <string.h>
#include <QSystemTrayIcon>
#include <stdio.h>
#include <stdlib.h>
#include <QFileInfo>

void printHelp(const int exitStatus, const char* errorMsg){
  if(strlen(errorMsg) != 0)
    printf("[ERROR] %s\n", errorMsg);
  else
    printf("ZoomMe is an application for zooming/magnifying and noting the desktop.\n");

  printf("\nUsage: zoomme [options]\n");
  printf("Options:\n");
  printf("  --help                    Display this help message\n");
  printf("  -p [path/to/folder]       Specify the path where to save the exported (saved) image (default: Pictures folder)\n");
  printf("  -n [file_name]            Specify the name of the exported (saved) image (default: Zoomme {date})\n");
  printf("  -e:i [extension]          Specify the extension of the exported (saved) image (default: png)\n");
  printf("  -e:v [extension]          Specify the extension of the exported (saved) video file (default: mp4)\n");
  printf("  -l                        EXPERIMENTAL: Not use a background (live mode/transparent). In this mode there's no zooming, only drawings allowed\n");
  printf("  -i <image_path> [opts]    Specify the path to an image as the background, instead of the desktop. It will automatically fit it to the screen\n");
  printf("       -w                        Force to fit to the screen's width\n");
  printf("       -h                        Force to fit to the screen's height\n");
  printf("       --replace-on-save         This will replace the source image (autocompletes -p, -e and -n flags).\n");
  printf("  -r [path/to/file] [opts]  Load/Restore the state of the program saved in that file. It should be a '.zoomme' file\n");
  printf("       -w                        Force to fit to the screen's width if the recovered file doesn't have the same resolution\n");
  printf("       -h                        Force to fit to the screen's height if the recovered file doesn't have the same resolution\n");

  printf("\n  For more information, visit https://github.com/Ezee1015/zoomme\n");

  exit(exitStatus);
}

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  QSystemTrayIcon tray = QSystemTrayIcon(QIcon(":/resources/Icon.png"));
  tray.setVisible(true);
  tray.show();

  QString img;
  QString savePath;
  QString saveName;
  QString saveImgExt;
  QString saveVidExt;
  QString backupFile;
  bool liveMode = false;
  FitImage fitOption = FIT_AUTO;
  // Parsing arguments
  for(int i=1; i<argc ; ++i){
    if(strcmp(argv[i], "--help") == 0)
      printHelp(EXIT_SUCCESS, "");

    else if(strcmp(argv[i], "-l") == 0){
      if(liveMode == true)
        printHelp(EXIT_FAILURE, "Live mode already specified");

      liveMode=true;
    }

    else if(strcmp(argv[i], "-i") == 0) {
      if((i+1) == argc)
        printHelp(EXIT_FAILURE, "Image path not provided");

      if(img != "")
        printHelp(EXIT_FAILURE, "Image already provided");

      img = argv[++i];
    }

    else if(strcmp(argv[i], "-w") == 0) {
      if(img.isEmpty() && backupFile.isEmpty())
        printHelp(EXIT_FAILURE, "Fit width argument was given, but either the image or the recovery file was not provided");

      if(fitOption != FIT_AUTO)
        printHelp(EXIT_FAILURE, "Fit setting already provided");

      fitOption = FIT_TO_WIDTH;
    }

    else if(strcmp(argv[i], "-h") == 0) {
      if(img.isEmpty() && backupFile.isEmpty())
        printHelp(EXIT_FAILURE, "Fit height argument was given, but either the image or the recovery file was not provided");

      if(fitOption != FIT_AUTO)
        printHelp(EXIT_FAILURE, "Fit setting already provided");

      fitOption = FIT_TO_HEIGHT;
    }

    else if(strcmp(argv[i], "--replace-on-save") == 0) {
      if(img.isEmpty())
        printHelp(EXIT_FAILURE, "Override source image was indicated, but the source image is not provided");

      if(savePath != "")
        printHelp(EXIT_FAILURE, "Saving path already provided");
      if(saveName != "")
        printHelp(EXIT_FAILURE, "Saving name already provided");
      if(saveImgExt != "")
        printHelp(EXIT_FAILURE, "Saving extension already provided");

      QFileInfo imgInfo = QFileInfo(img);

      savePath      = imgInfo.path();
      saveName      = imgInfo.completeBaseName();
      saveImgExt    = imgInfo.suffix();
    }

    else if(strcmp(argv[i], "-p") == 0) {
      if((i+1) == argc)
        printHelp(EXIT_FAILURE, "Saving path not provided");

      if(savePath != "")
        printHelp(EXIT_FAILURE, "Saving path already provided");

      savePath = argv[++i];
    }

    else if(strcmp(argv[i], "-n") == 0) {
      if((i+1) == argc)
        printHelp(EXIT_FAILURE, "Saving name not provided");

      if(saveName != "")
        printHelp(EXIT_FAILURE, "Saving name already provided");

      saveName = argv[++i];
    }

    else if(strcmp(argv[i], "-e:i") == 0) {
      if((i+1) == argc)
        printHelp(EXIT_FAILURE, "Saving extension for the image not provided");

      if(saveImgExt != "")
        printHelp(EXIT_FAILURE, "Saving extension already provided");

      saveImgExt = argv[++i];
    }

    else if(strcmp(argv[i], "-e:v") == 0) {
      if((i+1) == argc)
        printHelp(EXIT_FAILURE, "Saving extension for the video file not provided");

      if(saveVidExt != "")
        printHelp(EXIT_FAILURE, "Saving extension fot the video file already provided");

      saveVidExt = argv[++i];
    }

    else if(strcmp(argv[i], "-r") == 0) {
      backupFile = argv[++i];

      if(QFileInfo(backupFile).suffix() != "zoomme")
        printHelp(EXIT_FAILURE, "It's not a '.zoomme' file");
    }

    else {
      QString textError;
      textError.append("Unknown flag: ");
      textError.append(argv[i]);
      printHelp(EXIT_FAILURE, QSTRING_TO_STRING(textError));
    }
  }

  ZoomWidget w;
  w.setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
  w.resize(QApplication::screenAt(QCursor::pos())->geometry().size());
  w.move(QApplication::screenAt(QCursor::pos())->geometry().topLeft());
  w.setCursor(QCursor(Qt::CrossCursor));
  // Set transparency
  w.setAttribute(Qt::WA_TranslucentBackground, true);
  w.show();

  // Set the path, name and extension for saving the file
  QString saveFileError = w.initFileConfig(savePath, saveName, saveImgExt, saveVidExt);
  if(!saveFileError.isEmpty())
    printHelp(EXIT_FAILURE, qPrintable(saveFileError));

  // Configure the app source
  if(!backupFile.isEmpty()) {
    bool restoreCorrect = w.restoreStateFromFile(backupFile, fitOption);
    if(!restoreCorrect)
      printHelp(EXIT_FAILURE, "Couldn't restore the state from the file");

  } else if(!img.isEmpty()) {
    bool getImageCorrect = w.grabImage(QPixmap(img), fitOption);
    if (!getImageCorrect)
      printHelp(EXIT_FAILURE, "Couldn't open the image");

  } else {
    w.grabDesktop(liveMode);
  }

  QApplication::beep();
  return a.exec();
}
