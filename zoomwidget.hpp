#ifndef ZOOMWIDGET_HPP
#define ZOOMWIDGET_HPP

#include <QOpenGLWidget>
#include <QString>
#include <QScreen>
#include <QCursor>
#include <QPen>
#include <QDir>
#include <QClipboard>

// CUSTOMIZATION
#define QCOLOR_RED       QColor(224,  49,  49)
#define QCOLOR_GREEN     QColor( 47, 158,  68)
#define QCOLOR_BLUE      QColor( 25, 113, 194)
#define QCOLOR_YELLOW    QColor(252, 196,  25)
#define QCOLOR_MAGENTA   QColor(156,  54, 181)
#define QCOLOR_CYAN      QColor( 27, 170, 191)
#define QCOLOR_ORANGE    QColor(232,  89,  12)
#define QCOLOR_BLACK     QColor(  7,   7,   7)
#define QCOLOR_WHITE     QColor(248, 249, 250)

#define BLACKBOARD_COLOR QColor( 33,  37,  41)

#define DATE_FORMAT_SAVED_IMAGE "dd-MM-yyyy hh.mm.ss"

#define BLOCK_ICON   "🔒"
#define NO_ZOOM_ICON "⛶"
#define ZOOM_ICON    "🔍"

// CODE

// The painter argument must be a pointer to the painter
#define changePenWidthFromPainter(painter, width) \
  QPen tempPen = painter->pen(); tempPen.setWidth(width); painter->setPen(tempPen);

#define drawDrawnPixmap(painter)                                   \
  painter.drawPixmap(_desktopPixmapPos.x(), _desktopPixmapPos.y(), \
      _desktopPixmapSize.width(), _desktopPixmapSize.height(),     \
      _drawnPixmap);

#define saveToImage() if(_drawnPixmap.save(savePath)) QApplication::beep();
#define saveToClipboard()                        \
  do {                                           \
    clipboard->setImage(_drawnPixmap.toImage()); \
    QApplication::beep();                        \
  } while(0)

#define switchFlashlightMode() _flashlightMode = !_flashlightMode;
#define switchBoardMode() _boardMode = !_boardMode;
#define switchOnlyShowDesktop() if(_state==STATE_MOVING) _onlyShowDesktop = !_onlyShowDesktop

#define getCursorPos() mapFromGlobal(QCursor::pos())

#define isInEditTextMode ((_state == STATE_MOVING) && (_drawMode == DRAWMODE_TEXT) && (_shiftPressed) && (!_onlyShowDesktop))
#define isTextEditable(cursorPos) ((isInEditTextMode) && (cursorOverForm(cursorPos) != -1))
#define isDisabledMouseTracking                                 \
  ( (_state != STATE_TYPING && _shiftPressed) ||                \
    (_state == STATE_TYPING && _freezeDesktopPosWhileWriting) )

// From a point in the screen (like the mouse cursor, because its position
// is relative to the screen, not the pixmap), it returns the position in the
// pixmap (which can be zoomed in and moved)
#define screenPointToPixmapPos(qpoint) ((qpoint - _desktopPixmapPos)/_desktopPixmapScale)

// From a point in the pixmap (like the position of the drawings), it
// returns the position relative to the screen
#define pixmapPointToScreenPos(qpoint) (_desktopPixmapPos + qpoint * _desktopPixmapScale)

// From the width and height of a form in the pixmap, it returns the correct
// width and height for the form in the screen applying the scale factor to
// them.
#define pixmapSizeToScreenSize(qsize)  (qsize * _desktopPixmapScale)

namespace Ui {
  class zoomwidget;
}

// User data structs.
struct UserObjectData {
  QPoint startPoint; // Point in the pixmap
  QPoint endPoint;   // Point in the pixmap
  QPen pen;
};

struct UserTextData {
  UserObjectData data;
  QFont font;
  int caretPos;
  QString text;
};

struct UserFreeFormData {
  QVector<QPoint> points;
  QPen pen;
  bool active;
};

enum FitImage {
  FIT_TO_WIDTH,
  FIT_TO_HEIGHT,
  FIT_AUTO,
};

enum ZoomWidgetState {
  STATE_MOVING,
  STATE_DRAWING,
  STATE_TYPING,
  STATE_DELETING,
};

enum ZoomWidgetDrawMode {
  DRAWMODE_ARROW,
  DRAWMODE_LINE,
  DRAWMODE_RECT,
  DRAWMODE_ELLIPSE,
  DRAWMODE_TEXT,
  DRAWMODE_FREEFORM,
  DRAWMODE_HIGHLIGHT,
};

class ZoomWidget : public QWidget
{
  Q_OBJECT

  public:
    explicit ZoomWidget(QWidget *parent = 0);
    ~ZoomWidget();

    void grabDesktop(bool liveMode);
    bool grabImage(QString path, FitImage config);

    // By passing an empty QString, sets the argument to the default
    QString initializeSaveFile(QString path, QString name, QString ext);

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

    QClipboard *clipboard;

    // Desktop pixmap properties.
    QScreen   *_desktopScreen;
    QPixmap		_desktopPixmap;
    QPoint		_desktopPixmapPos;
    QSize		_desktopPixmapSize;
    float		_desktopPixmapScale;
    // When Scaling is enable, this variable saves the correct size of the window
    // When there is no Scaling this variable is the same that _desktopPixmap.size()
    QSize		_desktopPixmapOriginalSize;
    QSize		_screenSize;

    // Pixmap shown in the screen. This can either be the _desktopPixmap, or the
    // blackboard if it's activated the _boardMode
    QPixmap		_drawnPixmap;

    QString    savePath;

    // User objects.
    QVector<UserObjectData>    _userRects;
    QVector<UserObjectData>    _userLines;
    QVector<UserObjectData>    _userArrows;
    QVector<UserObjectData>    _userEllipses;
    QVector<UserTextData>      _userTexts;
    QVector<UserFreeFormData>  _userFreeForms;
    QVector<UserObjectData>    _userHighlights;

    // ONLY FOR DEBUG PURPOSE OF THE HIT BOX
    // QVector<UserObjectData>    _userTests;
    /////////////////////////

    // Moving properties.
    float		_scaleSensivity;

    // Boolean for states
    bool _shiftPressed;
    bool _mousePressed;
    bool _boardMode;
    bool _liveMode;
    bool _flashlightMode;
    bool _onlyShowDesktop;
    // IN TEXT MODE: If the user was pressing shift when the mouse released
    // (finished sizing the text rectangle), disable mouse tracking while writing
    bool _freezeDesktopPosWhileWriting;

    int _flashlightRadius;

    ZoomWidgetState	_state;
    QPoint		_lastMousePos;

    // Drawing properties.
    ZoomWidgetDrawMode	_drawMode;
    QPoint	_startDrawPoint;
    QPoint	_endDrawPoint;
    QPen	_activePen;

    void drawSavedForms(QPainter *pixmapPainter);
    // Opaque the area outside the circle of the cursor
    void drawFlashlightEffect(QPainter *screenPainter);
    void drawActiveForm(QPainter *painter, bool drawToScreen);
    // The status is design to remember you things that you can forget they are
    // on or selected, for example, that you have selected some drawing mode,
    // the size of the pen, etc.
    void drawStatus(QPainter *screenPainter);

    void updateAtMousePos(QPoint mousePos);
    void shiftPixmap(const QPoint delta);
    void scalePixmapAt(const QPointF pos);
    void checkPixmapPos();

    // Returns the position in the vector of the form (from the current draw
    // mode) that is behind the cursor position. Returns -1 if there's no form
    // under the cursor
    int cursorOverForm(QPoint cursorPos);

    void removeFormBehindCursor(QPoint cursorPos);
    void updateCursorShape();
    bool isDrawingHovered(int drawMode, int i);

    // The X, Y, W and H arguments must be a point in the SCREEN, not in the pixmap
    // If floating is enabled, the form (the width and height) is not affected by zoom/scaling
    bool isCursorInsideHitBox(int x, int y, int w, int h, QPoint cursorPos, bool isFloating);

    // Functions for the mappings
    void escapeKeyFunction();
    void switchDeleteMode();
    void clearAllDrawings();
    void undoLastDrawing();

    // If posRelativeToScreen is true, it will return the positon be relative to
    // the screen, if it's false, it will return the position relative to the
    // pixmap
    void getRealUserObjectPos(const UserObjectData &userObj, int *x, int *y, int *w, int *h, bool posRelativeToScreen);
};

#endif // ZOOMWIDGET_HPP
