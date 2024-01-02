#include <QtWidgets/QApplication>
#include "zoomwidget.hpp"
#include <QCursor>
#include <QScreen>
#include <QImage>
#include <string.h>
#include <QSystemTrayIcon>
#include <stdio.h>
#include <stdlib.h>

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
  printf("  -e [extension]            Specify the extension of the exported (saved) image (default: png)\n");
  printf("  -l                        EXPERIMENTAL: Not use a background (live mode/transparent). In this mode there's no zooming, only drawings allowed\n");
  printf("  -i <image_path> [-w|-h]   Specify the path to an image as the background, instead of the desktop. It will automatically fit it to the screen\n");
  printf("                    -w            Force to fit to the screen's width\n");
  printf("                    -h            Force to fit to the screen's height\n");

  printf("\n  For more information, visit https://github.com/Ezee1015/zoomme\n");

  exit(exitStatus);
}

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  QSystemTrayIcon tray = QSystemTrayIcon(QIcon("./resources/Icon.png"));
  tray.setVisible(true);
  tray.show();

  QString img;
  QString savePath;
  QString saveName;
  QString saveExtension;
  bool liveMode = false;
  FitImage fitToWidth = FIT_AUTO;
  // Parsing arguments
  for(int i=0; i<argc ; ++i){
    if(strcmp(argv[i], "--help") == 0)
      printHelp(EXIT_SUCCESS, "");

    if(strcmp(argv[i], "-l") == 0)
      liveMode=true;

    if(strcmp(argv[i], "-i") == 0) {
      if((i+1) == argc)
        printHelp(EXIT_FAILURE, "Image path not provided");

      img = argv[++i];
    }

    if(strcmp(argv[i], "-w") == 0) {
      if(img.isEmpty())
        printHelp(EXIT_FAILURE, "Fit width argument was given, but the image not provided");

      fitToWidth = FIT_TO_WIDTH;
    }

    if(strcmp(argv[i], "-h") == 0) {
      if(img.isEmpty())
        printHelp(EXIT_FAILURE, "Fit height argument was given, but the image not provided");

      fitToWidth = FIT_TO_HEIGHT;
    }

    if(strcmp(argv[i], "-p") == 0) {
      if((i+1) == argc)
        printHelp(EXIT_FAILURE, "Saving path not provided");

      savePath = argv[++i];
    }

    if(strcmp(argv[i], "-n") == 0) {
      if((i+1) == argc)
        printHelp(EXIT_FAILURE, "Saving name not provided");

      saveName = argv[++i];
    }

    if(strcmp(argv[i], "-e") == 0) {
      if((i+1) == argc)
        printHelp(EXIT_FAILURE, "Saving extension not provided");

      saveExtension = argv[++i];
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
  QString saveFileError = w.initializeSaveFile(savePath, saveName, saveExtension);
  if(!saveFileError.isEmpty()) printHelp(EXIT_FAILURE, qPrintable(saveFileError));

  if(img.isEmpty()) w.grabDesktop(liveMode);
  else {
    if (!w.grabImage(img, fitToWidth))
      printHelp(EXIT_FAILURE, "Couldn't open the image");
  }

  QApplication::beep();
  return a.exec();
}
