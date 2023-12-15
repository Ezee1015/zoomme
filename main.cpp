#include <QtWidgets/QApplication>
#include "zoomwidget.hpp"
#include <QCursor>
#include <QScreen>
#include <QImage>
#include <string.h>

void printHelp(int exitStatus, const char* errorMsg){
  if(strlen(errorMsg) != 0)
    printf("\n[ERROR] %s\n", errorMsg);

  printf("\nHere goes the help message. I'm a little lazy to write this right now");
  exit(exitStatus);
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
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
  w.move( QApplication::screenAt(QCursor::pos())->geometry().topLeft() );
	w.showFullScreen();

	w.show();
	if(img.isEmpty()) w.grabDesktop();
  else {
    if (w.grabImage(img) == false)
      printHelp(1, "Couldn't open the image");
  }

  w.setCursor(QCursor(Qt::CrossCursor));

	return a.exec();
}
