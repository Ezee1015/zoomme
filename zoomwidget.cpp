#include "zoomwidget.hpp"
#include "ui_zoomwidget.h"

#include <cmath>
#include <QPainter>
#include <QMouseEvent>
#include <QScreen>
#include <QRect>
#include <QString>
#include <QFont>
#include <QPen>
#include <QGuiApplication>
#include <QOpenGLWidget>
#include <QCursor>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QColor>
#include <QPainterPath>

ZoomWidget::ZoomWidget(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::zoomwidget)
{
  ui->setupUi(this);
  setMouseTracking(true);

  _state = STATE_MOVING;

  _desktopScreen = QGuiApplication::screenAt(QCursor::pos());
  _desktopPixmapPos = QPoint(0, 0);
  _desktopPixmapSize = QApplication::screenAt(QCursor::pos())->geometry().size();
  _desktopPixmapOriginalSize = _desktopPixmapSize;
  _desktopPixmapScale = 1.0f;

  _scaleSensivity = 0.2f;

  _drawMode = DRAWMODE_LINE;
  _boardMode=0;

  _shiftPressed = false;
  _mousePressed = false;
  _flashlightMode = false;
  _flashlightRadius = 80;

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
  if(!_liveMode || _boardMode){
    setAttribute(Qt::WA_TranslucentBackground, false);
    p.drawPixmap(_desktopPixmapPos.x(), _desktopPixmapPos.y(),
        _desktopPixmapSize.width(), _desktopPixmapSize.height(),
        _drawnPixmap);
  }

  // Draw user rectangles.
  int x, y, w, h;
  for (int i = 0; i < _userRects.size(); ++i) {
    p.setPen(_userRects.at(i).pen);
    getRealUserObjectPos(_userRects.at(i), &x, &y, &w, &h);
    p.drawRect(x, y, w, h);
  }

  // Draw user Highlights
  for (int i = 0; i < _userHighlights.size(); ++i) {
    p.setPen(_userHighlights.at(i).pen);
    getRealUserObjectPos(_userHighlights.at(i), &x, &y, &w, &h);
    QColor color = p.pen().color();
    p.fillRect(QRect(x, y, w, h), QColor(color.red(), color.green(), color.blue(), 75));
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

  // Draw user FreeForms.
  int freeFormCount = (!_userFreeForms.isEmpty() && _userFreeForms.last().active) ? _userFreeForms.size()-1: _userFreeForms.size();
  for (int i = 0; i < freeFormCount; ++i) {
    p.setPen(_userFreeForms.at(i).pen);

    for (int z = 0; z < _userFreeForms.at(i).points.size()-1; ++z) {
      QPoint current = _userFreeForms.at(i).points.at(z);
      QPoint next    = _userFreeForms.at(i).points.at(z+1);

      current = _desktopPixmapPos + current * _desktopPixmapScale;
      next = _desktopPixmapPos + next * _desktopPixmapScale;

      p.drawLine(current.x(), current.y(), next.x(), next.y());
    }
  }

  // Draw user Texts.
  for (int i = 0; i < _userTexts.size(); ++i) {
    // If it's writing, not draw it
    if( (i == _userTexts.size()-1) && (_state == STATE_TYPING) )
      break;

    p.setPen(_userTexts.at(i).data.pen);
    p.setFont(_userTexts.at(i).font);
    getRealUserObjectPos(_userTexts.at(i).data, &x, &y, &w, &h);
    QString text = _userTexts.at(i).text;
    p.drawText(QRect(x, y, w, h), Qt::AlignCenter | Qt::TextWordWrap, text);
  }

  // Opaque the area outside the circle of the cursor
  if(_flashlightMode){
    QPoint c = mapFromGlobal(QCursor::pos());
    int radius = _flashlightRadius;

    QRect mouseFlashlightBorder = QRect(c.x()-radius, c.y()-radius, radius*2, radius*2);
    QPainterPath mouseFlashlight;
    mouseFlashlight.addEllipse( mouseFlashlightBorder );

    // p.setPen(QColor(186,186,186,200));
    // p.drawEllipse( mouseFlashlightBorder );

    QPainterPath pixmapPath;
    pixmapPath.addRect(_drawnPixmap.rect());

    QPainterPath flashlightArea = pixmapPath.subtracted(mouseFlashlight);
    p.fillPath(flashlightArea, QColor(  0,  0,  0, 190));
  }

  // If it's writing the text
  if(_state == STATE_TYPING){
    UserTextData textObject = _userTexts.last();
    p.setPen(textObject.data.pen);
    p.setFont(textObject.font);
    getRealUserObjectPos(textObject.data, &x, &y, &w, &h);
    QString text = textObject.text;
    if( text.isEmpty() )
      text="Type some text... \nThen press Enter to finish...";
    p.drawText(QRect(x, y, w, h), Qt::AlignCenter | Qt::TextWordWrap, text);
    QPen tempPen = p.pen(); tempPen.setWidth(1); p.setPen(tempPen);
    p.drawRect(x, y, w, h);
  }

  // Draw active user object.
  if (_state == STATE_DRAWING) {
    p.setPen(_activePen);

    int x = _desktopPixmapPos.x() + _startDrawPoint.x()*_desktopPixmapScale;
    int y = _desktopPixmapPos.y() + _startDrawPoint.y()*_desktopPixmapScale;
    int width = (_endDrawPoint.x() - _startDrawPoint.x())*_desktopPixmapScale;
    int height = (_endDrawPoint.y() - _startDrawPoint.y())*_desktopPixmapScale;

    switch(_drawMode) {
      case DRAWMODE_RECT:
        p.drawRect(x, y, width, height);
        break;
      case DRAWMODE_LINE:
        p.drawLine(x, y, width + x, height + y);
        break;
      case DRAWMODE_ARROW:
        p.drawLine(x, y, width + x, height + y);
        drawArrowHead(x, y, width, height, &p);
        break;
      case DRAWMODE_ELLIPSE:
        p.drawEllipse(x, y, width, height);
        break;
      case DRAWMODE_TEXT:
        {
          QPen tempPen = p.pen(); tempPen.setWidth(1); p.setPen(tempPen);
          QFont font; font.setPixelSize(_activePen.width() * 4); p.setFont(font);
          p.drawRect(x, y, width, height);
          QString defaultText;
          defaultText.append("Sizing... (");
          defaultText.append(QString::number(width));
          defaultText.append("x");
          defaultText.append(QString::number(height));
          defaultText.append(")");
          p.drawText(QRect(x, y, width, height), Qt::AlignCenter | Qt::TextWordWrap, defaultText);
          break;
        }
      case DRAWMODE_FREEFORM:
        if( !_userFreeForms.isEmpty() && _userFreeForms.last().active ){
          p.setPen(_userFreeForms.last().pen);

          for (int z = 0; z < _userFreeForms.last().points.size()-1; ++z) {
            QPoint current = _userFreeForms.last().points.at(z);
            QPoint next    = _userFreeForms.last().points.at(z+1);

            current = _desktopPixmapPos + current * _desktopPixmapScale;
            next = _desktopPixmapPos + next * _desktopPixmapScale;

            p.drawLine(current.x(), current.y(), next.x(), next.y());
          }
        }
        break;
      case DRAWMODE_HIGHLIGHT:
        QColor color = p.pen().color();
        p.fillRect(QRect(x, y, width, height), QColor(color.red(), color.green(), color.blue(), 75));
        break;
    }
  }

  p.end();
}

void ZoomWidget::mousePressEvent(QMouseEvent *event)
{
  // If it's writing a text and didn't saved it (by pressing Enter or
  // Escape), it removes. To disable this, just comment this if statement below
  if (_state == STATE_TYPING)
    _userTexts.removeLast();

  if(!_shiftPressed)
    _lastMousePos = event->pos();

  _state = STATE_DRAWING;
  _mousePressed = true;

  _startDrawPoint = (event->pos() - _desktopPixmapPos)/_desktopPixmapScale;
  _endDrawPoint = _startDrawPoint;
}

void ZoomWidget::mouseReleaseEvent(QMouseEvent *event)
{
  _mousePressed = false;

  if (_state == STATE_DRAWING) {
    _endDrawPoint = (event->pos() - _desktopPixmapPos)/_desktopPixmapScale;

    UserObjectData data;
    data.pen = _activePen;
    data.startPoint = _startDrawPoint;
    data.endPoint = _endDrawPoint;
    switch(_drawMode){
      case DRAWMODE_LINE:
        _userLines.append(data);
        break;
      case DRAWMODE_RECT:
        _userRects.append(data);
        break;
      case DRAWMODE_ARROW:
        _userArrows.append(data);
        break;
      case DRAWMODE_ELLIPSE:
        _userEllipses.append(data);
        break;
      case DRAWMODE_TEXT:
        {
          QFont font;
          font.setPixelSize(_activePen.width() * 4);

          UserTextData textData;
          textData.data = data;
          textData.text = "";
          textData.font = font;
          _userTexts.append(textData);

          _state = STATE_TYPING;
          update();
          return;
        }
      case DRAWMODE_FREEFORM:
        {
          // The registration of the points of the FreeForms are in mouseMoveEvent()
          UserFreeFormData data = _userFreeForms.last();
          _userFreeForms.removeLast();
          data.active = false;
          _userFreeForms.append(data);
          break;
        }
      case DRAWMODE_HIGHLIGHT:
        _userHighlights.append(data);
        break;
    }

    _state = STATE_MOVING;
    update();
  }
}

void ZoomWidget::mouseMoveEvent(QMouseEvent *event)
{
  // If the app lost focus, request it again
  if(!QWidget::isActiveWindow())
    QWidget::activateWindow();

  updateAtMousePos(event->pos());

  // Register the position of the cursor for the FreeForm
  if(_mousePressed && _drawMode == DRAWMODE_FREEFORM){
    QPoint curPos = (event->pos() - _desktopPixmapPos) / _desktopPixmapScale;

    if( _userFreeForms.isEmpty() || (!_userFreeForms.isEmpty() && !_userFreeForms.last().active) ){
      UserFreeFormData data;
      data.pen = _activePen;
      data.active = true;
      data.points.append(curPos);
      _userFreeForms.append(data);
    }

    else if(!_userFreeForms.isEmpty() && _userFreeForms.last().points.last()!=curPos){
      UserFreeFormData data = _userFreeForms.last();
      _userFreeForms.removeLast();
      data.points.append(curPos);
      _userFreeForms.append(data);
    }
  }

  update();
}

void ZoomWidget::updateAtMousePos(QPoint mousePos){
  if (!_shiftPressed){
    QPoint delta = mousePos - _lastMousePos;

    shiftPixmap(delta);
    checkPixmapPos();

    _lastMousePos = mousePos;
  }

  if (_state == STATE_DRAWING)
    _endDrawPoint = (mousePos - _desktopPixmapPos)/_desktopPixmapScale;
}

void ZoomWidget::wheelEvent(QWheelEvent *event)
{
  if (_state == STATE_MOVING) {
    int sign;
    if( event->angleDelta().y() > 0 )
      sign=1;
    else
      sign=-1;

    if(_flashlightMode && _shiftPressed) {
      _flashlightRadius -= sign * _scaleSensivity * 50;

      if( _flashlightRadius < 20)
        _flashlightRadius=20;
      if( _flashlightRadius > 180)
        _flashlightRadius=180;

      update();
      return;
    }

    if(_liveMode)
      return;

    _desktopPixmapScale += sign * _scaleSensivity;
    if (_desktopPixmapScale < 1.0f) {
      _desktopPixmapScale = 1.0f;
    }

    scalePixmapAt(event->position());
    checkPixmapPos();

    update();
  }
}

void ZoomWidget::keyPressEvent(QKeyEvent *event)
{
  int key = event->key();

  if(key == Qt::Key_Shift)
    _shiftPressed = true;

  if(_state == STATE_TYPING){
    if ((!_shiftPressed && key == Qt::Key_Return) || key == Qt::Key_Escape) {
      if( _userTexts.last().text.isEmpty() )
        _userTexts.removeLast();
      _state = STATE_MOVING;
      update();
      return;
    }

    UserTextData textData = _userTexts.last();
    if (key == Qt::Key_Backspace)
      textData.text.chop(1);
    else if(_shiftPressed && (key == Qt::Key_Return))
      textData.text += "\n";
    else
      textData.text += event->text();
    _userTexts.removeLast();
    _userTexts.append(textData);
    update();
    return;
  }

  if (key == Qt::Key_Escape) {
    if(_flashlightMode)
      _flashlightMode = false;

    else if(_desktopPixmapSize != _desktopPixmapOriginalSize){ // If it's zoomed in, go back to normal
      _desktopPixmapScale = 1.0f;

      scalePixmapAt(QPoint(0,0));
      checkPixmapPos();
    } else {
      QApplication::beep();
      QApplication::quit();
    }
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
  } else if (key == Qt::Key_O) {
    _activePen.setColor(QColor(255, 140, 0));
  } else if (key == Qt::Key_M) {
    _activePen.setColor(QColor(255, 0, 255));
  } else if (key == Qt::Key_Y) {
    _activePen.setColor(QColor(255, 255, 0));
  } else if (key == Qt::Key_W) {
    _activePen.setColor(QColor(255, 255, 255));
  } else if (key == Qt::Key_D) {
    _activePen.setColor(QColor(0, 0, 0));
  } else if (key == Qt::Key_U) {
    if( _drawMode == DRAWMODE_LINE && (! _userLines.isEmpty()) )
      _userLines.removeLast();
    else if( _drawMode == DRAWMODE_RECT && (! _userRects.isEmpty()) )
      _userRects.removeLast();
    else if( _drawMode == DRAWMODE_ARROW && (! _userArrows.isEmpty()) )
      _userArrows.removeLast();
    else if( _drawMode == DRAWMODE_ELLIPSE && (! _userEllipses.isEmpty()) )
      _userEllipses.removeLast();
    else if( _drawMode == DRAWMODE_TEXT && (! _userTexts.isEmpty()) )
      _userTexts.removeLast();
    else if( _drawMode == DRAWMODE_FREEFORM && (! _userFreeForms.isEmpty()) )
      _userFreeForms.removeLast();
    else if( _drawMode == DRAWMODE_HIGHLIGHT && (! _userHighlights.isEmpty()) )
      _userHighlights.removeLast();
  } else if (key == Qt::Key_Q) {
    _userRects.clear();
    _userLines.clear();
    _userArrows.clear();
    _userEllipses.clear();
    _userTexts.clear();
    _userFreeForms.clear();
    _userHighlights.clear();
    _state = STATE_MOVING;
  } else if (key == Qt::Key_P) {
    _boardMode = !_boardMode;
    if(_boardMode)
      _drawnPixmap.fill("#2C2C2C");
    else
      _drawnPixmap = _desktopPixmap;
  } else if (key == Qt::Key_Z) {
    _drawMode = DRAWMODE_LINE;
  } else if (key == Qt::Key_X) {
    _drawMode = DRAWMODE_RECT;
  } else if (key == Qt::Key_A) {
    _drawMode = DRAWMODE_ARROW;
  } else if (key == Qt::Key_E) {
    _drawMode = DRAWMODE_ELLIPSE;
  } else if (key == Qt::Key_T) {
    _drawMode = DRAWMODE_TEXT;
  } else if (key == Qt::Key_F) {
    _drawMode = DRAWMODE_FREEFORM;
  } else if (key == Qt::Key_H) {
    _drawMode = DRAWMODE_HIGHLIGHT;
  } else if (key == Qt::Key_S) {
    QApplication::beep();

    // Screenshot
    QPixmap screenshot = _desktopScreen->grabWindow(0);

    // Path
    QString pathFile = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (pathFile.isEmpty())
      pathFile = QDir::currentPath();

    // File
    const QString date = QDateTime::currentDateTime().toString("dd-MM-yyyy hh.mm.ss");
    const QString format = "png";
    pathFile.append("/ZoomMe ");
    pathFile.append(date);
    pathFile.append("." + format);

    // Save screenshot
    screenshot.save(pathFile);
  } else if (key == Qt::Key_Period) {
    _flashlightMode = !_flashlightMode;
  }

  update();
}

void ZoomWidget::keyReleaseEvent(QKeyEvent *event)
{
  if(event->key() == Qt::Key_Shift) {
    _shiftPressed = false;
    updateAtMousePos(mapFromGlobal(QCursor::pos()));
    update();
  }
}

void ZoomWidget::grabDesktop(bool liveMode)
{
  _liveMode = liveMode;

  _desktopPixmap = _desktopScreen->grabWindow(0);
  _drawnPixmap = _desktopPixmap;

  if(!liveMode)
    showFullScreen();

  // If there's Scaling enabled in the PC, scale the Window to the "scaled resolution"
  // This cause a pixelated image of the desktop. But if I don't do this, the program
  // would be always a little zoomed in
  // if(_desktopPixmap.size() != _desktopPixmapSize)
  //   _desktopPixmap = _desktopPixmap.scaled(_desktopPixmapSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  // This statement was replaced with a better approach with `_desktopPixmapOriginalSize`
  // that takes in count the correct original size of the screen instead of scaling it
  // down, that caused a loss in quality
}

bool ZoomWidget::grabImage(QString path){
  _desktopPixmap = QPixmap(path);

  if(_desktopPixmap.isNull())
    return false;

  // If it has transparency, fill it with some color
  if (_desktopPixmap.hasAlpha()) {
    QPixmap background(_desktopPixmap.size());
    background.fill("#2C2C2C");

    QPainter painter(&background);
    painter.drawPixmap(0, 0, _desktopPixmap);

    _desktopPixmap = background;
  }

  _drawnPixmap = _desktopPixmap;
  return true;
}

void ZoomWidget::shiftPixmap(const QPoint delta)
{
  int newX = _desktopPixmapPos.x() - delta.x() * (_desktopPixmapSize.width() / _desktopPixmapOriginalSize.width());
  int newY = _desktopPixmapPos.y() - delta.y() * (_desktopPixmapSize.height() / _desktopPixmapOriginalSize.height());
  _desktopPixmapPos.setX(newX);
  _desktopPixmapPos.setY(newY);
}

void ZoomWidget::scalePixmapAt(const QPointF pos)
{
  int old_w = _desktopPixmapSize.width();
  int old_h = _desktopPixmapSize.height();

  int new_w = _desktopPixmapOriginalSize.width() * _desktopPixmapScale;
  int new_h = _desktopPixmapOriginalSize.height() * _desktopPixmapScale;
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
