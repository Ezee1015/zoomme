#ifndef ZOOMWIDGET_HPP
#define ZOOMWIDGET_HPP

#include <QOpenGLWidget>
#include <QString>
#include <QScreen>
#include <QPen>

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

#define changePenWidthFromPainter(painter, width) \
  QPen tempPen = painter.pen(); tempPen.setWidth(width); painter.setPen(tempPen);

#define switchFlashlightMode() _flashlightMode = !_flashlightMode;

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
  int caretPos;
  QString text;
};

struct UserFreeFormData {
  QVector<QPoint> points;
  QPen pen;
  bool active;
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
    bool grabImage(QString path);

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
    QScreen   *_desktopScreen;
    QPixmap		_desktopPixmap;
    QPoint		_desktopPixmapPos;
    QSize		_desktopPixmapSize;
    float		_desktopPixmapScale;
    // When Scaling is enable, this variable saves the correct size of the window
    // When there is no Scaling this variable is the same that _desktopPixmap.size()
    QSize		_desktopPixmapOriginalSize;

    // Pixmap shown in the screen. This can either be the _desktopPixmap, or the
    // blackboard if it's activated the _boardMode
    QPixmap		_drawnPixmap;

    // User objects.
    QVector<UserObjectData>    _userRects;
    QVector<UserObjectData>    _userLines;
    QVector<UserObjectData>    _userArrows;
    QVector<UserObjectData>    _userEllipses;
    QVector<UserTextData>      _userTexts;
    QVector<UserFreeFormData>  _userFreeForms;
    QVector<UserObjectData>    _userHighlights;

    // Moving properties.
    float		_scaleSensivity;

    // Boolean for states
    bool _shiftPressed;
    bool _mousePressed;
    bool _boardMode;
    bool _liveMode;
    bool _flashlightMode;

    int _flashlightRadius;

    ZoomWidgetState	_state;
    QPoint		_lastMousePos;

    // Drawing properties.
    ZoomWidgetDrawMode	_drawMode;
    QPoint	_startDrawPoint;
    QPoint	_endDrawPoint;
    QPen	_activePen;

    void saveScreenshot();

    void updateAtMousePos(QPoint mousePos);
    void shiftPixmap(const QPoint delta);
    void scalePixmapAt(const QPointF pos);

    void checkPixmapPos();

    // Returns the position in the vector of the form (from the current draw
    // mode) that is behind the cursor position. Returns -1 if there's no form
    // under the cursor
    int cursorOverForm(QPoint cursorPos);

    bool isCursorInsideHitBox(int x, int y, int w, int h, QPoint cursorPos);
    void removeFormBehindCursor(QPoint cursorPos);
    void updateCursorShape();
    bool isDrawingHovered(int drawMode, int i);
    bool isTextEditable(QPoint cursorPos);

    // Functions for the mappings
    void escapeKeyFunction();
    void switchDeleteMode();
    void switchBoardMode();
    void clearAllDrawings();
    void undoLastDrawing();

    void getRealUserObjectPos(const UserObjectData &userObj, int *x, int *y, int *w, int *h);
};

#endif // ZOOMWIDGET_HPP
