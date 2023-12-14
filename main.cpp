#include <QtWidgets/QApplication>
#include "zoomwidget.hpp"
#include <QCursor>
#include <QScreen>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	ZoomWidget w;
	w.setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
	w.resize(QApplication::screenAt(QCursor::pos())->geometry().size());
  w.move( QApplication::screenAt(QCursor::pos())->geometry().topLeft() );
	w.showFullScreen();

	w.show();
	w.grabDesktop();

  w.setCursor(QCursor(Qt::CrossCursor));

	return a.exec();
}
