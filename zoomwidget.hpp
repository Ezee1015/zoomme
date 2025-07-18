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
#define QCOLOR_TOOL_BAR_DISABLED QColor( 70,  70,  70)
#define QCOLOR_NODE              QCOLOR_BLUE
#define QCOLOR_BACKGROUND        QColor( 52,  58,  64)
#define QCOLOR_GRID              QCOLOR_BACKGROUND

/// This is the scale factor for the font.
#define FONT_SCALE 3
/// This is the scale factor for the line's width
#define LINE_WIDTH_SCALE 1

/// Default folder where to export the different files
#define DEFAULT_FOLDER QStandardPaths::DesktopLocation // QStandardPaths::PicturesLocation
/// Default date format (used for the generic name when exporting files)
#define DATE_FORMAT_FOR_FILE "yyyy-MM-dd hh.mm.ss"

/// Icons
#define ENABLE_TOOLBAR_ICONS // Comment this line to disable toolbar icons

#define BLOCK_ICON            ""
#define NO_ZOOM_ICON          "⛶"
#define ZOOM_ICON             "󰍉"
#define RECORD_STATUS_ICON    "●"
#define EXIT_STATUS_ICON      "⊗" // ✖
#define DYNAMIC_ICON          "" //  󰐰  ⟺

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
#define TRACKING_ICON         ""
#define BLACKBOARD_ICON       "󰃥"
#define GRID_ICON             "󰝘"
#define PICK_COLOR_ICON       ""
#define SCREEN_OPTS_ICON      ""
#define RESIZE_ICON           "󰩨" // 󰙕
#define MOVE_ICON             "󰆾"
#define CLEAR_ICON            "󱘕"
#define DELETE_ICON           "󰷭" // 󰷮
#define UNDO_ICON             "󰕍" // 
#define REDO_ICON             "󰑏"
#define FULLSCREEN_ICON       "󰊓"
#define EXIT_ICON             "󰩈" // 
#define ESCAPE_ICON           "󰿅"
#define CANCEL_ICON           "󰜺"

#define EXPORT_IMG_ICON       ""
#define EXPORT_CLIP_ICON      ""
#define EXPORT_TRIM_IMG_ICON  " 󰈔" // 
#define EXPORT_TRIM_CLIP_ICON " " // 
#define EXPORT_PROJECT_ICON   "" // 
#define RECORD_ICON           ""

/// Show a border around the tool bar buttons (I think its prettier without a
/// border). You can:
///  - Draw always: you have to uncomment BUTTON_BORDER_ALWAYS and comment
///                 BUTTON_BORDER_ACTIVE
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

// This is the radius of the node when resizing a form
#define NODE_RADIUS 5

/// Roundness for the rectangles
#define POPUP_ROUNDNESS 12.0f // Status bar, pop-ups, tool bar and its buttons
#define RECT_ROUNDNESS  5.0f  // Drawn rectangles

/// This is the sensitivity of the mouse wheel for zooming in and out and
/// adjusting the radius of the flashlight effect
#define SCALE_SENSIVITY 0.1f // the higher the number, the more sensitive it is

/// This is the maximum length for the lines of the arrow head
#define MAX_ARROWHEAD_LENGTH 50 // pixels

/// This is the distance between the grid lines
#define GRID_DISTANCE_X 50 // pixels (vertical grid)
#define GRID_DISTANCE_Y 50 // pixels (horizontal grid)
/// This is the width of the grid lines
#define GRID_WIDTH 2 // pixels

/// This is the number of iterations of the smoothing function over the free form
#define FREEFORM_SMOOTHING 2 // 0 --> No smoothing

/// Pop-ups settings
#define POPUP_WIDTH     (_windowSize.width() / 4.0) // pixels
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
// So 'x' = mousePos * (_canvas.source.size()/_canvas.originalSize)
#define FIX_X_FOR_HDPI_SCALING(point) ((point) * ((float)_canvas.source.width()  / (float)_canvas.originalSize.width() ))
#define FIX_Y_FOR_HDPI_SCALING(point) ((point) * ((float)_canvas.source.height() / (float)_canvas.originalSize.height()))
// These macros revert the conversion that does FIX_Y_FOR_HDPI_SCALING and
// FIX_X_FOR_HDPI_SCALING.
#define GET_X_FROM_HDPI_SCALING(point) ((point) * ((float)_canvas.originalSize.width()  / (float)_canvas.source.width() ))
#define GET_Y_FROM_HDPI_SCALING(point) ((point) * ((float)_canvas.originalSize.height() / (float)_canvas.source.height()))

#define IS_RECORDING (_recordTimer->isActive())
#define IS_FFMPEG_RUNNING (_ffmpeg.state() != QProcess::NotRunning)

namespace Ui {
  class zoomwidget;
}

enum FormType {
  LINE,
  RECTANGLE,
  ELLIPSE,
  TEXT,
  FREEFORM,
};

// User data structs. Remember to update the save/restore state from file
// function when modifying this
struct Form {
  FormType type;

  QList<QPoint> points; // Points in the pixmap
  QPen pen;

  bool highlight;
  bool arrow;
  bool deleted;
  bool active; // If it's currently being modified

  // Free Forms
  QList<int> penWidths; // The pen width of each point

  // For text boxes
  int caretPos;
  QString text;
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

struct Canvas {
  // Pixmap shown on the screen. This can either be _canvas.source, the
  // blackboard or a transparent background, with the drawings on top.
  QPixmap pixmap;
  // The size of the pixmap should be the REAL size of the monitor when
  // capturing the desktop image (instead of taking the size of the scaled
  // monitor).
  QPixmap source; // This can be the desktop or an image

  // Zoom movement
  QPointF pos;
  FreezeCanvas freezePos;
  bool dragging;
  // Pixmap size for zooming (referenced with the resolution of the scaled
  // size of the monitor when capturing desktop)
  QSize size;
  QSize originalSize;
  // Zooming scale
  float scale;
};

struct ExportConfig {
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

enum State {
  STATE_NORMAL,
  STATE_DRAWING,
  STATE_TYPING,
  STATE_DELETING,
  STATE_RESIZE_FORM,
  STATE_RESIZING_FORM,
  STATE_MOVE_FORM,
  STATE_MOVING_FORM,
  STATE_COLOR_PICKER,
  STATE_TO_TRIM,  // State before trimming
  STATE_TRIMMING,
};

enum Action {
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
  ACTION_GRID,
  ACTION_PICK_COLOR,
  ACTION_DELETE_LAST,
  ACTION_UNDO_DELETE,
  ACTION_MOUSE_TRACKING,
  ACTION_DELETE,
  ACTION_RESIZE,
  ACTION_MOVE,
  ACTION_CLEAR,
  ACTION_SAVE_TO_FILE,
  ACTION_SAVE_TRIMMED_TO_IMAGE,
  ACTION_SAVE_TO_CLIPBOARD,
  ACTION_SAVE_TRIMMED_TO_CLIPBOARD,
  ACTION_SAVE_PROJECT,
  ACTION_SCREEN_OPTS,
  ACTION_RECORDING,
  ACTION_FULLSCREEN,

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
  Action action;
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
enum ScreenOptions {
  SCREENOPTS_HIDE_ALL, // Only show the background. This disables some functions
                       // too (like the mouse actions and changing modes)
  SCREENOPTS_HIDE_FLOATING, // Hides the status and the popups
  SCREENOPTS_SHOW_ALL,
};

enum LogUrgency {
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
  LogUrgency urgency;
};

struct PopupTray {
  QList<Popup> popups;
  int margin;
  QPoint start;
  QTimer *updateTimer;
};

class ZoomWidget : public QWidget
{
  Q_OBJECT

  public:
    explicit ZoomWidget(QWidget *parent = 0);
    ~ZoomWidget();

    void setLiveMode();

    void restoreStateFromFile(const QString path);

    // By passing an empty QString, sets the argument to the default
    void initFileConfig(const QString path, const QString name, const QString imgExt, const QString vidExt);

    void grabFromClipboard();
    void grabDesktop();
    void grabImage(const QPixmap img);
    void createBlackboard(const QSize size);

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

    // System variables
    QScreen *_desktopScreen;
    QClipboard *_clipboard;


    QList<Form> _forms;
    QList<int> _deletedHistory; // Saves the position of the deleted Form in _forms
    ToolBar _toolBar;
    PopupTray _popupTray;
    // Note: When scaling is enabled (such as HiDPI with Graphic Server scaling
    // in X11 or Wayland), if you grab the desktop, _canvas.source works with the
    // REAL size of the screen, while other variables in the canvas work with
    // the SCALED size.
    //
    // This program operates with the scaled size of the screen. However, if you
    // grab the desktop with HiDPI scaling, the _canvas.source and
    // _canvas.pixmap, to maintain image quality, are saved with the original
    // resolution (REAL size of the screen) when painting _canvas.pixmap, it
    // overlays the REAL size image on the SCALED size monitor without losing
    // quality.
    Canvas _canvas;


    // STATE/CONFIG VARIABLES
    QSize _windowSize;
    ExportConfig _fileConfig; // For exporting files
    bool _forceMouseTracking;
    int _resizeNodePosition; // The position of the selected node (point of the form)
                             // The form selected is the last one of the vector
    TrimOptions _trimDestination;
    ScreenOptions _screenOpts;
    State _state;
    FormType _drawMode;
    QPen _activePen;
    QPoint _lastMousePos;
    // These two following points should be fixed to hdpi scaling
    QPoint _startDrawPoint;
    QPoint _endDrawPoint;
    // Color that was active before the 'pick a color' mode (so that the color
    // can be reverted after exiting that mode)
    QColor _colorBeforePickColorMode;

    // Modes
    bool _boardMode;
    bool _grid;
    bool _liveMode;
    bool _flashlightMode;
    int  _flashlightRadius;
    bool _highlight;
    bool _arrow;
    bool _dynamicWidth; // Dynamic pen's width for the free form


    // Timer that cancels the escape after some time
    QTimer *_exitTimer;

    // Recording
    QProcess _ffmpeg;
    QTimer *_recordTimer;
    QFile *_recordTempFile;

    // Drawing functions
    void drawDrawnPixmap(QPainter *painter);
    void drawSavedForms(QPainter *pixmapPainter);
    void drawActiveForm(QPainter *painter, const bool drawToScreen);
    // Opaque the area outside the circle of the cursor
    void drawFlashlightEffect(QPainter *screenPainter, const bool drawToScreen);
    // The status is design to remember you things that you can forget they are
    // on or selected, for example, that you have selected some drawing mode,
    // the size of the pen, etc.
    void drawStatus(QPainter *screenPainter);
    void drawToolBar(QPainter *screenPainter);
    void drawButton(QPainter *screenPainter, const Button button);
    ArrowHead getArrowHead(const int x, const int y, const int width, const int height, int lineLength);
    ArrowHead getFreeFormArrowHead(const Form freeForm);
    void drawTrimmed(QPainter *pixmapPainter);
    void drawPopupTray(QPainter *screenPainter);
    void drawPopup(QPainter *screenPainter, const int listPos);
    void drawNode(QPainter *painter, const QPoint point);
    void drawHandle(QPainter *painter, const QPoint point);

    // Pop-up
    void setPopupTrayPos();
    QRect getPopupRect(const int listPos);
    bool isPressingPopup(const QPoint cursorPos);
    void closePopupUnderCursor(const QPoint cursorPos);
    void updateForPopups(); // Timer function

    // Resizing nodes
    void resizeForm(QPoint cursorPos);
    void moveForm(QPoint cursorPos);
    void drawAllNodes(QPainter *screenPainter);
    void drawAllHandles(QPainter *screenPainter);
    bool isCursorOverNode(const QPoint cursorPos, const QPoint point);
    // Moves the form behind the cursor to the top of the list and populates the
    // _resize variable
    bool selectNode(const QPoint cursorPos);
    bool selectHandle(const QPoint cursorPos);

    // Tool bar and buttons
    void loadButtons();
    void generateToolBar();
    void toggleAction(const Action action);
    bool isToolBarVisible();
    bool isCursorOverButton(const QPoint cursorPos);
    bool isCursorOverToolBar(const QPoint cursorPos);
    // Returns the position in the vector of the buttons (_toolBar) that is behind
    // the cursor position. Returns -1 if there's no button under the cursor
    int buttonBehindCursor(const QPoint cursorPos);
    // This function checks if the tool/action of the button is active. Don't
    // use it to check the state of a variable in the app (like checking if the
    // state is in delete mode. Use: _state == STATE_DELETING, NOT
    // isToolActive(ACTION_DELETE)), for example.
    ButtonStatus isButtonActive(const Button button);
    bool isActionActive(const Action action);
    bool adjustFontSize(QFont *font, const QString text, const int rectWidth, const int minPointSize);

    // _canvas functions
    void updateAtMousePos(const QPoint mousePos);
    void dragPixmap(const QPoint delta);
    void shiftPixmap(const QPoint cursorPos);
    void scalePixmapAt(const QPointF pos);
    QPoint centerCanvas();

    // From a point in the screen (like the mouse cursor, because its position
    // is relative to the screen, not the pixmap), it returns the position in the
    // pixmap (which can be zoomed in and moved).
    // The point should NOT be fixed to HDPI scaling.
    // Returns the pixmap point that IS fixed to HDPI scaling.
    QPoint screenPointToPixmapPos(const QPoint qpoint);
    // From a point in the pixmap (like the position of the drawings), it
    // returns the position relative to the screen.
    // The point should be fixed to HDPI scaling.
    // Returns the screen point that is NOT fixed to HDPI scaling.
    QPoint pixmapPointToScreenPos(const QPoint qpoint);
    // From the width and height of a form in the pixmap, it returns the correct
    // width and height for the form in the screen applying the scale factor to
    // them.
    // The size should be fixed to HDPI scaling
    // Returns the size that is NOT fixed to HDPI scaling
    QSize pixmapSizeToScreenSize(const QSize qsize);

    // Form Functions
    QList<int> getFreeFormWidth(const Form form);
    Form smoothFreeForm(Form form);
    void removeFormBehindCursor(const QPoint cursorPos);
    bool isDrawingHovered(const int i);
    bool isTextEditable(const QPoint cursorPos);
    // Returns the position in the vector of the form (from the current draw
    // mode) that is behind the cursor position. Returns -1 if there's no form
    // under the cursor
    int cursorOverForm(const QPoint cursorPos);
    // The X, Y, W and H arguments must be a point in the SCREEN, not in the pixmap
    // If floating is enabled, the form (the width and height) is not affected by zoom/scaling
    bool isCursorInsideHitBox(int x, int y, int w, int h, const QPoint cursorPos, const bool isFloating);
    bool isCursorOverLine(int x, int y, int w, int h, const QPoint cursorPos);
    bool isCursorOverArrowHead(ArrowHead head, const QPoint cursorPos);
    // If posRelativeToScreen is true, it will return the positon be relative to
    // the screen, if it's false, it will return the position relative to the
    // pixmap
    void getSimpleFormPosition(const Form &userObj, int *x, int *y, int *w, int *h, const bool posRelativeToScreen);

    // Exporting
    QString getFilePath(const FileType type);
    // If toImage is false, the functions saves it to the clipboard
    void saveImage(const QPixmap pixmap, const bool toImage);
    void saveFrameToFile(); // Timer function for recording
    void createVideoFFmpeg();
    void saveStateToFile(); // Create a .zoomme file

    // Cursor
    void updateCursorShape();
    bool isDisabledMouseTracking();

    // If the popupMsg is empty, the *fmt will be the popupMsg (if the type
    // accepts popups)
    void logUser(const LogUrgency type, QString popupMsg, const char *console_log_fmt, ...);
};

#endif // ZOOMWIDGET_HPP
