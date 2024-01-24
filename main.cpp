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

void printHelp(const char *errorMsg){
  FILE *output;
  int exitStatus;

  if(strlen(errorMsg) == 0){
    output = stdout;
    exitStatus = EXIT_SUCCESS;
    fprintf(output, "ZoomMe is an application for zooming/magnifying and noting the desktop.\n");
  } else {
    output = stderr;
    exitStatus = EXIT_FAILURE;
    fprintf(output, "[ERROR] %s\n", errorMsg);
  }

  fprintf(output, "\nUsage: zoomme [options]\n");
  fprintf(output, "Options:\n");
  fprintf(output, "  --help                    Display this help message\n");
  fprintf(output, "  -p [path/to/folder]       Set the path where to save the exported files (default: Desktop folder)\n");
  fprintf(output, "  -n [file_name]            Specify the name of the exported files (default: Zoomme {date})\n");
  fprintf(output, "  -e:i [extension]          Specify the extension of the exported (saved) image (default: png)\n");
  fprintf(output, "  -e:v [extension]          Specify the extension of the exported (saved) video file (default: mp4)\n");
  fprintf(output, "  -l                        Not use a background (transparent). In this mode zooming is disabled\n");
  fprintf(output, "  -i <image_path> [opts]    Specify the path to an image as the background, instead of the desktop. It will automatically fit it to the screen\n");
  fprintf(output, "       -w                        Force to fit to the screen's width\n");
  fprintf(output, "       -h                        Force to fit to the screen's height\n");
  fprintf(output, "       --replace-on-save         This will replace the source image (autocompletes -p, -e and -n flags).\n");
  fprintf(output, "  -r [path/to/file] [opts]  Load/Restore the state of the program saved in that file. It should be a '.zoomme' file\n");
  fprintf(output, "       -w                        Force to fit to the screen's width if the recovered file doesn't have the same resolution\n");
  fprintf(output, "       -h                        Force to fit to the screen's height if the recovered file doesn't have the same resolution\n");

  fprintf(output, "\n  For more information, visit https://github.com/Ezee1015/zoomme\n");

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
      printHelp("");

    else if(strcmp(argv[i], "-l") == 0){
      if(liveMode == true)
        printHelp("Live mode already specified");

      liveMode=true;
    }

    else if(strcmp(argv[i], "-i") == 0) {
      if((i+1) == argc)
        printHelp("Image path not provided");

      if(img != "")
        printHelp("Image already provided");

      img = argv[++i];
    }

    else if(strcmp(argv[i], "-w") == 0) {
      if(img.isEmpty() && backupFile.isEmpty())
        printHelp("Fit width argument was given, but either the image or the recovery file was not provided");

      if(fitOption != FIT_AUTO)
        printHelp("Fit setting already provided");

      fitOption = FIT_TO_WIDTH;
    }

    else if(strcmp(argv[i], "-h") == 0) {
      if(img.isEmpty() && backupFile.isEmpty())
        printHelp("Fit height argument was given, but either the image or the recovery file was not provided");

      if(fitOption != FIT_AUTO)
        printHelp("Fit setting already provided");

      fitOption = FIT_TO_HEIGHT;
    }

    else if(strcmp(argv[i], "--replace-on-save") == 0) {
      if(img.isEmpty())
        printHelp("Override source image was indicated, but the source image is not provided");

      if(savePath != "")
        printHelp("Saving path already provided");
      if(saveName != "")
        printHelp("Saving name already provided");
      if(saveImgExt != "")
        printHelp("Saving extension already provided");

      QFileInfo imgInfo = QFileInfo(img);

      savePath      = imgInfo.path();
      saveName      = imgInfo.completeBaseName();
      saveImgExt    = imgInfo.suffix();
    }

    else if(strcmp(argv[i], "-p") == 0) {
      if((i+1) == argc)
        printHelp("Saving path not provided");

      if(savePath != "")
        printHelp("Saving path already provided");

      savePath = argv[++i];
    }

    else if(strcmp(argv[i], "-n") == 0) {
      if((i+1) == argc)
        printHelp("Saving name not provided");

      if(saveName != "")
        printHelp("Saving name already provided");

      saveName = argv[++i];
    }

    else if(strcmp(argv[i], "-e:i") == 0) {
      if((i+1) == argc)
        printHelp("Saving extension for the image not provided");

      if(saveImgExt != "")
        printHelp("Saving extension already provided");

      saveImgExt = argv[++i];
    }

    else if(strcmp(argv[i], "-e:v") == 0) {
      if((i+1) == argc)
        printHelp("Saving extension for the video file not provided");

      if(saveVidExt != "")
        printHelp("Saving extension fot the video file already provided");

      saveVidExt = argv[++i];
    }

    else if(strcmp(argv[i], "-r") == 0) {
      backupFile = argv[++i];

      if(QFileInfo(backupFile).suffix() != "zoomme")
        printHelp("It's not a '.zoomme' file");
    }

    else {
      QString textError;
      textError.append("Unknown flag: ");
      textError.append(argv[i]);
      printHelp(QSTRING_TO_STRING(textError));
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
  w.initFileConfig(savePath, saveName, saveImgExt, saveVidExt);

  // Configure the app source
  if(!backupFile.isEmpty()) {
    w.restoreStateFromFile(backupFile, fitOption);
    // The backup file has it's own live mode

  } else if(!img.isEmpty()) {
    w.grabImage(QPixmap(img), fitOption);
    w.setLiveMode(false);

  } else {
    w.grabDesktop();
    w.setLiveMode(liveMode);
  }

  QApplication::beep();
  return a.exec();
}
