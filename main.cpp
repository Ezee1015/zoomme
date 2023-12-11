#include <QtWidgets/QApplication>
#include "zoomwidget.hpp"
#include <QDesktopWidget>
#include <QCursor>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	ZoomWidget w;
	w.setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
	w.resize(QApplication::desktop()->size());
	w.showFullScreen();

	w.show();
	w.grabDesktop();

  w.setCursor(QCursor(Qt::CrossCursor));

	return a.exec();
}
