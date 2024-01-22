#include "zoomwidget.hpp"
#include "ui_zoomwidget.h"

#include <cmath>
#include <cstdio>
#include <QPainter>
#include <QMouseEvent>
#include <QRect>
#include <QGuiApplication>
#include <QOpenGLWidget>
#include <QDir>
#include <QCursor>
#include <QDateTime>
#include <QColor>
#include <QPainterPath>
#include <QList>
#include <QImageWriter>
#include <QBuffer>

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
  _screenOpts = SCREENOPTS_SHOW_ALL;

  clipboard = QApplication::clipboard();

  recordTimer = new QTimer(this);
  connect( recordTimer,
           &QTimer::timeout,
           this,
           &ZoomWidget::saveFrameToFile );
  // Show the ffmpeg output on the screen
  ffmpeg.setProcessChannelMode(ffmpeg.ForwardedChannels);
  // Don't create a file if the setProcessChannelMode is set, because it's an
  // inconsistency
  // ffmpeg.setStandardErrorFile("ffmpeg_log.txt");
  // ffmpeg.setStandardOutputFile("ffmpeg_output.txt");
  recordTempFile = new QFile(RECORD_TMP_FILEPATH);

  _activePen.setColor(QCOLOR_RED);
  _activePen.setWidth(4);
}

ZoomWidget::~ZoomWidget()
{
  delete ui;
}

void ZoomWidget::saveStateToFile()
{
  QFile file(_saveFilePath + ".zoomme");
  if (!file.open(QIODevice::WriteOnly))
   return;

  QDataStream out(&file);
  // There should be the same arguments that the restoreStateToFile()
  out << _screenSize
      << _desktopPixmap
      // I don't want to be zoomed in when restoring
      // << _desktopPixmapPos
      // << _desktopPixmapSize
      // << _desktopPixmapScale
      << _desktopPixmapOriginalSize
      // The save path is absolute, so it is bound to the PC, so it shouldn't
      // be saved
      // << _saveFilePath
      << _imageExtension
      << _videoExtension
      << _liveMode
      // I don't think is good to save if the status bar and/or the
      // drawings are hiden
      // << _screenOpts
      << _drawMode
      << _activePen
      << _userRects.size()
      << _userLines.size()
      << _userArrows.size()
      << _userEllipses.size()
      << _userTexts.size()
      << _userFreeForms.size()
      << _userHighlights.size();

  // Save the drawings
  // Rectangles
  for(int i=0; i<_userRects.size(); i++)
      out << _userRects.at(i).startPoint << _userRects.at(i).endPoint << _userRects.at(i).pen;
  // Lines
  for(int i=0; i<_userLines.size(); i++)
      out << _userLines.at(i).startPoint << _userLines.at(i).endPoint << _userLines.at(i).pen;
  // Arrows
  for(int i=0; i<_userArrows.size(); i++)
      out << _userArrows.at(i).startPoint << _userArrows.at(i).endPoint << _userArrows.at(i).pen;
  // Ellipses
  for(int i=0; i<_userEllipses.size(); i++)
      out << _userEllipses.at(i).startPoint << _userEllipses.at(i).endPoint << _userEllipses.at(i).pen;
  // Texts
  for(int i=0; i<_userTexts.size(); i++){
    out << _userTexts.at(i).data.startPoint << _userTexts.at(i).data.endPoint << _userTexts.at(i).data.pen;
    out << _userTexts.at(i).caretPos << _userTexts.at(i).text;
  }
  // Free Forms
  for(int i=0; i<_userFreeForms.size(); i++){
    out << _userFreeForms.at(i).points.size();
    for(int x=0; x<_userFreeForms.at(i).points.size(); x++)
      out << _userFreeForms.at(i).points.at(x);
    out << _userFreeForms.at(i).pen << _userFreeForms.at(i).active;
  }
  // Highlights
  for(int i=0; i<_userHighlights.size(); i++)
      out << _userHighlights.at(i).startPoint << _userHighlights.at(i).endPoint << _userHighlights.at(i).pen;

  QApplication::beep();
}

bool ZoomWidget::restoreStateFromFile(QString path, FitImage config)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
   return false;

  long long userRectsCount      = 0,
            userLinesCount      = 0,
            userArrowsCount     = 0,
            userEllipsesCount   = 0,
            userTextsCount      = 0,
            userFreeFormsCount  = 0,
            userHighlightsCount = 0;

  QSize savedScreenSize;
  QPixmap savedPixmap;
  QSize savedPixmapSize;

  // There should be the same arguments that the saveStateToFile()
  QDataStream in(&file);
  in  >> savedScreenSize
      >> savedPixmap
      // I don't want to be zoomed in when restoring
      // >> _desktopPixmapPos
      // >> _desktopPixmapSize
      // >> _desktopPixmapScale
      >> savedPixmapSize
      // The save path is absolute, so it is bound to the PC, so it shouldn't
      // be saved
      // >> _saveFilePath
      >> _imageExtension
      >> _videoExtension
      >> _liveMode
      // I don't think is good to save if the status bar and/or the
      // drawings are hiden
      // >> _screenOpts
      >> _drawMode
      >> _activePen
      >> userRectsCount
      >> userLinesCount
      >> userArrowsCount
      >> userEllipsesCount
      >> userTextsCount
      >> userFreeFormsCount
      >> userHighlightsCount;

  // VARIABLES FOR SCALING THE DRAWINGS
  float scaleFactorX = 1.0, scaleFactorY = 1.0;
  int marginTop = 0, marginLeft = 0;

  if(savedScreenSize == _screenSize) {
    _desktopPixmapOriginalSize = savedPixmapSize;
    _desktopPixmapSize = savedPixmapSize;
    _desktopPixmap = savedPixmap;
  } else {
    if(config == FIT_AUTO)
      config = (savedPixmap.width() > savedPixmap.height()) ? FIT_TO_HEIGHT : FIT_TO_WIDTH;

    QImage scaledPixmap = savedPixmap.toImage();
    if(config == FIT_TO_WIDTH)
      scaledPixmap = scaledPixmap.scaledToWidth(_screenSize.width());
    else
      scaledPixmap = scaledPixmap.scaledToHeight(_screenSize.height());

    printf("[INFO] Scaling ZoomMe recover file...\n");
    printf("[INFO]   - Recovered screen size: %dx%d\n", savedScreenSize.width(), savedScreenSize.height());
    printf("[INFO]   - Actual screen size: %dx%d\n", _screenSize.width(), _screenSize.height());
    printf("[INFO]   - Recovered image size: %dx%d\n", savedPixmapSize.width(), savedPixmapSize.height());
    printf("[INFO]   - Scaled (actual) image size: %dx%d\n", scaledPixmap.width(), scaledPixmap.height());

    // With 'The Rule of Three'...
    // oldPixmapSize --> pointOfDrawing
    // newPixmapSize   -->     x (new -scaled- point of the drawing)
    // so...
    // x = ( newPixmapSize * pointOfDrawing ) / oldPixmapSize
    // x = pointOfDrawing * ( newPixmapSize / oldPixmapSize )
    //
    // Don't use the savedPixmapSize variable as the oldPimapSize because it
    // doesn't take in count the REAL monitor resolution when grabbing the
    // desktop (with hdpi scaling enabled), and the drawings will be misplaced.
    // Use instead savePixmap.size() that has the REAL monitor resolution
    scaleFactorX = (float)(scaledPixmap.width() ) / (float)(savedPixmap.width());
    scaleFactorY = (float)(scaledPixmap.height()) / (float)(savedPixmap.height());

    // Adjust the drawings to the margin of the image after scaling
    if(_screenSize.height() > scaledPixmap.height())
      marginTop = (_screenSize.height() - scaledPixmap.height()) / 2;
    if(_screenSize.width() > scaledPixmap.width())
      marginLeft = (_screenSize.width() - scaledPixmap.width()) / 2;

    grabImage(savedPixmap, config);
  }

  // Read the drawings
  // Rectangles
  for(int i=0; i<userRectsCount; i++){
    UserObjectData objectData;
    in >> objectData.startPoint >> objectData.endPoint >> objectData.pen;

    objectData.startPoint.setX( marginLeft + objectData.startPoint.x() * scaleFactorX );
    objectData.startPoint.setY( marginTop  + objectData.startPoint.y() * scaleFactorY );
    objectData.endPoint.setX( marginLeft + objectData.endPoint.x() * scaleFactorX );
    objectData.endPoint.setY( marginTop  + objectData.endPoint.y() * scaleFactorY );

    _userRects.append(objectData);
  }
  // Lines
  for(int i=0; i<userLinesCount; i++){
    UserObjectData objectData;
    in >> objectData.startPoint >> objectData.endPoint >> objectData.pen;

    objectData.startPoint.setX( marginLeft + objectData.startPoint.x() * scaleFactorX );
    objectData.startPoint.setY( marginTop  + objectData.startPoint.y() * scaleFactorY );
    objectData.endPoint.setX( marginLeft + objectData.endPoint.x() * scaleFactorX );
    objectData.endPoint.setY( marginTop  + objectData.endPoint.y() * scaleFactorY );

    _userLines.append(objectData);
  }
  // Arrows
  for(int i=0; i<userArrowsCount; i++){
    UserObjectData objectData;
    in >> objectData.startPoint >> objectData.endPoint >> objectData.pen;

    objectData.startPoint.setX( marginLeft + objectData.startPoint.x() * scaleFactorX );
    objectData.startPoint.setY( marginTop  + objectData.startPoint.y() * scaleFactorY );
    objectData.endPoint.setX( marginLeft + objectData.endPoint.x() * scaleFactorX );
    objectData.endPoint.setY( marginTop  + objectData.endPoint.y() * scaleFactorY );

    _userArrows.append(objectData);
  }
  // Ellipses
  for(int i=0; i<userEllipsesCount; i++){
    UserObjectData objectData;
    in >> objectData.startPoint >> objectData.endPoint >> objectData.pen;

    objectData.startPoint.setX( marginLeft + objectData.startPoint.x() * scaleFactorX );
    objectData.startPoint.setY( marginTop  + objectData.startPoint.y() * scaleFactorY );
    objectData.endPoint.setX( marginLeft + objectData.endPoint.x() * scaleFactorX );
    objectData.endPoint.setY( marginTop  + objectData.endPoint.y() * scaleFactorY );

    _userEllipses.append(objectData);
  }
  // Texts
  for(int i=0; i<userTextsCount; i++){
    UserTextData textData;
    in >> textData.data.startPoint >> textData.data.endPoint >> textData.data.pen;
    in >> textData.caretPos >> textData.text;

    textData.data.startPoint.setX( marginLeft + textData.data.startPoint.x() * scaleFactorX );
    textData.data.startPoint.setY( marginTop  + textData.data.startPoint.y() * scaleFactorY );
    textData.data.endPoint.setX( marginLeft + textData.data.endPoint.x() * scaleFactorX );
    textData.data.endPoint.setY( marginTop  + textData.data.endPoint.y() * scaleFactorY );

    _userTexts.append(textData);
  }
  // Free forms
  for(int i=0; i<userFreeFormsCount; i++){
    UserFreeFormData freeFormData;
    long long freeFormPointsCount = 0;
    in >> freeFormPointsCount;
    for(int x=0; x<freeFormPointsCount; x++){
      QPoint point;
      in >> point;

      point.setX( marginLeft + point.x() * scaleFactorX );
      point.setY( marginTop  + point.y() * scaleFactorY );

      freeFormData.points.append(point);
    }
    in >> freeFormData.pen >> freeFormData.active;
    _userFreeForms.append(freeFormData);
  }
  // Highlights
  for(int i=0; i<userHighlightsCount; i++){
    UserObjectData objectData;
    in >> objectData.startPoint >> objectData.endPoint >> objectData.pen;

    objectData.startPoint.setX( marginLeft + objectData.startPoint.x() * scaleFactorX );
    objectData.startPoint.setY( marginTop  + objectData.startPoint.y() * scaleFactorY );
    objectData.endPoint.setX( marginLeft + objectData.endPoint.x() * scaleFactorX );
    objectData.endPoint.setY( marginTop  + objectData.endPoint.y() * scaleFactorY );

    _userHighlights.append(objectData);
  }

  if(!_liveMode)
    showFullScreen();
  return true;
}

bool ZoomWidget::createVideoFFmpeg()
{
  QString resolution;
  resolution.append(QString::number(_drawnPixmap.width()));
  resolution.append("x");
  resolution.append(QString::number(_drawnPixmap.height()));

  // Read the video bytes and pipe it to FFmpeg...
  // Arguments for FFmpeg taken from:
  // https://github.com/tsoding/rendering-video-in-c-with-ffmpeg/blob/master/ffmpeg_linux.c
  QString script;

  script.append("cat ");
    script.append(RECORD_TMP_FILEPATH);

  script.append(" | ");

  script.append("ffmpeg ");
  // GLOBAL ARGS
    script.append("-loglevel "); script.append("verbose "       );
    script.append("-y "       );
  // INPUT ARGS
    // No rawvideo because it's now compressed in jpeg
    // script.append("-f "      );  script.append("rawvideo ");
    script.append("-pix_fmt " );  script.append("yuv420p "      );
    script.append("-s "       );  script.append(resolution + " ");
    script.append("-r "       );  script.append(QString::number(RECORD_FPS) + " ");
    script.append("-i "       );  script.append("- "            );
  // OUTPUT ARGS
    script.append("-c:v "     );  script.append("libx264 "      );
    script.append("-vb "      );  script.append("2500k "        );
    script.append("-c:a "     );  script.append("aac "          );
    script.append("-ab "      );  script.append("200k "         );
    script.append("-pix_fmt " );  script.append("yuv420p "      );
    script.append("'" + _saveFilePath + "." + _videoExtension + "'");

  QList<QString> arguments;
  arguments << "-c";
  arguments << script;

  // Start process
  ffmpeg.start("bash", arguments);

  updateCursorShape();

  const int timeout = 10000;
  if (!ffmpeg.waitForStarted(timeout) || !ffmpeg.waitForFinished(-1))
    return false;

  return true;
}

void ZoomWidget::saveFrameToFile()
{
  QImage image = _drawnPixmap.toImage();

  // Save the image as jpeg into a byte array (is not a raw image, it's
  // compressed)
  QByteArray imageBytes;
  QBuffer buffer(&imageBytes); buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "JPEG", RECORD_QUALITY);

  recordTempFile->write(imageBytes);
}

QRect fixQRectForText(int x, int y, int width, int height)
{
  // The width and height of the text must be positive, otherwise strange
  // things happen, like only showing the first word of the text
  if(width < 0)  { x+=width;  width=abs(width);   }
  if(height < 0) { y+=height; height=abs(height); }

  return QRect(x, y, width, height);
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
  if(!isDeleting && !IS_IN_EDIT_TEXT_MODE)
    return false;

  // This is the position of the form (in the current draw mode) in the vector,
  // that is behind the cursor.
  int posFormBehindCursor = cursorOverForm(getCursorPos(false));

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
  if(_screenOpts == SCREENOPTS_HIDE_ALL || _screenOpts == SCREENOPTS_HIDE_STATUS)
    return;

  const int lineHeight        = 25;
  const int margin            = 20;
  const int penWidth          = 5;
  const int fontSize          = 16;
  const int w                 = 160;
  const int initialLineHeight = lineHeight + 5; // lineHeight + margin

  int h = initialLineHeight;

  // Text to display
  QString text;

  // Line 1 -ALWAYS DISPLAYING-
  if(IS_DISABLED_MOUSE_TRACKING)
    text.append(BLOCK_ICON);
  else
    text.append( (_desktopPixmapScale == 1.0f) ? NO_ZOOM_ICON : ZOOM_ICON );
  text.append(" ");

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
    case STATE_MOVING:       break;
    case STATE_DRAWING:      break;
    case STATE_TYPING:       text.append("\n-- TYPING --");   h += lineHeight; break;
    case STATE_DELETING:     text.append("\n-- DELETING --"); h += lineHeight; break;
    case STATE_COLOR_PICKER: text.append("\n-- PICK COLOR --"); h += lineHeight; break;
  };
  if(IS_IN_EDIT_TEXT_MODE){
    text += "\n-- SELECT --";
    h += lineHeight;
  }

  // Last Line
  if(IS_RECORDING){
    text.append("\n");
    text.append(RECORD_ICON);
    text.append(" Recording...");

    h += lineHeight;
  }

  // You can't forget that you have enabled the board mode, as you can clearly
  // see that there's no desktop
  // if(_boardMode){
  //   text += "\n# Black board #";
  //   h += lineHeight;
  // }

  // You can't forget that you have enabled the flashlight effect, as you can
  // clearly see it
  // if(_flashlightMode){
  //   text += "\n# Flashlight #";
  //   h += lineHeight;
  // }

  // Position
  const int x = _screenSize.width() - w - margin;
  const int y = margin;

  // Image
  // const int marginLeftImage = 10;
  // const int sizeImage = h;
  // const int xImage = x - marginLeftImage - sizeImage;
  // const int yImage = y;
  //
  // If the mouse is near the hit box, don't draw it
  // QRect hitBox = QRect(
  //                       xImage - margin,
  //                       yImage - margin,
  //                       w + sizeImage + marginLeftImage + margin*2,
  //                       h + margin*2
  //                     );

  // If the mouse is near the hit box, don't draw it
  QRect hitBox = QRect(x-margin, y-margin, w+margin*2, h+margin*2);
  if( isCursorInsideHitBox( hitBox.x(),
                            hitBox.y(),
                            hitBox.width(),
                            hitBox.height(),
                            // The getCursorPos is not fixed to hdpi because the hitbox is not either relative to hdpi
                            getCursorPos(false),
                            true) ) {
    return;
  }

  const QRect rect = QRect(x, y, w, h);

  // Draw Image
  // const QImage img = QImage(":/resources/Icon.png");
  // screenPainter->drawImage(
  //                           QRect(xImage, yImage, sizeImage, sizeImage),
  //                           img,
  //                           img.rect(),
  //                           Qt::AutoColor
  //                         );

  // Settings
  screenPainter->setPen(_activePen);
  QFont font; font.setPixelSize(fontSize); screenPainter->setFont(font);
  CHANGE_PEN_WIDTH_FROM_PAINTER(screenPainter, penWidth);

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
  if(_screenOpts == SCREENOPTS_HIDE_ALL)
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
    UPDATE_FONT_SIZE(pixmapPainter);
    getRealUserObjectPos(_userTexts.at(i).data, &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_TEXT, i))
      invertColorPainter(pixmapPainter);

    QString text = _userTexts.at(i).text;
    pixmapPainter->drawText(fixQRectForText(x, y, w, h), Qt::AlignCenter | Qt::TextWordWrap, text);
  }
}

void ZoomWidget::drawFlashlightEffect(QPainter *screenPainter)
{
  QPoint c = getCursorPos(false);
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
  if(_screenOpts == SCREENOPTS_HIDE_ALL)
    return;

  // If it's writing the text (active text)
  if(_state == STATE_TYPING) {
    UserTextData textObject = _userTexts.last();
    int x, y, w, h;
    painter->setPen(textObject.data.pen);
    UPDATE_FONT_SIZE(painter);
    getRealUserObjectPos(textObject.data, &x, &y, &w, &h, drawToScreen);

    QString text = textObject.text;
    if(text.isEmpty()) text="Type some text... \nThen press Enter to finish...";
    else text.insert(textObject.caretPos, '|');
    painter->drawText(fixQRectForText(x, y, w, h), Qt::AlignCenter | Qt::TextWordWrap, text);

    CHANGE_PEN_WIDTH_FROM_PAINTER(painter, 1);
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
          UPDATE_FONT_SIZE(painter);
          CHANGE_PEN_WIDTH_FROM_PAINTER(painter, 1);
          painter->drawRect(x, y, width, height);
          QString defaultText;
          defaultText.append("Sizing... (");
          defaultText.append(QString::number(abs(width)));
          defaultText.append("x");
          defaultText.append(QString::number(abs(height)));
          defaultText.append(")");
          painter->drawText(fixQRectForText(x, y, width, height), Qt::AlignCenter | Qt::TextWordWrap, defaultText);
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

  _drawnPixmap = _desktopPixmap;

  if(_liveMode)
    _drawnPixmap.fill(Qt::transparent);

  if(_boardMode)
    _drawnPixmap.fill(BLACKBOARD_COLOR);

  QPainter pixmapPainter(&_drawnPixmap);
  QPainter screen; screen.begin(this);

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
    DRAW_DRAWN_PIXMAP(screen);
    drawFlashlightEffect(&screen);
    drawActiveForm(&screen, true);
  } else{
    drawActiveForm(&pixmapPainter, false);
    DRAW_DRAWN_PIXMAP(screen);
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

// The cursor pos shouln't be fixed to hdpi scaling
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
  (void) event;

  if(_screenOpts == SCREENOPTS_HIDE_ALL)
    return;

  _mousePressed = true;

  if(_state == STATE_COLOR_PICKER){
    _activePen.setColor(GET_COLOR_UNDER_CURSOR());
    _state = STATE_MOVING;
    update();
    return;
  }

  if (_state == STATE_TYPING && _userTexts.last().text.isEmpty())
    _userTexts.removeLast();

  if(_state == STATE_DELETING)
    return removeFormBehindCursor(getCursorPos(false));

  // If you're in text mode (without drawing nor writing) and you press a text
  // with shift pressed, you access it and you can modify it
  if(isTextEditable(getCursorPos(false))) {
    _state = STATE_TYPING;
    updateCursorShape();

    int formPosBehindCursor = cursorOverForm(getCursorPos(false));
    UserTextData textData = _userTexts.at(formPosBehindCursor);
    _userTexts.remove(formPosBehindCursor);
    _userTexts.append(textData);

    update();
    return;
  }

  if(!_shiftPressed)
    _lastMousePos = getCursorPos(false);

  _state = STATE_DRAWING;

  _startDrawPoint = screenPointToPixmapPos(getCursorPos(false));
  _endDrawPoint = _startDrawPoint;
}

void ZoomWidget::mouseReleaseEvent(QMouseEvent *event)
{
  (void) event;

  if(_screenOpts == SCREENOPTS_HIDE_ALL)
    return;

  _mousePressed = false;

  if (_state != STATE_DRAWING)
    return;

  _endDrawPoint = screenPointToPixmapPos(getCursorPos(false));

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
        UserTextData textData;
        textData.data = data;
        textData.text = "";
        textData.caretPos = 0;
        _userTexts.append(textData);

        _state = STATE_TYPING;
        _freezeDesktopPosWhileWriting = _shiftPressed;
        update();
        return;
      }
    case DRAWMODE_FREEFORM:
      {
        // BUG: If the Freeform is empty or the last one is inactive, it is because
        // the user had pressed the mouse but didn't move it, so the free form was
        // not created. This if statement fixes the segfault
        if(_userFreeForms.isEmpty())
          break;

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
  QCursor pointHand = QCursor(Qt::PointingHandCursor);
  QCursor blank     = QCursor(Qt::BlankCursor);
  QCursor waiting   = QCursor(Qt::WaitCursor);

  QPixmap pickColorPixmap(":/resources/color-picker-16.png");
  if (pickColorPixmap.isNull()) printf("[ERROR] Failed to load pixmap for custom cursor (color-picker)\n");
  QCursor pickColor = QCursor(pickColorPixmap, 0, pickColorPixmap.height()-1);

  if(IS_FFMPEG_RUNNING)
    setCursor(waiting);

  else if(_state == STATE_COLOR_PICKER)
    setCursor(pickColor);

  else if(_state == STATE_DELETING)
    setCursor(pointHand);

  else if( isTextEditable(getCursorPos(false)) )
    setCursor(pointHand);

  else if(_flashlightMode)
    setCursor(blank);

  else
    setCursor(QCursor(Qt::CrossCursor));
}

void ZoomWidget::mouseMoveEvent(QMouseEvent *event)
{
  (void) event;

  // If the app lost focus, request it again
  if(!QWidget::isActiveWindow())
    QWidget::activateWindow();

  updateCursorShape();

  updateAtMousePos(getCursorPos(false));

  if(_screenOpts == SCREENOPTS_HIDE_ALL){
    update();
    return;
  }

  if(_state == STATE_COLOR_PICKER){
    _activePen.setColor(GET_COLOR_UNDER_CURSOR());
    update();
    return;
  }

  // Register the position of the cursor for the FreeForm
  if(_mousePressed && _drawMode == DRAWMODE_FREEFORM) {
    QPoint curPos = screenPointToPixmapPos(getCursorPos(false));

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

// The mouse pos shouldn't be fixed to the hdpi scaling
void ZoomWidget::updateAtMousePos(QPoint mousePos)
{
  if(!IS_DISABLED_MOUSE_TRACKING) {
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
  if (_state == STATE_DRAWING || _state == STATE_TYPING)
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

  scalePixmapAt(getCursorPos(false));
  checkPixmapPos();

  update();
}

QString ZoomWidget::initFileConfig(QString path, QString name, QString imgExt, QString vidExt)
{
  // Path
  QDir folderPath;
  if(path.isEmpty()){
    QString picturesFolder = QStandardPaths::writableLocation(DEFAULT_FOLDER);
    folderPath = (picturesFolder.isEmpty()) ? QDir::currentPath() : picturesFolder;
  } else {
    folderPath = QDir(path);
    if(!folderPath.exists())
      return "The given path doesn't exits or it's a file";
  }

  // Check if extension is supported
  QList supportedExtensions = QImageWriter::supportedImageFormats();
  if( (!imgExt.isEmpty()) && (!supportedExtensions.contains(imgExt)) )
    return "Image extension not supported";

  // Name
  QString fileName;
  const QString date = QDateTime::currentDateTime().toString(DATE_FORMAT_FOR_FILE);
  fileName = (name.isEmpty()) ? ("ZoomMe " + date) : name;

  // Extension
  const char* defaultImgExt = "png";
  _imageExtension += (imgExt.isEmpty()) ? defaultImgExt : imgExt;
  const char* defaultVidExt = "mp4";
  _videoExtension += (vidExt.isEmpty()) ? defaultVidExt : vidExt;

  _saveFilePath = folderPath.absoluteFilePath(fileName);

  return "";
}

// The cursor pos should be fixed to the hdpi scaling if the x, y, width and
// height is relative to the REAL screen size, the hdpi one (for example, if
// it's from the pixmap). Otherwise, it should'nt be fixed to the hdpi scaling
// (for exmaple, if it's from the status bar, that is drawn onto the screen).
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

// The cursor pos shouln't be fixed to hdpi scaling, because in
// getRealUserObjectPos() they are scaled to the screen 'scaled' (no hdpi)
// resolution (and the cursor has to be relative to the same resolution that the
// drawings)
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
    case Qt::Key_T: _drawMode = DRAWMODE_TEXT;      break;
    case Qt::Key_F: _drawMode = DRAWMODE_FREEFORM;  break;
    case Qt::Key_H: _drawMode = DRAWMODE_HIGHLIGHT; break;

    case Qt::Key_E:
                    if(_shiftPressed)
                      saveStateToFile();
                    else
                      _drawMode = DRAWMODE_ELLIPSE;
                    break;

    case Qt::Key_S:
                    if(_shiftPressed)
                      SAVE_TO_CLIPBOARD();
                    else
                      SAVE_TO_IMAGE();
                    break;

    case Qt::Key_U:      undoLastDrawing();       break;
    case Qt::Key_Q:      clearAllDrawings();      break;
    case Qt::Key_Tab:    SWITCH_BOARD_MODE();       break;
    case Qt::Key_Space:  CYCLE_SCREEN_OPTS();       break;
    case Qt::Key_Period: SWITCH_FLASHLIGHT_MODE();  break;
    case Qt::Key_Comma:  switchDeleteMode();      break;
    case Qt::Key_Escape: escapeKeyFunction();     break;
    case Qt::Key_Minus:  toggleRecording();       break;
    case Qt::Key_P:      pickColorMode();         break;

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

void ZoomWidget::pickColorMode()
{
  if(_state == STATE_COLOR_PICKER) {
    _state = STATE_MOVING;
    _activePen.setColor(_colorBeforePickColorMode);
    return;
  }

  _colorBeforePickColorMode = _activePen.color();

  if(_screenOpts == SCREENOPTS_HIDE_ALL || _state != STATE_MOVING)
    return;

  _state = STATE_COLOR_PICKER;

  _activePen.setColor(GET_COLOR_UNDER_CURSOR());
}

void ZoomWidget::switchDeleteMode()
{
  if(_screenOpts == SCREENOPTS_HIDE_ALL)
    return;

  if(_state == STATE_MOVING)        _state = STATE_DELETING;
  else if(_state == STATE_DELETING) _state = STATE_MOVING;
}

void ZoomWidget::toggleRecording()
{
  // In theory, ffmpeg blocks the thread, so it shouldn't be possible to toggle
  // the recording while ffmpeg is running. But, just in case, we check it
  if(IS_FFMPEG_RUNNING)
    return;

  if(IS_RECORDING){
    recordTimer->stop();
    if(createVideoFFmpeg()) {
      QApplication::beep();
    } else {
      printf("[ERROR] Couldn't start ffmpeg or timeout occurred (10 sec.): %s\n", QSTRING_TO_STRING(ffmpeg.errorString()));
      printf("[ERROR] Killing the ffmpeg process...\n");
      ffmpeg.terminate();
    }
    recordTempFile->remove();
    return;
  }

  // Start recording
  recordTempFile->remove(); // If already exists
  if (!recordTempFile->open(QIODevice::ReadWrite)) {
    printf("[ERROR] Couldn't open the temp file for the bytes output\n");
    return;
  }
  recordTimer->start(1000/RECORD_FPS);
  QApplication::beep();
}

void ZoomWidget::clearAllDrawings()
{
  if(_screenOpts == SCREENOPTS_HIDE_ALL)
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
  if(_screenOpts == SCREENOPTS_HIDE_ALL)
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
  if(_state == STATE_COLOR_PICKER){
    _state = STATE_MOVING;
    _activePen.setColor(_colorBeforePickColorMode);
  } else if(_state == STATE_DELETING){
    _state = STATE_MOVING;
  } else if(_flashlightMode) {
    _flashlightMode = false;
  } else if(_screenOpts == SCREENOPTS_HIDE_ALL) {
    CYCLE_SCREEN_OPTS();
  } else if(_desktopPixmapSize != _desktopPixmapOriginalSize) {
    _desktopPixmapScale = 1.0f;
    scalePixmapAt(QPoint(0,0));
    checkPixmapPos();
  } else if(IS_RECORDING){
    toggleRecording();
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
    updateAtMousePos(getCursorPos(false));
    update();
  }
}

void ZoomWidget::grabDesktop(bool liveMode)
{
  _liveMode = liveMode;

  QPixmap desktop = _desktopScreen->grabWindow(0);

  // Paint the desktop over _desktopPixmap
  // Fixes the issue with hdpi scaling (now the size of the image is the real
  // resolution of the screen)
  _desktopPixmap = QPixmap(desktop.size());
  QPainter painter(&_desktopPixmap);
  painter.drawPixmap(0, 0, desktop.width(), desktop.height(), desktop);
  painter.end();

  if(!liveMode)
    showFullScreen();
}

bool ZoomWidget::grabImage(QPixmap img, FitImage config)
{
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
  _desktopPixmap.fill(BLACKBOARD_COLOR);
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
  int newX = _desktopPixmapPos.x() - delta.x() * (_desktopPixmapSize.width()  / _screenSize.width() );
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

QPoint ZoomWidget::getCursorPos(bool hdpiScaling) {
  // The screen point of the cursor is relative to the resolution of the scaled
  // monitor
  QPoint point = mapFromGlobal(QCursor::pos());

  if(hdpiScaling){
    point.setX(FIX_X_FOR_HDPI_SCALING(point.x()));
    point.setY(FIX_Y_FOR_HDPI_SCALING(point.y()));
  }

  return point;
}

QPoint ZoomWidget::screenPointToPixmapPos(QPoint qpoint) {
  QPoint returnPoint = (qpoint - _desktopPixmapPos)/_desktopPixmapScale;

  returnPoint.setX( FIX_X_FOR_HDPI_SCALING(returnPoint.x()) );
  returnPoint.setY( FIX_Y_FOR_HDPI_SCALING(returnPoint.y()) );

  return returnPoint;
}

QPoint ZoomWidget::pixmapPointToScreenPos(QPoint qpoint) {
  qpoint.setX( GET_X_FROM_HDPI_SCALING(qpoint.x()) );
  qpoint.setY( GET_Y_FROM_HDPI_SCALING(qpoint.y()) );

  QPoint point = _desktopPixmapPos + qpoint * _desktopPixmapScale;

  return point;
}

QSize ZoomWidget::pixmapSizeToScreenSize(QSize qsize){
  qsize.setWidth(  GET_X_FROM_HDPI_SCALING(qsize.width() ) );
  qsize.setHeight( GET_Y_FROM_HDPI_SCALING(qsize.height()) );

  return qsize * _desktopPixmapScale;
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
