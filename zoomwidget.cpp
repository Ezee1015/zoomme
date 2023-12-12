#include "zoomwidget.hpp"
#include "ui_zoomwidget.h"

#include <cmath>
#include <QPainter>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QScreen>
#include <QGuiApplication>

ZoomWidget::ZoomWidget(QWidget *parent) :
		QGLWidget(parent),
		ui(new Ui::zoomwidget)
{
	ui->setupUi(this);
	setMouseTracking(true);

	_state = STATE_MOVING;

	_desktopPixmapPos = QPoint(0, 0);
	_desktopPixmapSize = QApplication::desktop()->size();
	_desktopPixmapScale = 1.0f;

	_scaleSensivity = 0.1f;

	_drawMode = DRAWMODE_LINE;

	_activePen.setColor(QColor(255, 0, 0));
	_activePen.setWidth(4);
}

ZoomWidget::~ZoomWidget()
{
	delete ui;
}

void drawArrowHead(int x, int y, int width, int height, QPainter *p) {
  float opposite=-1 * height;
  float adjacent=width;

  float angle;
  if(adjacent==0) angle=M_PI/2;
  else angle = atanf(abs(opposite) / abs(adjacent));

  if( opposite>=0 && adjacent<0 )
    angle = M_PI-angle;
  else if( opposite<0 && adjacent<=0 )
    angle = M_PI+angle;
  else if( opposite<=0 && adjacent>0 )
    angle = 2*M_PI-angle;

  float proportion= 0.25 * sin(4*angle-(M_PI/2)) + 0.75;
  int large = sqrt(pow(opposite,2) + pow(adjacent,2)) * 0.15;
  if (large>85) large=85;
  int originX=width+x, originY=height+y;
  int rightLineX, rightLineY, leftLineX, leftLineY;

  if(angle<=(M_PI/4)){
    rightLineX=-large*proportion;
    rightLineY=-large*(1-proportion);
    leftLineX=-large*(1-proportion);
    leftLineY=large*proportion;
  } else if(angle<=(2*M_PI/4)){
    rightLineX=-large*proportion;
    rightLineY=large*(1-proportion);
    leftLineX=large*(1-proportion);
    leftLineY=large*proportion;
  } else if(angle<=(3*M_PI/4)){
    rightLineX=-large*(1-proportion);
    rightLineY=large*proportion;
    leftLineX=large*proportion;
    leftLineY=large*(1-proportion);
  } else if(angle<=(4*M_PI/4)){
    rightLineX=large*(1-proportion);
    rightLineY=large*proportion;
    leftLineX=large*proportion;
    leftLineY=-large*(1-proportion);
  } else if(angle<=(5*M_PI/4)){
    rightLineX=large*proportion;
    rightLineY=large*(1-proportion);
    leftLineX=large*(1-proportion);
    leftLineY=-large*proportion;
  } else if(angle<=(6*M_PI/4)){
    rightLineX=large*proportion;
    rightLineY=-large*(1-proportion);
    leftLineX=-large*(1-proportion);
    leftLineY=-large*proportion;
  } else if(angle<=(7*M_PI/4)){
    rightLineX=large*(1-proportion);
    rightLineY=-large*proportion;
    leftLineX=-large*proportion;
    leftLineY=-large*(1-proportion);
  } else {
    rightLineX=-large*(1-proportion);
    rightLineY=-large*proportion;
    leftLineX=-large*proportion;
    leftLineY=large*(1-proportion);
  }

  p->drawLine(originX, originY, originX+rightLineX, originY+rightLineY);
  p->drawLine(originX, originY, originX+leftLineX, originY+leftLineY);
}

void ZoomWidget::paintEvent(QPaintEvent *event)
{
	QPainter p;

	p.begin(this);

	// Draw desktop pixmap.
	p.drawPixmap(_desktopPixmapPos.x(), _desktopPixmapPos.y(),
		     _desktopPixmapSize.width(), _desktopPixmapSize.height(),
		     _desktopPixmap);

	// Draw user objects.
	int x, y, w, h;
	for (int i = 0; i < _userRects.size(); ++i) {
		p.setPen(_userRects.at(i).pen);
		getRealUserObjectPos(_userRects.at(i), &x, &y, &w, &h);
		p.drawRect(x, y, w, h);
	}

	// Draw user lines.
	for (int i = 0; i < _userLines.size(); ++i) {
		p.setPen(_userLines.at(i).pen);
		getRealUserObjectPos(_userLines.at(i), &x, &y, &w, &h);
		p.drawLine(x, y, x+w, y+h);
	}

	// Draw user arrows.
	for (int i = 0; i < _userArrows.size(); ++i) {
		p.setPen(_userArrows.at(i).pen);
		getRealUserObjectPos(_userArrows.at(i), &x, &y, &w, &h);
		p.drawLine(x, y, x+w, y+h);
    drawArrowHead(x, y, w, h, &p);
	}

	// Draw user ellipses.
	for (int i = 0; i < _userEllipses.size(); ++i) {
		p.setPen(_userEllipses.at(i).pen);
		getRealUserObjectPos(_userEllipses.at(i), &x, &y, &w, &h);
		p.drawEllipse(x, y, w, h);
	}

	// Draw active user object.
	if (_state == STATE_DRAWING) {
		p.setPen(_activePen);

		int x = _desktopPixmapPos.x() + _startDrawPoint.x()*_desktopPixmapScale;
		int y = _desktopPixmapPos.y() + _startDrawPoint.y()*_desktopPixmapScale;
		int width = (_endDrawPoint.x() - _startDrawPoint.x())*_desktopPixmapScale;
		int height = (_endDrawPoint.y() - _startDrawPoint.y())*_desktopPixmapScale;

		if (_drawMode == DRAWMODE_RECT) {
			p.drawRect(x, y, width, height);
		} else if (_drawMode == DRAWMODE_LINE) {
			p.drawLine(x, y, width + x, height + y);
		} else if (_drawMode == DRAWMODE_ARROW) {
      p.drawLine(x, y, width + x, height + y);
      drawArrowHead(x, y, width, height, &p);
		} else if (_drawMode == DRAWMODE_ELLIPSE) {
			p.drawEllipse(x, y, width, height);
		}
	}

	p.end();
}

void ZoomWidget::mousePressEvent(QMouseEvent *event)
{
	_lastMousePos = event->pos();

	_state = STATE_DRAWING;

	_startDrawPoint = (event->pos() - _desktopPixmapPos)/_desktopPixmapScale;
	_endDrawPoint = _startDrawPoint;
}

void ZoomWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (_state == STATE_DRAWING) {
		_endDrawPoint = (event->pos() - _desktopPixmapPos)/_desktopPixmapScale;

		UserObjectData data;
		data.pen = _activePen;
		data.startPoint = _startDrawPoint;
		data.endPoint = _endDrawPoint;
		if (_drawMode == DRAWMODE_LINE) {
			_userLines.append(data);
		} else if (_drawMode == DRAWMODE_RECT) {
			_userRects.append(data);
		} else if (_drawMode == DRAWMODE_ARROW) {
			_userArrows.append(data);
		} else if (_drawMode == DRAWMODE_ELLIPSE) {
			_userEllipses.append(data);
    }

		_state = STATE_MOVING;
		update();
	}
}

void ZoomWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (_state == STATE_MOVING) {
		QPoint delta = event->pos() - _lastMousePos;

		shiftPixmap(delta);
		checkPixmapPos();
	} else if (_state == STATE_DRAWING) {
		_endDrawPoint = (event->pos() - _desktopPixmapPos)/_desktopPixmapScale;
	}

	_lastMousePos = event->pos();
	update();
}

void ZoomWidget::wheelEvent(QWheelEvent *event)
{
	if (_state == STATE_MOVING) {
		int sign = event->delta() / abs(event->delta());

		_desktopPixmapScale += sign * _scaleSensivity;
		if (_desktopPixmapScale < 1.0f) {
			_desktopPixmapScale = 1.0f;
		}

		scalePixmapAt(event->pos());
		checkPixmapPos();

		update();
	}
}

void ZoomWidget::keyPressEvent(QKeyEvent *event)
{
	int key = event->key();

	if (key == Qt::Key_Escape) {
    if(_desktopPixmapSize != QApplication::desktop()->size()){ // If it's zoomed in, go back to normal
			_desktopPixmapScale = 1.0f;

      scalePixmapAt(QPoint(0,0));
      checkPixmapPos();
      update();
    } else // Otherwise, exit
      QApplication::quit();
	} else if ((key >= Qt::Key_1) && (key <= Qt::Key_9)) {
		_activePen.setWidth(key - Qt::Key_0);
	} else if (key == Qt::Key_R) {
		_activePen.setColor(QColor(255, 0, 0));
	} else if (key == Qt::Key_G) {
		_activePen.setColor(QColor(0, 255, 0));
	} else if (key == Qt::Key_B) {
		_activePen.setColor(QColor(0, 0, 255));
	} else if (key == Qt::Key_C) {
		_activePen.setColor(QColor(0, 255, 255));
	} else if (key == Qt::Key_M) {
		_activePen.setColor(QColor(255, 0, 255));
	} else if (key == Qt::Key_Y) {
		_activePen.setColor(QColor(255, 255, 0));
	} else if (key == Qt::Key_W) {
		_activePen.setColor(QColor(255, 255, 255));
	} else if (key == Qt::Key_U) {
    if( _drawMode == DRAWMODE_LINE && (! _userLines.isEmpty()) )
      _userLines.removeLast();
    if( _drawMode == DRAWMODE_RECT && (! _userRects.isEmpty()) )
      _userRects.removeLast();
    if( _drawMode == DRAWMODE_ARROW && (! _userArrows.isEmpty()) )
      _userArrows.removeLast();
    if( _drawMode == DRAWMODE_ELLIPSE && (! _userEllipses.isEmpty()) )
      _userEllipses.removeLast();
	} else if (key == Qt::Key_Q) {
		_userRects.clear();
		_userLines.clear();
		_userArrows.clear();
		_userEllipses.clear();
		_state = STATE_MOVING;
	} else if (key == Qt::Key_Z) {
		_drawMode = DRAWMODE_LINE;
	} else if (key == Qt::Key_X) {
		_drawMode = DRAWMODE_RECT;
	} else if (key == Qt::Key_A) {
		_drawMode = DRAWMODE_ARROW;
	} else if (key == Qt::Key_E) {
		_drawMode = DRAWMODE_ELLIPSE;
	}

	update();
}

void ZoomWidget::keyReleaseEvent(QKeyEvent *event)
{
}

void ZoomWidget::grabDesktop()
{
  QScreen *screen = QGuiApplication::primaryScreen();
  _desktopPixmap = screen->grabWindow(0);
}

void ZoomWidget::shiftPixmap(const QPoint delta)
{
	_desktopPixmapPos -= delta * (_desktopPixmapSize.width() / _desktopPixmap.width());
}

void ZoomWidget::scalePixmapAt(const QPoint pos)
{
	int old_w = _desktopPixmapSize.width();
	int old_h = _desktopPixmapSize.height();

	int new_w = _desktopPixmap.width() * _desktopPixmapScale;
	int new_h = _desktopPixmap.height() * _desktopPixmapScale;
	_desktopPixmapSize = QSize(new_w, new_h);

	int dw = new_w - old_w;
	int dh = new_h - old_h;

	int cur_x = pos.x() + abs(_desktopPixmapPos.x());
	int cur_y = pos.y() + abs(_desktopPixmapPos.y());

	float cur_px = -((float)cur_x / old_w);
	float cur_py = -((float)cur_y / old_h);

	_desktopPixmapPos.setX(_desktopPixmapPos.x() + dw*cur_px);
	_desktopPixmapPos.setY(_desktopPixmapPos.y() + dh*cur_py);
}

void ZoomWidget::checkPixmapPos()
{
	if (_desktopPixmapPos.x() > 0) {
		_desktopPixmapPos.setX(0);
	} else if ((_desktopPixmapSize.width() + _desktopPixmapPos.x()) < width()) {
		_desktopPixmapPos.setX(width() - _desktopPixmapSize.width());
	}

	if (_desktopPixmapPos.y() > 0) {
		_desktopPixmapPos.setY(0);
	} else if ((_desktopPixmapSize.height() + _desktopPixmapPos.y()) < height()) {
		_desktopPixmapPos.setY(height() - _desktopPixmapSize.height());
	}
}

void ZoomWidget::getRealUserObjectPos(const UserObjectData &userObj, int *x, int *y, int *w, int *h)
{
	*x = _desktopPixmapPos.x() + userObj.startPoint.x()*_desktopPixmapScale;
	*y = _desktopPixmapPos.y() + userObj.startPoint.y()*_desktopPixmapScale;
	*w = (userObj.endPoint.x() - userObj.startPoint.x())*_desktopPixmapScale;
	*h = (userObj.endPoint.y() - userObj.startPoint.y())*_desktopPixmapScale;
}
