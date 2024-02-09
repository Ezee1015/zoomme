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

void printHelp(const char *errorMsg)
{
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
  fprintf(output, "  -i <image_path> [opts]    Specify the path to an image as the background, instead of the desktop. It will automatically fit it to the screen\n");
  fprintf(output, "       -w                        Force to fit it to the screen's width\n");
  fprintf(output, "       -h                        Force to fit it to the screen's height\n");
  fprintf(output, "       --replace-on-save         This will replace the source image (autocompletes -p, -e and -n flags).\n");
  fprintf(output, "  -r [path/to/file] [opts]  Load/Restore the state of the program saved in that file. It should be a '.zoomme' file\n");
  fprintf(output, "       -w                        Force to fit it to the screen's width if the recovered file doesn't have the same resolution\n");
  fprintf(output, "       -h                        Force to fit it to the screen's height if the recovered file doesn't have the same resolution\n");
  fprintf(output, "  -c [opts]                 Load an image from the clipboard as the background, instead of the desktop. It will automatically fit it to the screen\n");
  fprintf(output, "       -w                        Force to fit it to the screen's width\n");
  fprintf(output, "       -h                        Force to fit it to the screen's height\n");

  fprintf(output, "\n  For more information, visit https://github.com/Ezee1015/zoomme\n");

  exit(exitStatus);
}

bool isDefined(QString mode)
{
  return !mode.isEmpty();
}

bool isDefined(FitImage mode)
{
  return mode != FIT_AUTO;
}

bool isDefinedFitSource(QString img, QString backupFile, bool fromClipboard)
{
  return isDefined(img) || isDefined(backupFile) || fromClipboard;
}

QString nextToken(int argc, char *argv[], int *pos, QString type)
{
  (*pos)++;

  if(*pos == argc){
    QString error(type + " not provided");
    printHelp(QSTRING_TO_STRING(error));
  }

  return argv[*pos];
}

void modeAlreadySelected(QString backupFile, QString img, bool liveMode, bool fromClipboard)
{
  if(isDefined(backupFile))
    printHelp("Mode already provided (Backup file provided)");

  else if(isDefined(img))
    printHelp("Mode already provided (image provided)");

  else if(liveMode)
    printHelp("Mode already provided (live mode)");

  else if(fromClipboard)
    printHelp("Mode already provided (load from clipboard)");
}

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  QSystemTrayIcon tray = QSystemTrayIcon(QIcon(":/resources/Icon.png"));
  tray.setVisible(true);
  tray.show();

  // Configurations
  QString savePath;
  QString saveName;
  QString saveImgExt;
  QString saveVidExt;

  // Modes
  QString img;
  QString backupFile;
  bool liveMode = false;
  bool fromClipboard = false;
  FitImage fitOption = FIT_AUTO;

  // Parsing arguments
  for(int i=1; i<argc ; ++i){
    if(strcmp(argv[i], "--help") == 0)
      printHelp("");

    else if(strcmp(argv[i], "-l") == 0){
      modeAlreadySelected(backupFile, img, liveMode, fromClipboard);

      liveMode=true;
    }

    else if(strcmp(argv[i], "-i") == 0) {
      modeAlreadySelected(backupFile, img, liveMode, fromClipboard);

      img = nextToken(argc, argv, &i, "Image path");
    }

    else if(strcmp(argv[i], "-w") == 0) {
      if(!isDefinedFitSource(img, backupFile, fromClipboard))
        printHelp("Fit width argument was given, but either the image or the recovery file was not provided");

      if(isDefined(fitOption)) printHelp("Fit setting already provided");

      fitOption = FIT_TO_WIDTH;
    }

    else if(strcmp(argv[i], "-h") == 0) {
      if(!isDefinedFitSource(img, backupFile, fromClipboard))
        printHelp("Fit height argument was given, but either the image or the recovery file was not provided");

      if(isDefined(fitOption)) printHelp("Fit setting already provided");

      fitOption = FIT_TO_HEIGHT;
    }

    else if(strcmp(argv[i], "--replace-on-save") == 0) {
      if(!isDefined(img))
        printHelp("Override source image was indicated, but the source image is not provided");

      if(isDefined(savePath))   printHelp("Saving path already provided");
      if(isDefined(saveName))   printHelp("Saving name already provided");
      if(isDefined(saveImgExt)) printHelp("Saving extension already provided");

      QFileInfo imgInfo = QFileInfo(img);

      savePath      = imgInfo.path();
      saveName      = imgInfo.completeBaseName();
      saveImgExt    = imgInfo.suffix();
    }

    else if(strcmp(argv[i], "-p") == 0) {
      if(isDefined(savePath)) printHelp("Saving path already provided");

      savePath = nextToken(argc, argv, &i, "Save path");
    }

    else if(strcmp(argv[i], "-n") == 0) {
      if(isDefined(saveName)) printHelp("Saving name already provided");

      saveName = nextToken(argc, argv, &i, "Save name");
    }

    else if(strcmp(argv[i], "-e:i") == 0) {
      if(isDefined(saveImgExt)) printHelp("Saving image extension already provided");

      saveImgExt = nextToken(argc, argv, &i, "Image extension");
    }

    else if(strcmp(argv[i], "-e:v") == 0) {
      if(isDefined(saveVidExt)) printHelp("Saving video extension already provided");

      saveVidExt = nextToken(argc, argv, &i, "Video extension");
    }

    else if(strcmp(argv[i], "-r") == 0) {
      modeAlreadySelected(backupFile, img, liveMode, fromClipboard);

      backupFile = nextToken(argc, argv, &i, "Backup file path");

      if(QFileInfo(backupFile).suffix() != "zoomme")
        printHelp("It's not a '.zoomme' file");
    }

    else if(strcmp(argv[i], "-c") == 0) {
      modeAlreadySelected(backupFile, img, liveMode, fromClipboard);

      fromClipboard = true;
    }

    else {
      QString textError("Unknown flag: ");
      textError.append(argv[i]);
      printHelp(QSTRING_TO_STRING(textError));
    }
  }

  ZoomWidget w;
  w.setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
  w.resize(QApplication::screenAt(QCursor::pos())->geometry().size());
  w.move(QApplication::screenAt(QCursor::pos())->geometry().topLeft());
  w.setCursor(QCursor(Qt::CrossCursor));
  // Set transparency
  w.setAttribute(Qt::WA_TranslucentBackground, true);
  w.show();

  // Set the path, name and extension for saving the file
  w.initFileConfig(savePath, saveName, saveImgExt, saveVidExt);

  // Configure the app mode
  if(isDefined(backupFile)) {
    // The backup file has it's own live mode
    w.restoreStateFromFile(backupFile, fitOption);

  } else if(isDefined(img)) {
    w.setLiveMode(false);
    w.grabImage(QPixmap(img), fitOption);

  } else if(fromClipboard) {
    w.grabFromClipboard(fitOption);

  } else {
    w.setLiveMode(liveMode);
    w.grabDesktop();
  }

  QApplication::beep();
  return a.exec();
}
