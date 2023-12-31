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

ZoomWidget::ZoomWidget(QWidget *parent) : QWidget(parent), ui(new Ui::zoomwidget)
{
  ui->setupUi(this);
  setMouseTracking(true);

  _state = STATE_MOVING;

  _desktopScreen = QGuiApplication::screenAt(QCursor::pos());
  _screenSize = _desktopScreen->geometry().size();
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
  _onlyShowDesktop = false;

  _activePen.setColor(QCOLOR_RED);
  _activePen.setWidth(4);
}

ZoomWidget::~ZoomWidget()
{
  delete ui;
}

void drawArrowHead(int x, int y, int width, int height, QPainter *p)
{
  float opposite=-1 * height;
  float adjacent=width;
  float hypotenuse=sqrt(pow(opposite,2) + pow(adjacent,2));

  float angle;
  if(adjacent==0) angle=M_PI/2;
  else angle = atanf(fabs(opposite) / fabs(adjacent));

  if( opposite>=0 && adjacent<0 )
    angle = M_PI-angle;
  else if( opposite<0 && adjacent<=0 )
    angle = M_PI+angle;
  else if( opposite<=0 && adjacent>0 )
    angle = 2*M_PI-angle;

  // This proportion determines the inclination of the arrowhead's lines.
  // For example, when the arrow is horizontal, the X and Y lengths of the arrow
  // should be the same. When it's at a 45º angle, the right-side line of the arrowhead
  // in X should be 0%, while in Y it should be 100%. When the arrow is at 90º,
  // the results should be the same as when it was horizontal, and so on...

  // I concluded that the Y-axis should be the opposite of the X-axis on the right-
  // side line of the arrowhead, and the left-side line of the arrowhead is
  // the opposite of the right-side line of the arrowhead.

  // I've simplified this behavior with a sinusoidal function, so that:
  // 0º = 0.5 (min); 45º = 1 (max); 90º = 0.5(min); etc.
  float lengthProportion = 0.25 * sin(4*angle-(M_PI/2)) + 0.75;

  // The line's length of the arrow head is a 15% of the main line size
  int lineLength = hypotenuse * 0.15;
  if (lineLength>85) lineLength=85;

  // Tip of the line where the arrow head should be drawn
  int originX=width+x, originY=height+y;

  int rightLineX  = lineLength,
      rightLineY  = lineLength,
      leftLineX = lineLength,
      leftLineY = lineLength;

  // Multiple the size with the direction in the axis
  rightLineX *= (angle<=(  M_PI/4)) || (angle>(5*M_PI/4)) ? -1 : 1;
  rightLineY *= (angle<=(3*M_PI/4)) || (angle>(7*M_PI/4)) ? 1 : -1;
  leftLineX  *= (angle<=(3*M_PI/4)) || (angle>(7*M_PI/4)) ? -1 : 1;
  leftLineY  *= (angle<=(1*M_PI/4)) || (angle>(5*M_PI/4)) ? -1 : 1;

  // Multiply the size with the proportion
  if( (angle<=(M_PI/2)) || (angle>M_PI && angle<=(3*M_PI/2)) ) {
    rightLineX *= (1-lengthProportion); rightLineY *= lengthProportion;
    leftLineX  *= lengthProportion;     leftLineY  *= (1-lengthProportion);
  } else{
    rightLineX *= lengthProportion;     rightLineY *= (1-lengthProportion);
    leftLineX  *= (1-lengthProportion); leftLineY  *= lengthProportion;
  }

  p->drawLine(originX, originY, originX+rightLineX, originY+rightLineY);
  p->drawLine(originX, originY, originX+leftLineX, originY+leftLineY);
}

bool ZoomWidget::isDrawingHovered(int drawType, int vectorPos)
{
  // Only if it's deleting or if it's trying to modify a text
  bool isDeleting       = (_state == STATE_DELETING);
  if(!isDeleting && !isInEditTextMode)
    return false;

  // This is the position of the form (in the current draw mode) in the vector,
  // that is behind the cursor.
  int posFormBehindCursor = cursorOverForm(mapFromGlobal(QCursor::pos()));

  return (_drawMode == drawType) && (posFormBehindCursor==vectorPos);
}

QColor invertColor(QColor color){
  color.setRed(255 - color.red());
  color.setGreen(255 - color.green());
  color.setBlue(255 - color.blue());

  return color;
}

void invertColorPainter(QPainter *painter)
{
  QPen pen = painter->pen();
  QColor color = invertColor(pen.color());

  pen.setColor(color);
  painter->setPen(pen);
}

void ZoomWidget::drawStatus(QPainter *screenPainter)
{
  if(_onlyShowDesktop)
    return;

  const int lineHeight = 25;
  const int padding    = 20;
  const int penWidth   = 5;
  const int fontSize   = 16;
  const int w          = 140;

  int h = 0;

  // Text to display
  QString text;

  // Line 1
  h += lineHeight;
  switch(_drawMode) {
    case DRAWMODE_LINE:      text.append("Line");        break;
    case DRAWMODE_RECT:      text.append("Rectangle");   break;
    case DRAWMODE_HIGHLIGHT: text.append("Highlighter"); break;
    case DRAWMODE_ARROW:     text.append("Arrow");       break;
    case DRAWMODE_ELLIPSE:   text.append("Ellipse");     break;
    case DRAWMODE_TEXT:      text.append("Text");        break;
    case DRAWMODE_FREEFORM:  text.append("Free Form");   break;
  }
  text.append(" (");
  text.append(QString::number(_activePen.width()));
  text.append(")");

  // Line 2
  switch(_state) {
    case STATE_MOVING:   break;
    case STATE_DRAWING:  break;
    case STATE_TYPING:   text.append("\n-- TYPING --");   h += lineHeight; break;
    case STATE_DELETING: text.append("\n-- DELETING --"); h += lineHeight; break;
  };
  if(isInEditTextMode){
    text += "\n-- SELECT --";
    h += lineHeight;
  }

  // Line 3
  // You can't forget that you have enabled the board mode, as you can clearly
  // see that there's no desktop
  // if(_boardMode){
  //   text += "\n# Black board #";
  //   h += lineHeight;
  // }

  // Line 4
  // You can't forget that you have enabled the flashlight effect, as you can
  // clearly see it
  // if(_flashlightMode){
  //   text += "\n# Flashlight #";
  //   h += lineHeight;
  // }

  // Position
  const int x = _screenSize.width() - w - padding;
  const int y = padding;

  // If the mouse is near the hit box, don't draw it
  QRect hitBox = QRect(x-padding, y-padding, w+padding*2, h+padding*2);
  if( isCursorInsideHitBox( hitBox.x(),
                            hitBox.y(),
                            hitBox.width(),
                            hitBox.height(),
                            mapFromGlobal(QCursor::pos()),
                            true) ) {
    return;
  }

  const QRect rect = QRect(x, y, w, h);

  // Settings
  screenPainter->setPen(_activePen);
  QFont font; font.setPixelSize(fontSize); screenPainter->setFont(font);
  changePenWidthFromPainter(screenPainter, penWidth);

  // Background (highlight) to improve contrast
  QColor color = (_activePen.color() == QCOLOR_BLACK) ? QCOLOR_WHITE : QCOLOR_BLACK;
  color.setAlpha(175); // Transparency
  screenPainter->fillRect(rect, color);

  // Background (highlight) for current color
  color = _activePen.color();
  color.setAlpha(65); // Transparency
  screenPainter->fillRect(rect, color);

  // Border
  screenPainter->drawRect(rect);

  // Text
  screenPainter->drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, text);
}

void ZoomWidget::drawSavedForms(QPainter *pixmapPainter)
{
  if(_onlyShowDesktop)
    return;

  // Draw user rectangles.
  int x, y, w, h;
  for (int i = 0; i < _userRects.size(); ++i) {
    pixmapPainter->setPen(_userRects.at(i).pen);
    getRealUserObjectPos(_userRects.at(i), &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_RECT, i))
      invertColorPainter(pixmapPainter);

    pixmapPainter->drawRect(x, y, w, h);
  }

  // Draw user Highlights
  for (int i = 0; i < _userHighlights.size(); ++i) {
    pixmapPainter->setPen(_userHighlights.at(i).pen);
    getRealUserObjectPos(_userHighlights.at(i), &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_HIGHLIGHT, i))
      invertColorPainter(pixmapPainter);

    QColor color = pixmapPainter->pen().color();
    color.setAlpha(75); // Transparency
    pixmapPainter->fillRect(QRect(x, y, w, h), color);
  }

  // Draw user lines.
  for (int i = 0; i < _userLines.size(); ++i) {
    pixmapPainter->setPen(_userLines.at(i).pen);
    getRealUserObjectPos(_userLines.at(i), &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_LINE, i))
      invertColorPainter(pixmapPainter);

    pixmapPainter->drawLine(x, y, x+w, y+h);
  }

  // Draw user arrows.
  for (int i = 0; i < _userArrows.size(); ++i) {
    pixmapPainter->setPen(_userArrows.at(i).pen);
    getRealUserObjectPos(_userArrows.at(i), &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_ARROW, i))
      invertColorPainter(pixmapPainter);

    pixmapPainter->drawLine(x, y, x+w, y+h);
    drawArrowHead(x, y, w, h, pixmapPainter);
  }

  // Draw user ellipses.
  for (int i = 0; i < _userEllipses.size(); ++i) {
    pixmapPainter->setPen(_userEllipses.at(i).pen);
    getRealUserObjectPos(_userEllipses.at(i), &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_ELLIPSE, i))
      invertColorPainter(pixmapPainter);

    pixmapPainter->drawEllipse(x, y, w, h);
  }

  // Draw user FreeForms.
  // If the last one is currently active, draw it in the "active forms" switch
  int freeFormCount = (!_userFreeForms.isEmpty() && _userFreeForms.last().active) ? _userFreeForms.size()-1: _userFreeForms.size();
  for (int i = 0; i < freeFormCount; ++i) {
    pixmapPainter->setPen(_userFreeForms.at(i).pen);

    if(isDrawingHovered(DRAWMODE_FREEFORM, i))
      invertColorPainter(pixmapPainter);

    for (int z = 0; z < _userFreeForms.at(i).points.size()-1; ++z) {
      QPoint current = _userFreeForms.at(i).points.at(z);
      QPoint next    = _userFreeForms.at(i).points.at(z+1);

      pixmapPainter->drawLine(current.x(), current.y(), next.x(), next.y());
    }
  }

  // Draw user Texts.
  // If the last one is currently active (user is typing), draw it in the
  // "active text" `if` statement
  int textsCount = _state == STATE_TYPING ? _userTexts.size()-1 : _userTexts.size();
  for (int i = 0; i < textsCount; ++i) {
    pixmapPainter->setPen(_userTexts.at(i).data.pen);
    pixmapPainter->setFont(_userTexts.at(i).font);
    getRealUserObjectPos(_userTexts.at(i).data, &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_TEXT, i))
      invertColorPainter(pixmapPainter);

    QString text = _userTexts.at(i).text;
    pixmapPainter->drawText(QRect(x, y, w, h), Qt::AlignCenter | Qt::TextWordWrap, text);
  }
}

void ZoomWidget::drawFlashlightEffect(QPainter *screenPainter)
{
  QPoint c = mapFromGlobal(QCursor::pos());
  int radius = _flashlightRadius;

  QRect mouseFlashlightBorder = QRect(c.x()-radius, c.y()-radius, radius*2, radius*2);
  QPainterPath mouseFlashlight;
  mouseFlashlight.addEllipse( mouseFlashlightBorder );

  // painter->setPen(QColor(186,186,186,200));
  // painter->drawEllipse( mouseFlashlightBorder );

  QPainterPath pixmapPath;
  pixmapPath.addRect(_drawnPixmap.rect());

  QPainterPath flashlightArea = pixmapPath.subtracted(mouseFlashlight);
  screenPainter->fillPath(flashlightArea, QColor(  0,  0,  0, 190));
}

void ZoomWidget::drawActiveForm(QPainter *painter, bool drawToScreen)
{
  if(_onlyShowDesktop)
    return;

  // If it's writing the text (active text)
  if(_state == STATE_TYPING) {
    UserTextData textObject = _userTexts.last();
    int x, y, w, h;
    painter->setPen(textObject.data.pen);
    painter->setFont(textObject.font);
    getRealUserObjectPos(textObject.data, &x, &y, &w, &h, drawToScreen);

    QString text = textObject.text;
    if(text.isEmpty()) text="Type some text... \nThen press Enter to finish...";
    else text.insert(textObject.caretPos, '|');
    painter->drawText(QRect(x, y, w, h), Qt::AlignCenter | Qt::TextWordWrap, text);

    changePenWidthFromPainter(painter, 1);
    painter->drawRect(x, y, w, h);
  }

  // Draw active user object.
  if(_state == STATE_DRAWING) {
    painter->setPen(_activePen);

    QPoint startPoint = _startDrawPoint;
    QPoint endPoint   = _endDrawPoint;

    if(drawToScreen){
      startPoint = pixmapPointToScreenPos(startPoint);
      endPoint   = pixmapPointToScreenPos(endPoint);
    }

    int x = startPoint.x();
    int y = startPoint.y();
    int width  = endPoint.x() - startPoint.x();
    int height = endPoint.y() - startPoint.y();

    switch(_drawMode) {
      case DRAWMODE_RECT:
        painter->drawRect(x, y, width, height);
        break;
      case DRAWMODE_LINE:
        painter->drawLine(x, y, width + x, height + y);
        break;
      case DRAWMODE_ARROW:
        painter->drawLine(x, y, width + x, height + y);
        drawArrowHead(x, y, width, height, painter);
        break;
      case DRAWMODE_ELLIPSE:
        painter->drawEllipse(x, y, width, height);
        break;
      case DRAWMODE_TEXT:
        {
          changePenWidthFromPainter(painter, 1);
          QFont font; font.setPixelSize(_activePen.width() * 4); painter->setFont(font);
          painter->drawRect(x, y, width, height);
          QString defaultText;
          defaultText.append("Sizing... (");
          defaultText.append(QString::number(width));
          defaultText.append("x");
          defaultText.append(QString::number(height));
          defaultText.append(")");
          painter->drawText(QRect(x, y, width, height), Qt::AlignCenter | Qt::TextWordWrap, defaultText);
          break;
        }
      case DRAWMODE_FREEFORM:
        if(_userFreeForms.isEmpty())
          break;
        if(!_userFreeForms.last().active)
          break;

        painter->setPen(_userFreeForms.last().pen);
        for (int z = 0; z < _userFreeForms.last().points.size()-1; ++z) {
          QPoint current = _userFreeForms.last().points.at(z);
          QPoint next    = _userFreeForms.last().points.at(z+1);

          if(drawToScreen){
            current = pixmapPointToScreenPos(current);
            next = pixmapPointToScreenPos(next);
          }

          painter->drawLine(current.x(), current.y(), next.x(), next.y());
        }
        break;
      case DRAWMODE_HIGHLIGHT:
        QColor color = painter->pen().color();
        color.setAlpha(75); // Transparency
        painter->fillRect(QRect(x, y, width, height), color);
        break;
    }
  }
}

void ZoomWidget::paintEvent(QPaintEvent *event)
{
  (void) event;

  _drawnPixmap = QPixmap(_desktopPixmap.size());
  _drawnPixmap.fill( (_liveMode) ? Qt::transparent : BLACKBOARD_COLOR );

  QPainter pixmapPainter(&_drawnPixmap);
  QPainter screen; screen.begin(this);

  if(!_liveMode && !_boardMode)
    drawDesktop(pixmapPainter);

  drawSavedForms(&pixmapPainter);

  // By drawing the active form in the pixmap, it gives a better user feedback
  // (because the user can see how it would really look like when saved), but
  // when the flashlight effect is on, its better to draw the active form onto
  // the screen, on top of the flashlight effect, so that the user can see the
  // active form over the opaque background. This can cause some differences
  // with the final result (like the width of the pen and the size of the
  // arrow's head)
  // By the way, ¿Why would you draw when the flashlight effect is enabled? I
  // don't know why I'm allowing this... You can't even see the cursor!
  if(_flashlightMode){
    drawDrawnPixmap(screen);
    drawFlashlightEffect(&screen);
    drawActiveForm(&screen, true);
  } else{
    drawActiveForm(&pixmapPainter, false);
    drawDrawnPixmap(screen);
  }

  drawStatus(&screen);

  // ONLY FOR DEBUG PURPOSE OF THE HIT BOX
  // int x, y, w, h;
  // for (int i = 0; i < _userTests.size(); ++i) {
  //   screen.setPen(_userTests.at(i).pen);
  //   getRealUserObjectPos(_userTests.at(i), &x, &y, &w, &h, false);
  //
  //   screen.drawRect(x, y, w, h);
  // }
  ///////////////////////////////////

  screen.end();
  pixmapPainter.end();
}

void ZoomWidget::removeFormBehindCursor(QPoint cursorPos)
{
  // This is the position of the form (in the current draw mode) in the vector,
  // that is behind the cursor.
  int formPosBehindCursor = cursorOverForm(cursorPos);

  if(formPosBehindCursor == -1)
    return;

  switch(_drawMode) {
    case DRAWMODE_LINE:      _userLines.remove(formPosBehindCursor);      break;
    case DRAWMODE_RECT:      _userRects.remove(formPosBehindCursor);      break;
    case DRAWMODE_HIGHLIGHT: _userHighlights.remove(formPosBehindCursor); break;
    case DRAWMODE_ARROW:     _userArrows.remove(formPosBehindCursor);     break;
    case DRAWMODE_ELLIPSE:   _userEllipses.remove(formPosBehindCursor);   break;
    case DRAWMODE_TEXT:      _userTexts.remove(formPosBehindCursor);      break;
    case DRAWMODE_FREEFORM:  _userFreeForms.remove(formPosBehindCursor);  break;
  }

  _state = STATE_MOVING;
  updateCursorShape();
  update();
}

void ZoomWidget::mousePressEvent(QMouseEvent *event)
{
  if(_onlyShowDesktop)
    return;

  _mousePressed = true;

  // If it's writing a text and didn't saved it (by pressing Enter or
  // Escape), it removes. To disable this, just comment this if statement below
  if (_state == STATE_TYPING)
    _userTexts.removeLast();

  if(_state == STATE_DELETING)
    return removeFormBehindCursor(event->pos());

  // If you're in text mode (without drawing nor writing) and you press a text
  // with shift pressed, you access it and you can modify it
  if(isTextEditable(event->pos())) {
    _state = STATE_TYPING;
    updateCursorShape();

    int formPosBehindCursor = cursorOverForm(event->pos());
    UserTextData textData = _userTexts.at(formPosBehindCursor);
    _userTexts.remove(formPosBehindCursor);
    _userTexts.append(textData);

    update();
    return;
  }

  if(!_shiftPressed)
    _lastMousePos = event->pos();

  _state = STATE_DRAWING;

  _startDrawPoint = screenPointToPixmapPos(event->pos());
  _endDrawPoint = _startDrawPoint;
}

void ZoomWidget::mouseReleaseEvent(QMouseEvent *event)
{
  if(_onlyShowDesktop)
    return;

  _mousePressed = false;

  if (_state != STATE_DRAWING)
    return;

  _endDrawPoint = screenPointToPixmapPos(event->pos());

  UserObjectData data;
  data.pen = _activePen;
  data.startPoint = _startDrawPoint;
  data.endPoint = _endDrawPoint;
  switch(_drawMode) {
    case DRAWMODE_LINE:      _userLines.append(data);      break;
    case DRAWMODE_RECT:      _userRects.append(data);      break;
    case DRAWMODE_HIGHLIGHT: _userHighlights.append(data); break;
    case DRAWMODE_ARROW:     _userArrows.append(data);     break;
    case DRAWMODE_ELLIPSE:   _userEllipses.append(data);   break;
    case DRAWMODE_TEXT:
      {
        QFont font;
        font.setPixelSize(_activePen.width() * 4);

        UserTextData textData;
        textData.data = data;
        textData.text = "";
        textData.font = font;
        textData.caretPos = 0;
        _userTexts.append(textData);

        _state = STATE_TYPING;
        update();
        return;
      }
    case DRAWMODE_FREEFORM:
      {
        // The registration of the points of the FreeForms are in mouseMoveEvent()
        // This only indicates that the drawing is no longer being actively drawn
        UserFreeFormData data = _userFreeForms.last();
        _userFreeForms.removeLast();
        data.active = false;
        _userFreeForms.append(data);
        break;
      }
  }

  _state = STATE_MOVING;
  update();
}

void ZoomWidget::updateCursorShape()
{
  if(_state == STATE_DELETING) {
    setCursor(QCursor(Qt::PointingHandCursor));
    return;
  }

  if( isTextEditable(mapFromGlobal(QCursor::pos())) ) {
    setCursor(QCursor(Qt::PointingHandCursor));
    return;
  }

  if(_flashlightMode) {
    setCursor(QCursor(Qt::BlankCursor));
    return;
  }

  setCursor(QCursor(Qt::CrossCursor));
}

void ZoomWidget::mouseMoveEvent(QMouseEvent *event)
{
  // If the app lost focus, request it again
  if(!QWidget::isActiveWindow())
    QWidget::activateWindow();

  updateCursorShape();

  updateAtMousePos(event->pos());

  if(_onlyShowDesktop){
    update();
    return;
  }

  // Register the position of the cursor for the FreeForm
  if(_mousePressed && _drawMode == DRAWMODE_FREEFORM) {
    QPoint curPos = screenPointToPixmapPos(event->pos());

    if( _userFreeForms.isEmpty() || (!_userFreeForms.isEmpty() && !_userFreeForms.last().active) ) {
      UserFreeFormData data;
      data.pen = _activePen;
      data.active = true;
      data.points.append(curPos);
      _userFreeForms.append(data);
    }

    else if(!_userFreeForms.isEmpty() && _userFreeForms.last().points.last()!=curPos) {
      UserFreeFormData data = _userFreeForms.last();
      _userFreeForms.removeLast();
      data.points.append(curPos);
      _userFreeForms.append(data);
    }
  }

  update();
}

void ZoomWidget::updateAtMousePos(QPoint mousePos)
{
  if (!_shiftPressed) {
    QPoint delta = mousePos - _lastMousePos;

    shiftPixmap(delta);
    checkPixmapPos();

    _lastMousePos = mousePos;
  }

  if (_state == STATE_DRAWING)
    _endDrawPoint = screenPointToPixmapPos(mousePos);
}

void ZoomWidget::wheelEvent(QWheelEvent *event)
{
  if (_state != STATE_MOVING && _state != STATE_DELETING)
    return;

  int sign = (event->angleDelta().y() > 0) ? 1 : -1;

  // Adjust flashlight radius
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
  if (_desktopPixmapScale < 1.0f)
    _desktopPixmapScale = 1.0f;

  scalePixmapAt(event->position());
  checkPixmapPos();

  update();
}

void ZoomWidget::saveScreenshot()
{
  // Screenshot
  QPixmap screenshot = _drawnPixmap;

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

  // Beep for fun :)
  QApplication::beep();
}

bool ZoomWidget::isCursorInsideHitBox(int x, int y, int w, int h, QPoint cursorPos, bool isFloating)
{
  // Minimum size of the hit box
  int minimumSize =  25;
  if(!isFloating)
    minimumSize *= _desktopPixmapScale;

  if(abs(w) < minimumSize) {
    int direction = (w >= 0) ? 1 : -1;
    x -= (minimumSize*direction-w)/2;
    w = minimumSize * direction;
  }
  if(abs(h) < minimumSize) {
    int direction = (h >= 0) ? 1 : -1;
    y -= (minimumSize*direction-h)/2;
    h = minimumSize * direction;
  }

  // ONLY FOR DEBUG PURPOSE OF THE HIT BOX
  // UserObjectData data;
  // data.pen = QColor(Qt::blue);
  // data.startPoint = QPoint(x,y);
  // data.endPoint = QPoint(w+x,h+y);
  // _userTests.append(data);
  ///////////////////////////////////

  QRect hitBox = QRect(x, y, w, h);
  return hitBox.contains(cursorPos);
}

int ZoomWidget::cursorOverForm(QPoint cursorPos)
{
  // ONLY FOR DEBUG PURPOSE OF THE HIT BOX
  // _userTests.clear();
  /////////////////////////
  int x, y, w, h;
  switch(_drawMode) {
    case DRAWMODE_LINE:
      for (int i = 0; i < _userLines.size(); ++i) {
        getRealUserObjectPos(_userLines.at(i), &x, &y, &w, &h, true);
        if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
          return i;
      }
      break;
    case DRAWMODE_RECT:
      for (int i = 0; i < _userRects.size(); ++i) {
        getRealUserObjectPos(_userRects.at(i), &x, &y, &w, &h, true);
        if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
          return i;
      }
      break;
    case DRAWMODE_HIGHLIGHT:
      for (int i = 0; i < _userHighlights.size(); ++i) {
        getRealUserObjectPos(_userHighlights.at(i), &x, &y, &w, &h, true);
        if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
          return i;
      }
      break;
    case DRAWMODE_ARROW:
      for (int i = 0; i < _userArrows.size(); ++i) {
        getRealUserObjectPos(_userArrows.at(i), &x, &y, &w, &h, true);
        if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
          return i;
      }
      break;
    case DRAWMODE_ELLIPSE:
      for (int i = 0; i < _userEllipses.size(); ++i) {
        getRealUserObjectPos(_userEllipses.at(i), &x, &y, &w, &h, true);
        if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
          return i;
      }
      break;
    case DRAWMODE_TEXT:
      for (int i = 0; i < _userTexts.size(); ++i) {
        getRealUserObjectPos(_userTexts.at(i).data, &x, &y, &w, &h, true);
        if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
          return i;
      }
      break;
    case DRAWMODE_FREEFORM:
      for (int i = 0; i < _userFreeForms.size(); ++i) {
        for(int z = 0; z < _userFreeForms.at(i).points.size()-1; ++z) {
          QPoint current = _userFreeForms.at(i).points.at(z);
          QPoint next    = _userFreeForms.at(i).points.at(z+1);

          current = pixmapPointToScreenPos(current);
          next = pixmapPointToScreenPos(next);

          x = current.x();
          y = current.y();
          w = next.x() - x;
          h = next.y() - y;

          if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
            return i;
        }
      }
      break;
  }
  return -1;
}

void ZoomWidget::keyPressEvent(QKeyEvent *event)
{
  int key = event->key();

  if(key == Qt::Key_Shift)
    _shiftPressed = true;

  if(_state == STATE_TYPING) {
    // If it's pressed Enter (without Shift) or Escape
    if ((!_shiftPressed && key == Qt::Key_Return) || key == Qt::Key_Escape) {
      if( _userTexts.last().text.isEmpty() )
        _userTexts.removeLast();
      _state = STATE_MOVING;
      update();
      return;
    }

    UserTextData textData = _userTexts.last();
    switch(key) {
      case Qt::Key_Backspace:
        textData.caretPos--;
        textData.text.remove(textData.caretPos, 1);
        break;
      case Qt::Key_Return:
        // Shift IS pressed in here because if it wasn't pressed it would fall
        // into the previous ´if´ statement
        textData.text.insert(textData.caretPos, '\n');
        textData.caretPos++;
        break;
      case Qt::Key_Left:
        if(textData.caretPos == 0) return;
        textData.caretPos--;
        break;
      case Qt::Key_Right:
        if(textData.caretPos == textData.text.size()) return;
        textData.caretPos++;
        break;
      case Qt::Key_Up:
        for(int i = textData.caretPos-1; i > 0; --i) {
          if(textData.text.at(i-1) == '\n') {
            textData.caretPos = i;
            break;
          }
          if(i == 1) textData.caretPos = 0;
        }
        break;
      case Qt::Key_Down:
        for(int i = textData.caretPos+1; i <= textData.text.size(); ++i) {
          if(textData.text.at(i-1) == '\n' || i == textData.text.size()) {
            textData.caretPos = i;
            break;
          }
        }
        break;
      default:
        if(event->text().isEmpty())
          break;
        textData.text.insert(textData.caretPos, event->text());
        textData.caretPos++;
        break;
    }
    _userTexts.removeLast();
    _userTexts.append(textData);
    update();
    return;
  }

  switch(key) {
    case Qt::Key_R: _activePen.setColor(QCOLOR_RED);     break;
    case Qt::Key_G: _activePen.setColor(QCOLOR_GREEN);   break;
    case Qt::Key_B: _activePen.setColor(QCOLOR_BLUE);    break;
    case Qt::Key_C: _activePen.setColor(QCOLOR_CYAN);    break;
    case Qt::Key_O: _activePen.setColor(QCOLOR_ORANGE);  break;
    case Qt::Key_M: _activePen.setColor(QCOLOR_MAGENTA); break;
    case Qt::Key_Y: _activePen.setColor(QCOLOR_YELLOW);  break;
    case Qt::Key_W: _activePen.setColor(QCOLOR_WHITE);   break;
    case Qt::Key_D: _activePen.setColor(QCOLOR_BLACK);   break;

    case Qt::Key_Z: _drawMode = DRAWMODE_LINE;      break;
    case Qt::Key_X: _drawMode = DRAWMODE_RECT;      break;
    case Qt::Key_A: _drawMode = DRAWMODE_ARROW;     break;
    case Qt::Key_E: _drawMode = DRAWMODE_ELLIPSE;   break;
    case Qt::Key_T: _drawMode = DRAWMODE_TEXT;      break;
    case Qt::Key_F: _drawMode = DRAWMODE_FREEFORM;  break;
    case Qt::Key_H: _drawMode = DRAWMODE_HIGHLIGHT; break;

    case Qt::Key_S:      saveScreenshot();        break;
    case Qt::Key_U:      undoLastDrawing();       break;
    case Qt::Key_Q:      clearAllDrawings();      break;
    case Qt::Key_P:      switchBoardMode();       break;
    case Qt::Key_Space:  switchOnlyShowDesktop(); break;
    case Qt::Key_Period: switchFlashlightMode();  break;
    case Qt::Key_Comma:  switchDeleteMode();      break;
    case Qt::Key_Escape: escapeKeyFunction();     break;

    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
      _activePen.setWidth(key - Qt::Key_0);
      break;
  }

  updateCursorShape();
  update();
}

void ZoomWidget::switchDeleteMode()
{
  if(_onlyShowDesktop)
    return;

  if(_state == STATE_MOVING)        _state = STATE_DELETING;
  else if(_state == STATE_DELETING) _state = STATE_MOVING;
}

void ZoomWidget::clearAllDrawings()
{
  if(_onlyShowDesktop)
    return;

  _userRects.clear();
  _userLines.clear();
  _userArrows.clear();
  _userEllipses.clear();
  _userTexts.clear();
  _userFreeForms.clear();
  _userHighlights.clear();
  _state = STATE_MOVING;
}

// Remove last drawing from the current draw mode
void ZoomWidget::undoLastDrawing()
{
  if(_onlyShowDesktop)
    return;

  switch(_drawMode) {
    case DRAWMODE_LINE:      if(!_userLines.isEmpty())      _userLines.removeLast();      break;
    case DRAWMODE_RECT:      if(!_userRects.isEmpty())      _userRects.removeLast();      break;
    case DRAWMODE_ARROW:     if(!_userArrows.isEmpty())     _userArrows.removeLast();     break;
    case DRAWMODE_ELLIPSE:   if(!_userEllipses.isEmpty())   _userEllipses.removeLast();   break;
    case DRAWMODE_TEXT:      if(!_userTexts.isEmpty())      _userTexts.removeLast();      break;
    case DRAWMODE_FREEFORM:  if(!_userFreeForms.isEmpty())  _userFreeForms.removeLast();  break;
    case DRAWMODE_HIGHLIGHT: if(!_userHighlights.isEmpty()) _userHighlights.removeLast(); break;
  }
}

void ZoomWidget::escapeKeyFunction()
{
  if(_state == STATE_DELETING){
    _state = STATE_MOVING;
  } else if(_flashlightMode) {
    _flashlightMode = false;
  } else if(_onlyShowDesktop) {
    _onlyShowDesktop = false;
  } else if(_desktopPixmapSize != _desktopPixmapOriginalSize) {
    _desktopPixmapScale = 1.0f;
    scalePixmapAt(QPoint(0,0));
    checkPixmapPos();
  } else {
    QApplication::beep();
    QApplication::quit();
  }
}

void ZoomWidget::keyReleaseEvent(QKeyEvent *event)
{
  if(event->key() == Qt::Key_Shift) {
    _shiftPressed = false;
    updateCursorShape();
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
}

bool ZoomWidget::grabImage(QString path, FitImage config)
{
  QPixmap img = QPixmap(path);

  if(img.isNull())
    return false;

  // Auto detect the fit config
  if(config == FIT_AUTO){
    if(img.width() > img.height()) config = FIT_TO_HEIGHT;
    else config = FIT_TO_WIDTH;
  }

  // Scale
  int width, height, x = 0, y = 0;
  if(config == FIT_TO_WIDTH){
    width = _screenSize.width();
    img = img.scaledToWidth(width, Qt::SmoothTransformation);

    // Take the largest height for the pixmap: the image or the screen height
    height = (_screenSize.height() > img.height()) ? _screenSize.height() : img.height();

    // Center the image in the screen
    if(_screenSize.height() > img.height()) y = (height - img.height()) / 2;
  } else { // FIT_TO_HEIGHT
    height = _screenSize.height();
    img = img.scaledToHeight(height, Qt::SmoothTransformation);

    // Take the largest width: the image or the screen width
    width = (_screenSize.width() > img.width()) ? _screenSize.width() : img.width();

    // Center the image in the screen
    if(_screenSize.width() > img.width()) x = (width - img.width()) / 2;
  }

  // Draw the image into the pixmap
  _desktopPixmap = QPixmap(width, height);
  _desktopPixmap.fill(Qt::transparent);
  QPainter painter(&_desktopPixmap);
  painter.drawPixmap(x, y, img);
  painter.end();

  _desktopPixmapSize = _desktopPixmap.size();
  _desktopPixmapOriginalSize = _desktopPixmapSize;

  showFullScreen();
  return true;
}

void ZoomWidget::shiftPixmap(const QPoint delta)
{
  int newY = _desktopPixmapPos.y() - delta.y() * (_desktopPixmapSize.height() / _screenSize.height());
  int newX = _desktopPixmapPos.x() - delta.x() * (_desktopPixmapSize.width() / _screenSize.width());
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

void ZoomWidget::getRealUserObjectPos(const UserObjectData &userObj, int *x, int *y, int *w, int *h, bool posRelativeToScreen)
{
  QPoint startPoint = userObj.startPoint;
  QSize size;
  size.setWidth(userObj.endPoint.x() - startPoint.x());
  size.setHeight(userObj.endPoint.y() - startPoint.y());

  if (posRelativeToScreen){
    startPoint = pixmapPointToScreenPos(startPoint);
    size = pixmapSizeToScreenSize(size);
  }

  *x = startPoint.x();
  *y = startPoint.y();
  *w = size.width();
  *h = size.height();
}
