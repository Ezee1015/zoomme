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
#include <QSize>
#include <QPoint>

//////////////////////////////////////////// CUSTOMIZATION

/// Color scheme
                             //( R ,  G ,  B )
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

/// This is the scale factor for the font.
/// Font size = (1 to 9 -pen's width-) * FONT_SIZE_FACTOR
#define FONT_SIZE_FACTOR 3

/// Default folder where to export the different files
#define DEFAULT_FOLDER QStandardPaths::DesktopLocation // QStandardPaths::PicturesLocation
/// Default date format (used for the generic name when exporting files)
#define DATE_FORMAT_FOR_FILE "dd-MM-yyyy hh.mm.ss"

/// Icons
#define ENABLE_TOOLBAR_ICONS // Comment this line to disable toolbar icons

#define BLOCK_ICON            "🔒"
#define NO_ZOOM_ICON          "⛶"
#define ZOOM_ICON             "🔍"
#define RECORD_STATUS_ICON    "●"
#define EXIT_STATUS_ICON      "⊗" // ✖
#define DYNAMIC_ICON          " " //  󰐰  ⟺

#define LINE_ICON             "󰕞"
#define RECT_ICON             "󰗆"
#define ELLIPSE_ICON          "󰢓" // 󰺡
#define FREEFORM_ICON         "󱦹"
#define TEXT_ICON             "󰦨"
#define HIGHLIGHT_ICON        "󰙒"
#define ARROW_ICON            ""

#define WIDTH_ICON            "󱍓" //  󰳂 󰺾 󰑭 
#define COLOR_ICON            "" //  󰉦

#define FLASHLIGHT_ICON       "󰉄"
#define BLACKBOARD_ICON       "󰃥"
#define PICK_COLOR_ICON       ""
#define SCREEN_OPTS_ICON      ""
#define CLEAR_ICON            "󱘕"
#define DELETE_ICON           "󰷭" // 󰷮
#define UNDO_ICON             "󰕍" // 
#define REDO_ICON             "󰑏"
#define EXIT_ICON             "󰩈" // 
#define ESCAPE_ICON           "󰿅"
#define CANCEL_ICON           "󰜺"

#define EXPORT_IMG_ICON       ""
#define EXPORT_CLIP_ICON      ""
#define EXPORT_TRIM_IMG_ICON  "" // 
#define EXPORT_TRIM_CLIP_ICON "" // 
#define EXPORT_PROJECT_ICON   "" // 
#define RECORD_ICON           ""

/// Show a border around the tool bar buttons (I think its prettier without a
/// border). You can:
///  - Draw always: you have to uncomment BUTTON_BORDER_ALWAYS and comment
///      BUTTON_BORDER_ACTIVE
///  - Draw when active (default)
///  - Never draw it, by commenting both lines
// #define BUTTON_BORDER_ALWAYS // Always draw the button's border
#define BUTTON_BORDER_ACTIVE // Only draw the button's border if they're active

/// Recording settings
#define RECORD_FPS 16
#define RECORD_FRAME_QUALITY 70 // 0-100 | This is the JPEG compression of the frame
// This is the name for the file located in the temporal folder which is going
// to save the frames for the video
#define RECORD_TEMP_FILENAME "ZoomMe_video_bytes"

/// This is the name for the file located in the temporal folder, which is
/// going to save the screenshot taken in order to pass it to the Linux clipboard
/// manager (xclip or wl-copy)
#define CLIPBOARD_TEMP_FILENAME "zoomme_clipboard"

/// This is what separates the file name of the exported file and the index
/// number when a file with the same name and extension already exist
#define FILE_INDEX_DIVIDER " "

/// This assigns the button of the mouse that is going to be in charge of
/// dragging the desktop/background
#define DRAG_MOUSE_BUTTON Qt::RightButton

/// This is the time that the exit confirmation is shown. After this time the
/// confirmation gets canceled
#define EXIT_CONFIRMATION 1500 // msec

/// This is the transparency of the highlight for the forms
#define HIGHLIGHT_ALPHA 75 // 0-255

/// Roundness for the rectangles
#define POPUP_ROUNDNESS 12.0f // Status bar, pop-ups, tool bar and its buttons
#define RECT_ROUNDNESS  5.0f  // Drawn rectangles

/// This is the sensitivity of the mouse wheel for zooming in and out and
/// adjusting the radius of the flashlight effect
#define SCALE_SENSIVITY 0.2f // the higher the number, the more sensitive it is

/// This is the maximum length for the lines of the arrow head
#define MAX_ARROWHEAD_LENGTH 50 // pixels

/// This is the number of iterations of the smoothing function over the free form
#define FREEFORM_SMOOTHING 2 // 0 --> No smoothing

/// By uncommenting the DISABLE_MOUSE_TRACKING, you can disable the mouse
/// tracking function, that moves the desktop/background when zoomed in by
/// following the mouse. If disabled, you can move the background with the mouse
/// button specified in DRAG_MOUSE_BUTTON (just like you can with the mouse
/// tracking enabled and by pressing shift)
// #define DISABLE_MOUSE_TRACKING

/// Pop-ups settings
#define POPUP_WIDTH     300  // pixels
#define POPUP_ERROR     8000 // msec
#define POPUP_INFO      6000 // msec
#define POPUP_SUCCESS   3000 // msec
#define POPUP_SLIDE_IN  350  // msec
#define POPUP_SLIDE_OUT 350  // msec
// This is the amount of refreshes of the screen per second, in order to give
// the pop-ups its various animation effects
#define POPUP_FPS    50   // msec

//////////////////////////////////////////// END OF CUSTOMIZATION

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
#define FIX_X_FOR_HDPI_SCALING(point) ((point) * ((float)_sourcePixmap.width()  / (float)_canvas.originalSize.width() ))
#define FIX_Y_FOR_HDPI_SCALING(point) ((point) * ((float)_sourcePixmap.height() / (float)_canvas.originalSize.height()))
// These macros revert the conversion that does FIX_Y_FOR_HDPI_SCALING and
// FIX_X_FOR_HDPI_SCALING.
#define GET_X_FROM_HDPI_SCALING(point) ((point) * ((float)_canvas.originalSize.width()  / (float)_sourcePixmap.width() ))
#define GET_Y_FROM_HDPI_SCALING(point) ((point) * ((float)_canvas.originalSize.height() / (float)_sourcePixmap.height()))

#define IS_RECORDING (_recordTimer->isActive())
#define IS_FFMPEG_RUNNING (_ffmpeg.state() != QProcess::NotRunning)

namespace Ui {
  class zoomwidget;
}

// User data structs. Remember to update the save/restore state from file
// function when modifying this
struct UserObjectData {
  QPoint startPoint; // Point in the pixmap
  QPoint endPoint;   // Point in the pixmap
  QPen pen;
  bool highlight;
  bool arrow;
};

struct UserTextData {
  UserObjectData data;
  int caretPos;
  QString text;
};

struct UserFreeFormData {
  QList<QPoint> points;
  QPen pen;
  QList<int> penWidths; // The pen width of each point

  bool highlight;
  bool arrow;
  bool active;
};

struct ArrowHead {
  QPoint startPoint;
  QPoint leftLineEnd;
  QPoint rightLineEnd;
};

enum FreezeCanvas {
  FREEZE_BY_SHIFT,
  // IN TEXT MODE: If the user was pressing shift when the mouse released
  // (finished sizing the text rectangle), disable mouse tracking while writing
  FREEZE_BY_TEXT,
  FREEZE_FALSE,
};

struct UserCanvas {
  // Pixmap shown on the screen. This can either be _sourcePixmap, the
  // blackboard or a transparent background, with the drawings on top.
  QPixmap pixmap;
  // Zoom movement
  QPoint pos;
  FreezeCanvas freezePos;
  bool dragging;
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
  STATE_TO_TRIM,  // State before trimming
  STATE_TRIMMING,
};

enum ZoomWidgetDrawMode {
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
  ACTION_DYNAMIC_WIDTH,

  ACTION_SPACER, // Spacer for the tool bar

  ACTION_ESCAPE,
  ACTION_ESCAPE_CANCEL,
};

struct Button {
  ZoomWidgetAction action;
  QString icon;
  QString name;
  int row; // Starting from 0
  QRect rect;
};

struct ToolBar {
  QList<Button> buttons;
  bool show;

  // Configuration
  int rowHeight;
  int margin;
  QRect rect;
};

// When modifying this enum, don't forget to modify the toggleAction function
enum ZoomWidgetScreenOpts {
  SCREENOPTS_HIDE_ALL, // Only show the background. This disables some functions
                       // too (like the mouse actions and changing modes)
  SCREENOPTS_HIDE_FLOATING, // Hides the status and the popups
  SCREENOPTS_SHOW_ALL,
};

enum Log_Urgency {
  // With popup
  LOG_INFO,
  LOG_SUCCESS,
  LOG_ERROR,
  // No popup
  LOG_TEXT,            // To just print to stdout
  LOG_ERROR_AND_EXIT,
};

struct Popup {
  qint64 timeCreated;
  int lifetime;
  QString message;
  Log_Urgency urgency;
};

struct PopupTray {
  QList<Popup> popups;
  int margin;
  QPoint start;
  QTimer *updateTimer;
};

template<typename T>
class Drawing {
  private:
    QList<T> forms;
    QList<T> deleted;

  public:
    Drawing() {}

    void add(const T item) {
      forms.append(item);
    }

    T at(const qsizetype i) {
      return forms.at(i);
    }

    T last() {
      return forms.last();
    }

    qsizetype size() {
      return forms.size();
    }

    bool isEmpty() {
      return forms.isEmpty();
    }

    void clear() {
      deleted.append(forms);
      forms.clear();
    }

    void remove(const int itemPos) {
      if (itemPos >= forms.size()) return;
      deleted.append(forms.takeAt(itemPos));
    }

    // Removes the form. It DOESN'T put it in the deleted list.
    // Useful for editing the last form
    void destroyLast() {
      if (forms.isEmpty()) return;
      forms.removeLast();
    }

    // Move the form to the top of the list (e.g., for editing texts you edit
    // the last created -top of the list- text)
    void moveToTop(const int itemPos) {
      if (itemPos >= forms.size()) return;
      forms.append(forms.takeAt(itemPos));
    }

    void undo() {
      if (forms.isEmpty()) return;
      deleted.append(forms.takeLast());
    }

    void redo() {
      if (deleted.isEmpty()) return;
      forms.append(deleted.takeLast());
    }

    bool isDeletedEmpty() {
      return deleted.isEmpty();
    }
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
    void createBlackboard(QSize size);

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

    QClipboard *_clipboard;

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
    Drawing<UserObjectData>    _rects;
    Drawing<UserObjectData>    _lines;
    Drawing<UserObjectData>    _ellipses;
    Drawing<UserTextData>      _texts;
    Drawing<UserFreeFormData>  _freeForms;

    // ONLY FOR DEBUG PURPOSE OF THE HIT BOX
    // QList<UserObjectData>    _tests;
    /////////////////////////

    ToolBar _toolBar;
    PopupTray _popupTray;

    // Modes
    bool _boardMode;
    bool _liveMode;
    bool _flashlightMode;
    int _flashlightRadius;
    bool _highlight;
    bool _arrow;
    TrimOptions _trimDestination;
    bool _dynamicWidth; // Dynamic pen's width for the free form

    // Timer that cancels the escape after some time
    QTimer *_exitTimer;

    ZoomWidgetScreenOpts _screenOpts;

    // Color that was active before the 'pick a color' mode (so that the color
    // can be reverted after exiting that mode)
    QColor _colorBeforePickColorMode;

    // Recording
    QProcess _ffmpeg;
    QTimer *_recordTimer;
    QFile *_recordTempFile;

    ZoomWidgetState _state;
    QPoint _lastMousePos;

    // Drawing properties.
    ZoomWidgetDrawMode _drawMode;
    QPen _activePen;
    // These two points should be fixed to hdpi scaling
    QPoint _startDrawPoint;
    QPoint _endDrawPoint;

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
    ArrowHead getArrowHead(int x, int y, int width, int height, int lineLength);
    ArrowHead getFreeFormArrowHead(UserFreeFormData ff);
    void drawTrimmed(QPainter *pixmapPainter);
    void drawPopupTray(QPainter *screenPainter);
    void drawPopup(QPainter *screenPainter, const int listPos);

    // Pop-up
    void setPopupTrayPos();
    QRect getPopupRect(const int listPos);
    bool isPressingPopup(const QPoint cursorPos);
    void closePopupUnderCursor(const QPoint cursorPos);
    void updateForPopups(); // Timer function

    void saveImage(QPixmap pixmap, bool toImage);

    // Tool bar
    void loadButtons();
    void generateToolBar();
    void toggleAction(ZoomWidgetAction action);
    bool isToolBarVisible();
    bool isCursorOverButton(QPoint cursorPos);
    bool isCursorOverToolBar(QPoint cursorPos);
    // Returns the position in the vector of the buttons (_toolBar) that is behind
    // the cursor position. Returns -1 if there's no button under the cursor
    int buttonBehindCursor(QPoint cursor);
    // This function checks if the tool/action of the button is active. Don't
    // use it to check the state of a variable in the app (like checking if the
    // state is in delete mode. Use: _state == STATE_DELETING, NOT
    // isToolActive(ACTION_DELETE)), for example.
    ButtonStatus isButtonActive(Button button);
    bool isActionDisabled(ZoomWidgetAction action);
    void adjustFontSize(QFont *font, const QString text, const int rectWidth, const int minPointSize);

    void updateAtMousePos(QPoint mousePos);
    void dragPixmap(QPoint delta);
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
    bool isDisabledMouseTracking();
    bool isTextEditable(QPoint cursorPos);

    QList<int> getFreeFormWidth(UserFreeFormData form);
    UserFreeFormData smoothFreeForm(UserFreeFormData form);

    // The X, Y, W and H arguments must be a point in the SCREEN, not in the pixmap
    // If floating is enabled, the form (the width and height) is not affected by zoom/scaling
    bool isCursorInsideHitBox(int x, int y, int w, int h, QPoint cursorPos, bool isFloating);
    bool isCursorOverLine(int x, int y, int w, int h, QPoint cursorPos);
    bool isCursorOverArrowHead(ArrowHead head, QPoint cursorPos);

    // Recording
    void saveFrameToFile(); // Timer function
    void createVideoFFmpeg();

    // If posRelativeToScreen is true, it will return the positon be relative to
    // the screen, if it's false, it will return the position relative to the
    // pixmap
    void getRealUserObjectPos(const UserObjectData &userObj, int *x, int *y, int *w, int *h, bool posRelativeToScreen);

    void saveStateToFile(); // Create a .zoomme file

    // If the popupMsg is empty, the *fmt will be the popupMsg (if the type
    // accepts popups)
    void logUser(Log_Urgency type, QString popupMsg, const char *fmt, ...);
};

#endif // ZOOMWIDGET_HPP
