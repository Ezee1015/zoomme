#include <QtWidgets/QApplication>
#include "zoomwidget.hpp"
#include <QCursor>
#include <QScreen>
#include <QImage>
#include <string.h>
#include <QSystemTrayIcon>

void printHelp(int exitStatus, const char* errorMsg){
  if(strlen(errorMsg) != 0)
    printf("[ERROR] %s\n", errorMsg);
  else
    printf("ZoomMe is an application for zooming/magnifying and noting the desktop.\n");
  printf("\nUsage: zoomme [options]\n");
  printf("Options:\n");
  printf("  -h, --help        Display this help message\n");
  printf("  -i <image_path>   Specify the path to an image as the background, instead of the desktop\n");

  printf("\n\n  For more information, visit https://github.com/Ezee1015/zoomme\n");

  exit(exitStatus);
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
  QSystemTrayIcon tray = QSystemTrayIcon(QIcon("./resources/Icon.png"));
  tray.setVisible(true);
  tray.show();

  QString img;
  // Parsing arguments
  for(int i=0; i<argc ; ++i){
    if(strcmp(argv[i], "-h") == 0)
      printHelp(0, "");

    if(strcmp(argv[i], "-i") == 0) {
      if((i+1) == argc)
        printHelp(1, "Image path not provided");

      img = argv[i+1];
    }
  }

	ZoomWidget w;
	w.setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
	w.resize(QApplication::screenAt(QCursor::pos())->geometry().size());
  w.move(QApplication::screenAt(QCursor::pos())->geometry().topLeft());
	w.showFullScreen();
  w.setCursor(QCursor(Qt::CrossCursor));
	w.show();
	if(img.isEmpty()) w.grabDesktop();
  else {
    if (w.grabImage(img) == false)
      printHelp(1, "Couldn't open the image");
  }

  QApplication::beep();
	return a.exec();
}
