#ifndef ZOOMWIDGET_HPP
#define ZOOMWIDGET_HPP

#include <QOpenGLWidget>
#include <QStandardPaths>
#include <QString>
#include <QScreen>
#include <QPen>
#include <QClipboard>
#include <QProcess>
#include <QTimer>
#include <QFile>
#include <QFont>
#include <QDir>
#include <stdio.h>
#include <stdarg.h>

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

#define QCOLOR_BLACKBOARD        QColor( 33,  37,  41)
#define QCOLOR_TOOL_BAR          QCOLOR_BLUE
#define QCOLOR_TOOL_BAR_DISABLED QColor( 70, 70, 70)

// Font size = (1 to 9) * FONT_SIZE_FACTOR
#define FONT_SIZE_FACTOR 4

#define DEFAULT_FOLDER QStandardPaths::DesktopLocation // QStandardPaths::PicturesLocation
#define DATE_FORMAT_FOR_FILE "dd-MM-yyyy hh.mm.ss"

#define BLOCK_ICON      "🔒"
#define NO_ZOOM_ICON    "⛶"
#define ZOOM_ICON       "🔍"
#define RECORD_ICON     "●"
#define HIGHLIGHT_ICON  "🖍️"
#define EXIT_ICON       "⊗" // ✖

#define RECORD_FPS 16
#define RECORD_QUALITY 70 // 0-100
#define RECORD_TEMP_FILENAME "ZoomMe_video_bytes"

#define CLIPBOARD_TEMP_FILENAME "zoomme_clipboard"

#define EXIT_CONFIRM_MSECS 1500

#define HIGHLIGHT_ALPHA 75

#define FILE_INDEX_DIVIDER " "

#define POPUP_ROUNDNESS_FACTOR 12.0f
#define RECT_ROUNDNESS_FACTOR 5.0f

#define SCALE_SENSIVITY 0.2f // For the sensibility of the mouse wheel
// END OF CUSTOMIZATION

// CODE
#define QSTRING_TO_STRING(string) string.toStdString().c_str()

// It will give the mouse position relative to the resolution of scaled monitor
#define GET_CURSOR_POS() mapFromGlobal(QCursor::pos())

#define GET_COLOR_UNDER_CURSOR() _canvas.pixmap.toImage().pixel( screenPointToPixmapPos(GET_CURSOR_POS()) )

// If there's no HDPI scaling, it will return the same value, because the real
// and the scale resolution will be de same.
// For example, the screen point of the cursor is relative to the resolution of
// the scaled monitor, so I need to change it to the REAL position of the cursor
// (without hdpi scaling) with 'The Rule of Three'...
//
// scaledScreenResolution -------- mousePos
// realScreenResolution   --------    x
//
// So 'x' = mousePos * (_sourcePixmap.size()/_canvas.originalSize)
#define FIX_X_FOR_HDPI_SCALING(point) (point * (_sourcePixmap.width()  / _canvas.originalSize.width()) )
#define FIX_Y_FOR_HDPI_SCALING(point) (point * (_sourcePixmap.height() / _canvas.originalSize.height()))
// These macros revert the conversion that does FIX_Y_FOR_HDPI_SCALING and
// FIX_X_FOR_HDPI_SCALING.
#define GET_X_FROM_HDPI_SCALING(point) (point * (_canvas.originalSize.width())  / _sourcePixmap.width() )
#define GET_Y_FROM_HDPI_SCALING(point) (point * (_canvas.originalSize.height()) / _sourcePixmap.height())

#define IS_RECORDING (recordTimer->isActive())
#define IS_FFMPEG_RUNNING (ffmpeg.state() != QProcess::NotRunning)

#define IS_TRIMMING (_state == STATE_TRIMMING && _mousePressed)

namespace Ui {
  class zoomwidget;
}

// User data structs.
struct UserObjectData {
  QPoint startPoint; // Point in the pixmap
  QPoint endPoint;   // Point in the pixmap
  QPen pen;
  bool highlight;
};

struct UserTextData {
  UserObjectData data;
  int caretPos;
  QString text;
};

struct UserFreeFormData {
  QList<QPoint> points;
  QPen pen;
  bool highlight;
  bool active;
};

struct ArrowHead {
  QPoint startPoint;
  QPoint leftLineEnd;
  QPoint rightLineEnd;
};

struct UserCanvas {
  // Pixmap shown on the screen. This can either be _sourcePixmap, the
  // blackboard or a transparent background, with the drawings on top.
  QPixmap pixmap;
  // Zoom movement
  QPoint pos;
  // Pixmap size for zooming (referenced with the resolution of the scaled
  // size of the monitor when capturing desktop)
  QSize size;
  QSize originalSize;
  // Zooming scale
  float scale;
};

struct UserFileConfig {
  QDir folder;
  QString name;
  // Extensions
  QString videoExt;
  QString imageExt;
  QString zoommeExt;
};
enum FileType {
  FILE_VIDEO,
  FILE_IMAGE,
  FILE_ZOOMME
};

enum FitImage {
  FIT_TO_WIDTH,
  FIT_TO_HEIGHT,
  FIT_AUTO,
};

// Where to save the trimmed pixmap (from STATE_TRIMMING)
enum TrimOptions {
  TRIM_SAVE_TO_IMAGE,
  TRIM_SAVE_TO_CLIPBOARD,
};

enum ButtonStatus {
  BUTTON_ACTIVE,
  BUTTON_INACTIVE,
  BUTTON_NO_STATUS,
  BUTTON_DISABLED,
};

enum ZoomWidgetState {
  STATE_MOVING,
  STATE_DRAWING,
  STATE_TYPING,
  STATE_DELETING,
  STATE_COLOR_PICKER,
  STATE_TRIMMING,
};

enum ZoomWidgetDrawMode {
  DRAWMODE_ARROW,
  DRAWMODE_LINE,
  DRAWMODE_RECT,
  DRAWMODE_ELLIPSE,
  DRAWMODE_TEXT,
  DRAWMODE_FREEFORM,
};

enum ZoomWidgetAction {
  // DRAW MODES
  ACTION_LINE,
  ACTION_RECTANGLE,
  ACTION_ARROW,
  ACTION_ELLIPSE,
  ACTION_TEXT,
  ACTION_FREEFORM,
  ACTION_HIGHLIGHT,

  // FUNCTIONS
  ACTION_FLASHLIGHT,
  ACTION_BLACKBOARD,
  ACTION_PICK_COLOR,
  ACTION_UNDO,
  ACTION_REDO,
  ACTION_DELETE,
  ACTION_CLEAR,
  ACTION_SAVE_TO_FILE,
  ACTION_SAVE_TRIMMED_TO_IMAGE,
  ACTION_SAVE_TO_CLIPBOARD,
  ACTION_SAVE_TRIMMED_TO_CLIPBOARD,
  ACTION_SAVE_PROJECT,
  ACTION_SCREEN_OPTS,
  ACTION_RECORDING,

  // COLORS
  ACTION_COLOR_RED,
  ACTION_COLOR_GREEN,
  ACTION_COLOR_BLUE,
  ACTION_COLOR_YELLOW,
  ACTION_COLOR_ORANGE,
  ACTION_COLOR_MAGENTA,
  ACTION_COLOR_CYAN,
  ACTION_COLOR_BLACK,
  ACTION_COLOR_WHITE,

  // WIDTH OF PEN
  ACTION_WIDTH_1,
  ACTION_WIDTH_2,
  ACTION_WIDTH_3,
  ACTION_WIDTH_4,
  ACTION_WIDTH_5,
  ACTION_WIDTH_6,
  ACTION_WIDTH_7,
  ACTION_WIDTH_8,
  ACTION_WIDTH_9,

  ACTION_SPACER, // Spacer for the tool bar

  ACTION_ESCAPE,
  ACTION_ESCAPE_CANCEL,
};

struct Button {
  ZoomWidgetAction action;
  QString name;
  int line;
  QRect rect;
};

struct ToolBarProperties {
  int lineHeight;
  int margin;
  int numberOfLines;
  QRect rect;
};

// When modifying this enum, don't forget to modify the cycleScreenOpts function
enum ZoomWidgetScreenOpts {
  SCREENOPTS_HIDE_ALL, // Only show the background. This disables some functions
                       // too (like the mouse actions and changing modes)
  SCREENOPTS_HIDE_STATUS,
  SCREENOPTS_SHOW_ALL,
};

enum Log_Urgency {
  LOG_INFO,
  LOG_ERROR,
  LOG_ERROR_AND_EXIT,
  LOG_TEXT
};

class ZoomWidget : public QWidget
{
  Q_OBJECT

  public:
    explicit ZoomWidget(QWidget *parent = 0);
    ~ZoomWidget();

    void setLiveMode(bool liveMode);

    void restoreStateFromFile(QString path, FitImage config);

    // By passing an empty QString, sets the argument to the default
    void initFileConfig(QString path, QString name, QString imgExt, QString vidExt);

    void grabFromClipboard(FitImage config);
    void grabDesktop();
    void grabImage(QPixmap img, FitImage config);

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

    // Note: When scaling is enabled (such as HiDPI with Graphic Server scaling
    // in X11 or Wayland), if you grab the desktop, _sourcePixmap works with the
    // REAL size of the screen, while other variables in the canvas work with
    // the SCALED size.

    // This program operates with the scaled size of the screen. However, if you
    // grab the desktop with HiDPI scaling, the _sourcePixmap and
    // _canvas.pixmap, to maintain image quality, are saved with the original
    // resolution (REAL size of the screen) when painting _canvas.pixmap, it
    // overlays the REAL size image on the SCALED size monitor without losing
    // quality.
    UserCanvas _canvas;

    // The size of the pixmap should be the REAL size of the monitor when
    // capturing the desktop image (instead of taking the size of the scaled
    // monitor).
    QPixmap _sourcePixmap; // This can be the desktop or an image

    QScreen *_desktopScreen;
    // Resolution of the scaled monitor
    QSize _screenSize;

    // For exporting files
    UserFileConfig _fileConfig;

    // User objects.
    QVector<UserObjectData>    _userRects;
    QVector<UserObjectData>    _userLines;
    QVector<UserObjectData>    _userArrows;
    QVector<UserObjectData>    _userEllipses;
    QVector<UserTextData>      _userTexts;
    QVector<UserFreeFormData>  _userFreeForms;

    // Undo/Delete history
    QVector<UserObjectData>    _deletedRects;
    QVector<UserObjectData>    _deletedLines;
    QVector<UserObjectData>    _deletedArrows;
    QVector<UserObjectData>    _deletedEllipses;
    QVector<UserTextData>      _deletedTexts;
    QVector<UserFreeFormData>  _deletedFreeForms;

    QVector<Button> _toolBar;
    ToolBarProperties _toolBarOpts;

    // ONLY FOR DEBUG PURPOSE OF THE HIT BOX
    // QVector<UserObjectData>    _userTests;
    /////////////////////////

    // States
    bool _shiftPressed;
    bool _mousePressed;
    bool _showToolBar;
    bool _exitConfirm;

    // Modes
    bool _boardMode;
    bool _liveMode;
    bool _flashlightMode;
    int _flashlightRadius;
    bool _highlight;
    TrimOptions _trimDestination;

    // IN TEXT MODE: If the user was pressing shift when the mouse released
    // (finished sizing the text rectangle), disable mouse tracking while writing
    bool _freezeDesktopPosWhileWriting;

    ZoomWidgetScreenOpts _screenOpts;

    // Color that was active before the 'pick a color' mode (so that the color
    // can be reverted after exiting that mode)
    QColor _colorBeforePickColorMode;

    // Recording
    QProcess ffmpeg;
    QTimer *recordTimer;
    QFile *recordTempFile;

    ZoomWidgetState	_state;

    // Drawing properties.
    ZoomWidgetDrawMode	_drawMode;
    QPen	_activePen;
    // These two points should be fixed to hdpi scaling
    QPoint	_startDrawPoint;
    QPoint	_endDrawPoint;

    void drawDrawnPixmap(QPainter *painter);
    void drawSavedForms(QPainter *pixmapPainter);
    // Opaque the area outside the circle of the cursor
    void drawFlashlightEffect(QPainter *screenPainter, bool drawToScreen);
    void drawActiveForm(QPainter *painter, bool drawToScreen);
    // The status is design to remember you things that you can forget they are
    // on or selected, for example, that you have selected some drawing mode,
    // the size of the pen, etc.
    void drawStatus(QPainter *screenPainter);
    void drawToolBar(QPainter *screenPainter);
    void drawButton(QPainter *screenPainter, Button button);
    ArrowHead getArrowHead(int x, int y, int width, int height);
    void drawTrimmed(QPainter *pixmapPainter);

    void saveImage(QPixmap pixmap, bool toImage);

    // Tool bar
    void loadButtons();
    void generateToolBar();
    void toggleAction(ZoomWidgetAction action);
    bool isToolBarVisible();
    bool isCursorOverButton(QPoint cursorPos);
    // Returns the position in the vector of the buttons (_toolBar) that is behind
    // the cursor position. Returns -1 if there's no button under the cursor
    int buttonBehindCursor(QPoint cursor);
    // This function checks if the tool/action of the button is active. Don't
    // use it to check the state of a variable in the app (like checking if the
    // state is in delete mode. Use: _state == STATE_DELETING, NOT
    // isToolActive(ACTION_DELETE)), for example.
    ButtonStatus isButtonActive(Button button);
    bool isActionDisabled(ZoomWidgetAction action);

    void updateAtMousePos(QPoint mousePos);
    void shiftPixmap(const QPoint cursorPos);
    void scalePixmapAt(const QPointF pos);
    void checkPixmapPos();

    // From a point in the screen (like the mouse cursor, because its position
    // is relative to the screen, not the pixmap), it returns the position in the
    // pixmap (which can be zoomed in and moved).
    // The point should NOT be fixed to HDPI scaling.
    // Returns the pixmap point that IS fixed to HDPI scaling.
    QPoint screenPointToPixmapPos(QPoint qpoint);
    // From a point in the pixmap (like the position of the drawings), it
    // returns the position relative to the screen.
    // The point should be fixed to HDPI scaling.
    // Returns the screen point that is NOT fixed to HDPI scaling.
    QPoint pixmapPointToScreenPos(QPoint qpoint);
    // From the width and height of a form in the pixmap, it returns the correct
    // width and height for the form in the screen applying the scale factor to
    // them.
    // The size should be fixed to HDPI scaling
    // Returns the size that is NOT fixed to HDPI scaling
    QSize pixmapSizeToScreenSize(QSize qsize);

    // Returns the position in the vector of the form (from the current draw
    // mode) that is behind the cursor position. Returns -1 if there's no form
    // under the cursor
    int cursorOverForm(QPoint cursorPos);

    QString getFilePath(FileType type);

    void removeFormBehindCursor(QPoint cursorPos);
    void updateCursorShape();
    bool isDrawingHovered(ZoomWidgetDrawMode drawMode, int i);
    bool isInEditTextMode();
    bool isDisabledMouseTracking();
    bool isTextEditable(QPoint cursorPos);

    // The X, Y, W and H arguments must be a point in the SCREEN, not in the pixmap
    // If floating is enabled, the form (the width and height) is not affected by zoom/scaling
    bool isCursorInsideHitBox(int x, int y, int w, int h, QPoint cursorPos, bool isFloating);

    // Recording
    void saveFrameToFile(); // Timer function
    void createVideoFFmpeg();

    // If posRelativeToScreen is true, it will return the positon be relative to
    // the screen, if it's false, it will return the position relative to the
    // pixmap
    void getRealUserObjectPos(const UserObjectData &userObj, int *x, int *y, int *w, int *h, bool posRelativeToScreen);

    void saveStateToFile(); // Create a .zoomme file

    void logUser(Log_Urgency type, const char *fmt, ...);
};

#endif // ZOOMWIDGET_HPP
