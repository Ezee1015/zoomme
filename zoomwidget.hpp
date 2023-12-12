#ifndef ZOOMWIDGET_HPP
#define ZOOMWIDGET_HPP

#include <QGLWidget>
#include <QString>

namespace Ui {
	class zoomwidget;
}

// User data structs.
struct UserObjectData {
	QPoint startPoint;
	QPoint endPoint;
	QPen pen;
};

struct UserTextData {
  UserObjectData data;
  QFont font;
  QString text;
};

enum ZoomWidgetState {
	STATE_MOVING,
	STATE_DRAWING,
	STATE_TYPING,
};

enum ZoomWidgetDrawMode {
	DRAWMODE_ARROW,
	DRAWMODE_LINE,
	DRAWMODE_RECT,
	DRAWMODE_ELLIPSE,
	DRAWMODE_TEXT,
};

class ZoomWidget : public QGLWidget
{
	Q_OBJECT

public:
	explicit ZoomWidget(QWidget *parent = 0);
	~ZoomWidget();

	void grabDesktop();

protected:
	virtual void paintEvent(QPaintEvent *event);

	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);

	virtual void wheelEvent(QWheelEvent *event);

	virtual void keyPressEvent(QKeyEvent *event);
	virtual void keyReleaseEvent(QKeyEvent *event);

private:
	Ui::zoomwidget *ui;

	// Desktop pixmap properties.
	QPixmap		_desktopPixmap;
	QPoint		_desktopPixmapPos;
	QSize		_desktopPixmapSize;
	float		_desktopPixmapScale;

	// User objects.
	QVector<UserObjectData> _userRects;
	QVector<UserObjectData> _userLines;
	QVector<UserObjectData> _userArrows;
	QVector<UserObjectData> _userEllipses;
	QVector<UserTextData>   _userTexts;

	// Moving properties.
	float		_scaleSensivity;


	ZoomWidgetState	_state;
	QPoint		_lastMousePos;


	// Drawing properties.
	ZoomWidgetDrawMode	_drawMode;
	QPoint	_startDrawPoint;
	QPoint	_endDrawPoint;
	QPen	_activePen;


	void shiftPixmap(const QPoint delta);
	void scalePixmapAt(const QPointF pos);

	void checkPixmapPos();

	void getRealUserObjectPos(const UserObjectData &userObj, int *x, int *y, int *w, int *h);
};

#endif // ZOOMWIDGET_HPP
