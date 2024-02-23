#include "zoomwidget.hpp"
#include "ui_zoomwidget.h"

#include <cmath>
#include <cstdio>
#include <QPainter>
#include <QMouseEvent>
#include <QRect>
#include <QGuiApplication>
#include <QOpenGLWidget>
#include <QCursor>
#include <QDateTime>
#include <QColor>
#include <QPainterPath>
#include <QList>
#include <QImageWriter>
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

ZoomWidget::ZoomWidget(QWidget *parent) : QWidget(parent), ui(new Ui::zoomwidget)
{
  ui->setupUi(this);
  setMouseTracking(true);

  _state = STATE_MOVING;
  _drawMode  = DRAWMODE_LINE;

  _desktopScreen = QGuiApplication::screenAt(QCursor::pos());
  _screenSize = _desktopScreen->geometry().size();
  _canvas.pos = QPoint(0, 0);
  _canvas.size = QApplication::screenAt(QCursor::pos())->geometry().size();
  _canvas.originalSize = _canvas.size;
  _canvas.scale = 1.0f;
  _canvas.freezePos = FREEZE_FALSE;
  _canvas.dragging = false;

  _lastMousePos = GET_CURSOR_POS();

  _exitConfirm = false;
  _boardMode = false;
  _highlight = false;
  _liveMode = false;
  _flashlightMode = false;
  _flashlightRadius = 80;
  _screenOpts = SCREENOPTS_SHOW_ALL;

  clipboard = QApplication::clipboard();
  if(!clipboard)
    logUser(LOG_ERROR, "", "Couldn't grab the clipboard");

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
  QDir tempFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
  recordTempFile = new QFile(tempFile.absoluteFilePath(RECORD_TEMP_FILENAME));

  _activePen.setColor(QCOLOR_RED);
  _activePen.setWidth(4);

  _popupTray.margin = 20;
  _popupTray.updateTimer = new QTimer(this);
  connect( _popupTray.updateTimer,
           &QTimer::timeout,
           this,
           &ZoomWidget::updateForPopups );
  setPopupTrayPos();

  _toolBar.show = false;
  loadButtons();
  generateToolBar();
}

ZoomWidget::~ZoomWidget()
{
  delete ui;
}

bool ZoomWidget::isToolBarVisible()
{
  return _toolBar.show                 &&
         _state != STATE_COLOR_PICKER  &&
         _state != STATE_TYPING        &&
         _state != STATE_DRAWING;
}

void ZoomWidget::toggleAction(ZoomWidgetAction action)
{
  if(isActionDisabled(action))
    return;

  // The guardian clauses for each action should be in isActionDisabled()
  switch(action) {
    case ACTION_LINE:          _drawMode = DRAWMODE_LINE;          break;
    case ACTION_RECTANGLE:     _drawMode = DRAWMODE_RECT;          break;
    case ACTION_ELLIPSE:       _drawMode = DRAWMODE_ELLIPSE;       break;
    case ACTION_ARROW:         _drawMode = DRAWMODE_ARROW;         break;
    case ACTION_TEXT:          _drawMode = DRAWMODE_TEXT;          break;
    case ACTION_FREEFORM:      _drawMode = DRAWMODE_FREEFORM;      break;

    case ACTION_HIGHLIGHT:     _highlight = !_highlight;           break;
    case ACTION_FLASHLIGHT:    _flashlightMode = !_flashlightMode; break;
    case ACTION_BLACKBOARD:    _boardMode = !_boardMode;           break;

    case ACTION_DELETE:
       if(_state == STATE_MOVING)
         _state = STATE_DELETING;
       else if(_state == STATE_DELETING)
         _state = STATE_MOVING;
       break;

    case ACTION_CLEAR:
       _rects.clear();
       _lines.clear();
       _arrows.clear();
       _ellipses.clear();
       _texts.clear();
       _freeForms.clear();
       _state = STATE_MOVING;
       break;

    case ACTION_UNDO:
       switch(_drawMode) {
         case DRAWMODE_LINE:     _lines.undo();     break;
         case DRAWMODE_RECT:     _rects.undo();     break;
         case DRAWMODE_ARROW:    _arrows.undo();    break;
         case DRAWMODE_ELLIPSE:  _ellipses.undo();  break;
         case DRAWMODE_TEXT:     _texts.undo();     break;
         case DRAWMODE_FREEFORM: _freeForms.undo(); break;
       }
       break;

    case ACTION_REDO:
       switch(_drawMode) {
         case DRAWMODE_LINE:     _lines.redo();     break;
         case DRAWMODE_RECT:     _rects.redo();     break;
         case DRAWMODE_ARROW:    _arrows.redo();    break;
         case DRAWMODE_ELLIPSE:  _ellipses.redo();  break;
         case DRAWMODE_TEXT:     _texts.redo();     break;
         case DRAWMODE_FREEFORM: _freeForms.redo(); break;
       }
       break;

    case ACTION_RECORDING: {
       if(IS_RECORDING){
         recordTimer->stop();
         createVideoFFmpeg();
         recordTempFile->remove();
         break;
       }

       // Start recording
       logUser(LOG_TEXT, "", "Temporary record file path: %s", QSTRING_TO_STRING(recordTempFile->fileName()));

       recordTempFile->remove(); // Just in case for if it already exists

       bool openTempFile = recordTempFile->open(QIODevice::ReadWrite);
       if (!openTempFile) {
         logUser(LOG_ERROR, "", "Couldn't open the temp file for the bytes output");
         recordTempFile->close();
         break;
       }

       recordTimer->start(1000/RECORD_FPS);
       QApplication::beep();
       break;
    }

    case ACTION_PICK_COLOR:
       if(_state == STATE_COLOR_PICKER) {
         _state = STATE_MOVING;
         _activePen.setColor(_colorBeforePickColorMode);
         break;
       }

       _colorBeforePickColorMode = _activePen.color();
       _state = STATE_COLOR_PICKER;
       _activePen.setColor(GET_COLOR_UNDER_CURSOR());
       break;

    case ACTION_SAVE_TO_FILE:
       saveImage(_canvas.pixmap, true);
       break;

    case ACTION_SAVE_TO_CLIPBOARD:
       saveImage(_canvas.pixmap, false);
       break;

    case ACTION_SAVE_TRIMMED_TO_IMAGE:
      // If the mode is active, disable it
      if(_state == STATE_TO_TRIM) {
        if(_trimDestination == TRIM_SAVE_TO_IMAGE) {
          _state = STATE_MOVING;
          break;
        }

        _trimDestination = TRIM_SAVE_TO_IMAGE;
      }

      _state = STATE_TO_TRIM;
      _trimDestination = TRIM_SAVE_TO_IMAGE;
      _startDrawPoint = QPoint(0,0);
      _endDrawPoint = QPoint(0,0);
      break;

    case ACTION_SAVE_TRIMMED_TO_CLIPBOARD:
      // If the mode is active, disable it
      if(_state == STATE_TO_TRIM) {
        if(_trimDestination == TRIM_SAVE_TO_CLIPBOARD) {
          _state = STATE_MOVING;
          break;
        }

        _trimDestination = TRIM_SAVE_TO_CLIPBOARD;
      }

      _state = STATE_TO_TRIM;
      _trimDestination = TRIM_SAVE_TO_CLIPBOARD;
      _startDrawPoint = QPoint(0,0);
      _endDrawPoint = QPoint(0,0);
      break;

    case ACTION_SAVE_PROJECT:
       saveStateToFile();
       break;

    case ACTION_SCREEN_OPTS:
       switch(_screenOpts) {
         case SCREENOPTS_HIDE_ALL:      _screenOpts = SCREENOPTS_SHOW_ALL;    break;
         case SCREENOPTS_HIDE_FLOATING: _screenOpts = SCREENOPTS_HIDE_ALL;    break;
         case SCREENOPTS_SHOW_ALL:      _screenOpts = SCREENOPTS_HIDE_FLOATING; break;
       }
       break;


    case ACTION_COLOR_RED:     _activePen.setColor(QCOLOR_RED);     break;
    case ACTION_COLOR_GREEN:   _activePen.setColor(QCOLOR_GREEN);   break;
    case ACTION_COLOR_BLUE:    _activePen.setColor(QCOLOR_BLUE);    break;
    case ACTION_COLOR_CYAN:    _activePen.setColor(QCOLOR_CYAN);    break;
    case ACTION_COLOR_ORANGE:  _activePen.setColor(QCOLOR_ORANGE);  break;
    case ACTION_COLOR_MAGENTA: _activePen.setColor(QCOLOR_MAGENTA); break;
    case ACTION_COLOR_YELLOW:  _activePen.setColor(QCOLOR_YELLOW);  break;
    case ACTION_COLOR_WHITE:   _activePen.setColor(QCOLOR_WHITE);   break;
    case ACTION_COLOR_BLACK:   _activePen.setColor(QCOLOR_BLACK);   break;

    case ACTION_WIDTH_1:       _activePen.setWidth(1);              break;
    case ACTION_WIDTH_2:       _activePen.setWidth(2);              break;
    case ACTION_WIDTH_3:       _activePen.setWidth(3);              break;
    case ACTION_WIDTH_4:       _activePen.setWidth(4);              break;
    case ACTION_WIDTH_5:       _activePen.setWidth(5);              break;
    case ACTION_WIDTH_6:       _activePen.setWidth(6);              break;
    case ACTION_WIDTH_7:       _activePen.setWidth(7);              break;
    case ACTION_WIDTH_8:       _activePen.setWidth(8);              break;
    case ACTION_WIDTH_9:       _activePen.setWidth(9);              break;

    case ACTION_ESCAPE:
      if(_exitConfirm) {
        QApplication::beep();
        QApplication::quit();
        break;
      }

      if (_state == STATE_COLOR_PICKER) {
        toggleAction(ACTION_PICK_COLOR);

      } else if (_state == STATE_DELETING) {
        toggleAction(ACTION_DELETE);

      } else if (_state == STATE_TRIMMING || _state == STATE_TO_TRIM) {
        _state = STATE_MOVING;

      } else if (_flashlightMode) {
        toggleAction(ACTION_FLASHLIGHT);

      } else if (_screenOpts == SCREENOPTS_HIDE_ALL) {
        toggleAction(ACTION_SCREEN_OPTS);

      } else if(_canvas.size != _canvas.originalSize) {
        _canvas.scale = 1.0f;
        scalePixmapAt(QPoint(0,0));
        checkPixmapPos();

      } else if(IS_RECORDING) {
        toggleAction(ACTION_RECORDING);

      } else {
        _exitConfirm = true;
        // Timer that cancels the escape after some time
        QTimer::singleShot(EXIT_CONFIRM_MSECS, this, [=]() { toggleAction(ACTION_ESCAPE_CANCEL); });

        loadButtons();
        generateToolBar();
      }
      break;
    case ACTION_ESCAPE_CANCEL:
      _exitConfirm = false;

      loadButtons();
      generateToolBar();
      break;

    case ACTION_SPACER:        /* don't do anything here */         break;
  }

  updateCursorShape();
  update();
}

void ZoomWidget::loadButtons()
{
  _toolBar.buttons.clear();

  QRect nullRect(0, 0, 0, 0);

  _toolBar.buttons.append(Button{ACTION_WIDTH_1,           "1",                   0, nullRect});
  _toolBar.buttons.append(Button{ACTION_WIDTH_2,           "2",                   0, nullRect});
  _toolBar.buttons.append(Button{ACTION_WIDTH_3,           "3",                   0, nullRect});
  _toolBar.buttons.append(Button{ACTION_WIDTH_4,           "4",                   0, nullRect});
  _toolBar.buttons.append(Button{ACTION_WIDTH_5,           "5",                   0, nullRect});
  _toolBar.buttons.append(Button{ACTION_WIDTH_6,           "6",                   0, nullRect});
  _toolBar.buttons.append(Button{ACTION_WIDTH_7,           "7",                   0, nullRect});
  _toolBar.buttons.append(Button{ACTION_WIDTH_8,           "8",                   0, nullRect});
  _toolBar.buttons.append(Button{ACTION_WIDTH_9,           "9",                   0, nullRect});

  _toolBar.buttons.append(Button{ACTION_SPACER,            "",                    0, nullRect});

  _toolBar.buttons.append(Button{ACTION_COLOR_RED,         "Red",                 0, nullRect});
  _toolBar.buttons.append(Button{ACTION_COLOR_GREEN,       "Green",               0, nullRect});
  _toolBar.buttons.append(Button{ACTION_COLOR_BLUE,        "Blue",                0, nullRect});
  _toolBar.buttons.append(Button{ACTION_COLOR_YELLOW,      "Yellow",              0, nullRect});
  _toolBar.buttons.append(Button{ACTION_COLOR_ORANGE,      "Orange",              0, nullRect});
  _toolBar.buttons.append(Button{ACTION_COLOR_MAGENTA,     "Magenta",             0, nullRect});
  _toolBar.buttons.append(Button{ACTION_COLOR_CYAN,        "Cyan",                0, nullRect});
  _toolBar.buttons.append(Button{ACTION_COLOR_WHITE,       "White",               0, nullRect});
  _toolBar.buttons.append(Button{ACTION_COLOR_BLACK,       "Black",               0, nullRect});

  _toolBar.buttons.append(Button{ACTION_LINE,              "Line",                1, nullRect});
  _toolBar.buttons.append(Button{ACTION_RECTANGLE,         "Rectangle",           1, nullRect});
  _toolBar.buttons.append(Button{ACTION_ARROW,             "Arrow",               1, nullRect});
  _toolBar.buttons.append(Button{ACTION_ELLIPSE,           "Ellipse",             1, nullRect});
  _toolBar.buttons.append(Button{ACTION_FREEFORM,          "Free form",           1, nullRect});
  _toolBar.buttons.append(Button{ACTION_TEXT,              "Text",                1, nullRect});
  _toolBar.buttons.append(Button{ACTION_SPACER,            "",                    1, nullRect});
  _toolBar.buttons.append(Button{ACTION_HIGHLIGHT,         "Highlight",           1, nullRect});


  _toolBar.buttons.append(Button{ACTION_FLASHLIGHT,        "Flashlight",          2, nullRect});
  _toolBar.buttons.append(Button{ACTION_BLACKBOARD,        "Blackboard",          2, nullRect});
  _toolBar.buttons.append(Button{ACTION_PICK_COLOR,        "Pick color",          2, nullRect});
  _toolBar.buttons.append(Button{ACTION_SCREEN_OPTS,       "Hide elements",       2, nullRect});
  _toolBar.buttons.append(Button{ACTION_CLEAR,             "Clear",               2, nullRect});
  _toolBar.buttons.append(Button{ACTION_DELETE,            "Delete",              2, nullRect});
  _toolBar.buttons.append(Button{ACTION_UNDO,              "Undo",                2, nullRect});
  _toolBar.buttons.append(Button{ACTION_REDO,              "Redo",                2, nullRect});

  _toolBar.buttons.append(Button{ACTION_SPACER,            "",                    2, nullRect});

  if(_exitConfirm) {
    _toolBar.buttons.append(Button{ACTION_ESCAPE        ,    "Confirm Exit",        2, nullRect});
    _toolBar.buttons.append(Button{ACTION_ESCAPE_CANCEL,     "Cancel Exit",         2, nullRect});
  } else
    _toolBar.buttons.append(Button{ACTION_ESCAPE,            "Escape",              2, nullRect});

  _toolBar.buttons.append(Button{ACTION_SAVE_TO_FILE,      "Export image",        3, nullRect});
  _toolBar.buttons.append(Button{ACTION_SAVE_TO_CLIPBOARD, "Export to clipboard", 3, nullRect});
  _toolBar.buttons.append(Button{ACTION_SAVE_TRIMMED_TO_IMAGE,     "Export trimmed image",        3, nullRect});
  _toolBar.buttons.append(Button{ACTION_SAVE_TRIMMED_TO_CLIPBOARD, "Export trimmed to clipboard", 3, nullRect});
  _toolBar.buttons.append(Button{ACTION_SAVE_PROJECT,      "Save project",        3, nullRect});
  _toolBar.buttons.append(Button{ACTION_RECORDING,         "Record",              3, nullRect});
}

bool ZoomWidget::isActionDisabled(ZoomWidgetAction action)
{
  switch(action) {
    case ACTION_WIDTH_1:
    case ACTION_WIDTH_2:
    case ACTION_WIDTH_3:
    case ACTION_WIDTH_4:
    case ACTION_WIDTH_5:
    case ACTION_WIDTH_6:
    case ACTION_WIDTH_7:
    case ACTION_WIDTH_8:
    case ACTION_WIDTH_9:

    case ACTION_COLOR_RED:
    case ACTION_COLOR_GREEN:
    case ACTION_COLOR_BLUE:
    case ACTION_COLOR_YELLOW:
    case ACTION_COLOR_ORANGE:
    case ACTION_COLOR_MAGENTA:
    case ACTION_COLOR_CYAN:
    case ACTION_COLOR_WHITE:
    case ACTION_COLOR_BLACK:

    case ACTION_LINE:
    case ACTION_RECTANGLE:
    case ACTION_ARROW:
    case ACTION_ELLIPSE:
    case ACTION_FREEFORM:
    case ACTION_TEXT:
    case ACTION_HIGHLIGHT:
      return false;

    case ACTION_FLASHLIGHT:
    case ACTION_BLACKBOARD:
      return false;


    case ACTION_SCREEN_OPTS:
       if(_state != STATE_MOVING)
         return true;
       return false;

    case ACTION_PICK_COLOR:
       if( _state != STATE_MOVING && _state != STATE_COLOR_PICKER)
         return true;
       if(_screenOpts == SCREENOPTS_HIDE_ALL)
         return true;
       return false;

    case ACTION_REDO:
       if(_screenOpts == SCREENOPTS_HIDE_ALL)
         return true;
       switch(_drawMode) {
         case DRAWMODE_LINE:      return _lines.isDeletedEmpty();
         case DRAWMODE_RECT:      return _rects.isDeletedEmpty();
         case DRAWMODE_ARROW:     return _arrows.isDeletedEmpty();
         case DRAWMODE_ELLIPSE:   return _ellipses.isDeletedEmpty();
         case DRAWMODE_TEXT:      return _texts.isDeletedEmpty();
         case DRAWMODE_FREEFORM:  return _freeForms.isDeletedEmpty();
       }
       return false;

    case ACTION_UNDO:
       if(_screenOpts == SCREENOPTS_HIDE_ALL)
         return true;
       switch(_drawMode) {
         case DRAWMODE_LINE:     return _lines.isEmpty();
         case DRAWMODE_RECT:     return _rects.isEmpty();
         case DRAWMODE_ARROW:    return _arrows.isEmpty();
         case DRAWMODE_ELLIPSE:  return _ellipses.isEmpty();
         case DRAWMODE_TEXT:     return _texts.isEmpty();
         case DRAWMODE_FREEFORM: return _freeForms.isEmpty();
       }
       return false;

    case ACTION_CLEAR:
       if(_screenOpts == SCREENOPTS_HIDE_ALL)
         return true;
       if( _lines.isEmpty()     &&
           _rects.isEmpty()     &&
           _arrows.isEmpty()    &&
           _ellipses.isEmpty()  &&
           _texts.isEmpty()     &&
           _freeForms.isEmpty()
         ) {
         return true;
       }
       return false;

    case ACTION_DELETE:
       if(_screenOpts == SCREENOPTS_HIDE_ALL)
         return true;
       switch(_drawMode) {
         case DRAWMODE_LINE:     return _lines.isEmpty();
         case DRAWMODE_RECT:     return _rects.isEmpty();
         case DRAWMODE_ARROW:    return _arrows.isEmpty();
         case DRAWMODE_ELLIPSE:  return _ellipses.isEmpty();
         case DRAWMODE_TEXT:     return _texts.isEmpty();
         case DRAWMODE_FREEFORM: return _freeForms.isEmpty();
       }
       return false;

    case ACTION_SAVE_TO_FILE:
    case ACTION_SAVE_TO_CLIPBOARD:
    case ACTION_SAVE_PROJECT:
       return false;

    case ACTION_RECORDING:
       // In theory, ffmpeg blocks the thread, so it shouldn't be possible to toggle
       // the recording while ffmpeg is running. But, just in case, we check it
       if(IS_FFMPEG_RUNNING)
         return true;
       return false;

    case ACTION_SAVE_TRIMMED_TO_IMAGE:
    case ACTION_SAVE_TRIMMED_TO_CLIPBOARD:
      if( _state != STATE_MOVING   &&
          _state != STATE_TRIMMING &&
          _state != STATE_TO_TRIM
        )
         return true;
      if(_screenOpts == SCREENOPTS_HIDE_ALL)
        return true;
      return false;

    case ACTION_ESCAPE:
    case ACTION_ESCAPE_CANCEL:
      return false;

    case ACTION_SPACER:
      logUser(LOG_ERROR, "", "You shouldn't check if a 'spacer' is disabled");
      return false;
  }
  return false;
}

ButtonStatus ZoomWidget::isButtonActive(Button button)
{
  if(isActionDisabled(button.action))
    return BUTTON_DISABLED;

  bool actionStatus = false;
  switch(button.action) {
    case ACTION_WIDTH_1:           actionStatus = (_activePen.width() == 1);              break;
    case ACTION_WIDTH_2:           actionStatus = (_activePen.width() == 2);              break;
    case ACTION_WIDTH_3:           actionStatus = (_activePen.width() == 3);              break;
    case ACTION_WIDTH_4:           actionStatus = (_activePen.width() == 4);              break;
    case ACTION_WIDTH_5:           actionStatus = (_activePen.width() == 5);              break;
    case ACTION_WIDTH_6:           actionStatus = (_activePen.width() == 6);              break;
    case ACTION_WIDTH_7:           actionStatus = (_activePen.width() == 7);              break;
    case ACTION_WIDTH_8:           actionStatus = (_activePen.width() == 8);              break;
    case ACTION_WIDTH_9:           actionStatus = (_activePen.width() == 9);              break;

    case ACTION_COLOR_RED:         actionStatus = (_activePen.color() == QCOLOR_RED);     break;
    case ACTION_COLOR_GREEN:       actionStatus = (_activePen.color() == QCOLOR_GREEN);   break;
    case ACTION_COLOR_BLUE:        actionStatus = (_activePen.color() == QCOLOR_BLUE);    break;
    case ACTION_COLOR_YELLOW:      actionStatus = (_activePen.color() == QCOLOR_YELLOW);  break;
    case ACTION_COLOR_ORANGE:      actionStatus = (_activePen.color() == QCOLOR_ORANGE);  break;
    case ACTION_COLOR_MAGENTA:     actionStatus = (_activePen.color() == QCOLOR_MAGENTA); break;
    case ACTION_COLOR_CYAN:        actionStatus = (_activePen.color() == QCOLOR_CYAN);    break;
    case ACTION_COLOR_WHITE:       actionStatus = (_activePen.color() == QCOLOR_WHITE);   break;
    case ACTION_COLOR_BLACK:       actionStatus = (_activePen.color() == QCOLOR_BLACK);   break;

    case ACTION_LINE:              actionStatus = (_drawMode == DRAWMODE_LINE);           break;
    case ACTION_RECTANGLE:         actionStatus = (_drawMode == DRAWMODE_RECT);           break;
    case ACTION_ARROW:             actionStatus = (_drawMode == DRAWMODE_ARROW);          break;
    case ACTION_ELLIPSE:           actionStatus = (_drawMode == DRAWMODE_ELLIPSE);        break;
    case ACTION_FREEFORM:          actionStatus = (_drawMode == DRAWMODE_FREEFORM);       break;
    case ACTION_TEXT:              actionStatus = (_drawMode == DRAWMODE_TEXT);           break;
    case ACTION_HIGHLIGHT:         actionStatus = (_highlight);                           break;

    case ACTION_FLASHLIGHT:        actionStatus = (_flashlightMode);                      break;
    case ACTION_BLACKBOARD:        actionStatus = (_boardMode);                           break;
    case ACTION_PICK_COLOR:        actionStatus = (_state == STATE_COLOR_PICKER);         break;
    case ACTION_DELETE:            actionStatus = (_state == STATE_DELETING);             break;
    case ACTION_SCREEN_OPTS:       actionStatus = (_screenOpts != SCREENOPTS_SHOW_ALL);   break;
    case ACTION_UNDO:              return BUTTON_NO_STATUS;
    case ACTION_REDO:              return BUTTON_NO_STATUS;
    case ACTION_CLEAR:             return BUTTON_NO_STATUS;

    case ACTION_SAVE_TO_FILE:      return BUTTON_NO_STATUS;
    case ACTION_SAVE_TO_CLIPBOARD: return BUTTON_NO_STATUS;
    case ACTION_SAVE_PROJECT:      return BUTTON_NO_STATUS;
    case ACTION_RECORDING:         actionStatus = IS_RECORDING;                           break;
    case ACTION_SAVE_TRIMMED_TO_IMAGE:
                                   actionStatus = (_state == STATE_TRIMMING || _state == STATE_TO_TRIM) &&
                                                  (_trimDestination == TRIM_SAVE_TO_IMAGE);
                                   break;

    case ACTION_SAVE_TRIMMED_TO_CLIPBOARD:
                                   actionStatus = (_state == STATE_TRIMMING || _state == STATE_TO_TRIM) &&
                                                  (_trimDestination == TRIM_SAVE_TO_CLIPBOARD);
                                   break;

    case ACTION_ESCAPE:            actionStatus = _exitConfirm;                           break;
    case ACTION_ESCAPE_CANCEL:     return BUTTON_NO_STATUS;

    case ACTION_SPACER:            logUser(LOG_ERROR, "", "You shouldn't check if a 'spacer' is active");
                                   return BUTTON_NO_STATUS;

  }

  return (actionStatus) ? BUTTON_ACTIVE : BUTTON_INACTIVE;
}

void ZoomWidget::generateToolBar()
{
  const int margin = 20;
  const int lineHeight = 60;

  // Get the number of lines
  int numberOfLines = 0;
  for(int i=0; i<_toolBar.buttons.size(); i++) {
    if(_toolBar.buttons.at(i).line > numberOfLines)
      numberOfLines = _toolBar.buttons.at(i).line;
  }

  const QRect background = QRect (
                                   margin,
                                   _screenSize.height() - margin - lineHeight*(numberOfLines+1),
                                   _screenSize.width() - margin*2,
                                   lineHeight * (numberOfLines+1) - margin
                                 );

  _toolBar.lineHeight = lineHeight;
  _toolBar.margin = margin;
  _toolBar.numberOfLines = numberOfLines;
  _toolBar.rect = background;

  // Get the buttons per line
  int buttonsPerLine[numberOfLines+1];
  for(int i=0; i<=numberOfLines; i++) buttonsPerLine[i] = 0;
  for(int i=0; i<_toolBar.buttons.size(); i++)
    buttonsPerLine[ _toolBar.buttons.at(i).line ]++;

  // Clear the tool list
  QList<Button> buttons;
  buttons.append(_toolBar.buttons);
  _toolBar.buttons.clear();

  // Size the buttons
  const int buttonPadding = 2;
  float buttonCount[numberOfLines+1];
  for(int i=0; i<=numberOfLines; i++) buttonCount[i]=0; // Initialize to 0

  for(int i=0; i<buttons.size(); i++) {
    const int line = buttons.at(i).line;

    float width = (float)background.width() / (float)buttonsPerLine[line];
    int height  = lineHeight - (float)margin / (float)(numberOfLines+1);
    int x       = background.x() + buttonCount[line] * width;
    int y       = background.y() + line * height;

    // Padding
    x+=buttonPadding;
    y+=buttonPadding;
    width-=buttonPadding*2;
    height-=buttonPadding*2;

    // Add the button to the count
    buttonCount[line]++;

    QRect rect(x, y, width, height);

    _toolBar.buttons.append(Button{
        buttons.at(i).action,
        buttons.at(i).name,
        line,
        rect
    });
  }
}

// Returns -1 if there's no button behind the cursor
int ZoomWidget::buttonBehindCursor(QPoint cursor)
{
  if(!_toolBar.show)
    logUser(LOG_ERROR, "Source code error", "cursorOverButton() was called, but the tool box is not visible");

  for(int i=0; i<_toolBar.buttons.size(); i++) {
    if(_toolBar.buttons.at(i).rect.contains(cursor))
      return i;
  }

  return -1;
}

// It doesn't make any distinction between disabled and not disabled buttons
bool ZoomWidget::isCursorOverButton(QPoint cursorPos)
{
  if(!isToolBarVisible())
    return false;

  const int button = buttonBehindCursor(cursorPos);

  const bool isOverAButton = (button != -1);
  const bool isNotASpacer  = (_toolBar.buttons.at(button).action != ACTION_SPACER);

  return isOverAButton && isNotASpacer;
}

void ZoomWidget::saveStateToFile()
{
  QString filePath = getFilePath(FILE_ZOOMME);
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)){
    logUser(LOG_ERROR, "", "Couldn't create the file: %s", QSTRING_TO_STRING(filePath));
    return;
  }

  QDataStream out(&file);
  // There should be the same arguments that the restoreStateToFile()
  out << _screenSize
      << _sourcePixmap
      // I don't want to be zoomed in when restoring
      // << _canvas.pos
      // << _canvas.size
      // << _canvas.scale
      << _canvas.originalSize
      // The save path is absolute, so it is bound to the PC, so it shouldn't
      // be saved
      // << _fileConfig.folder
      << _fileConfig.name
      << _fileConfig.imageExt
      << _fileConfig.videoExt
      << _fileConfig.zoommeExt
      << _liveMode
      // I don't think is good to save if the status bar and/or the
      // drawings are hidden
      // << _screenOpts
      << _drawMode
      << _activePen
      << _highlight
      << _rects.size()
      << _lines.size()
      << _arrows.size()
      << _ellipses.size()
      << _texts.size()
      << _freeForms.size();

  // Save the drawings
  // Rectangles
  for(int i=0; i<_rects.size(); i++)
      out << _rects.at(i).startPoint
          << _rects.at(i).endPoint
          << _rects.at(i).pen
          << _rects.at(i).highlight;
  // Lines
  for(int i=0; i<_lines.size(); i++)
      out << _lines.at(i).startPoint
          << _lines.at(i).endPoint
          << _lines.at(i).pen
          << _lines.at(i).highlight;
  // Arrows
  for(int i=0; i<_arrows.size(); i++)
      out << _arrows.at(i).startPoint
          << _arrows.at(i).endPoint
          << _arrows.at(i).pen
          << _arrows.at(i).highlight;
  // Ellipses
  for(int i=0; i<_ellipses.size(); i++)
      out << _ellipses.at(i).startPoint
          << _ellipses.at(i).endPoint
          << _ellipses.at(i).pen
          << _ellipses.at(i).highlight;
  // Texts
  for(int i=0; i<_texts.size(); i++){
    out << _texts.at(i).data.startPoint
        << _texts.at(i).data.endPoint
        << _texts.at(i).data.pen
        << _texts.at(i).data.highlight;

    out << _texts.at(i).caretPos
        << _texts.at(i).text;
  }
  // Free Forms
  for(int i=0; i<_freeForms.size(); i++){
    out << _freeForms.at(i).points;

    out << _freeForms.at(i).pen
        << _freeForms.at(i).active
        << _freeForms.at(i).highlight;
  }

  QApplication::beep();
  logUser(LOG_SUCCESS, "Project file saved correctly!", "Project saved correctly: %s", QSTRING_TO_STRING(filePath));
}

void ZoomWidget::restoreStateFromFile(QString path, FitImage config)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    logUser(LOG_ERROR_AND_EXIT, "", "Couldn't restore the state from the file");

  long long userRectsCount      = 0,
            userLinesCount      = 0,
            userArrowsCount     = 0,
            userEllipsesCount   = 0,
            userTextsCount      = 0,
            userFreeFormsCount  = 0;

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
      // << _fileConfig.folder
      >> _fileConfig.name
      >> _fileConfig.imageExt
      >> _fileConfig.videoExt
      >> _fileConfig.zoommeExt
      >> _liveMode
      // I don't think is good to save if the status bar and/or the
      // drawings are hidden
      // >> _screenOpts
      >> _drawMode
      >> _activePen
      >> _highlight
      >> userRectsCount
      >> userLinesCount
      >> userArrowsCount
      >> userEllipsesCount
      >> userTextsCount
      >> userFreeFormsCount;

  // VARIABLES FOR SCALING THE DRAWINGS
  float scaleFactorX = 1.0, scaleFactorY = 1.0;
  int marginTop = 0, marginLeft = 0;

  if(savedScreenSize == _screenSize) {
    _canvas.originalSize = savedPixmapSize;
    _canvas.size = savedPixmapSize;
    _sourcePixmap = savedPixmap;
  } else {
    if(config == FIT_AUTO)
      config = (savedPixmap.width() > savedPixmap.height()) ? FIT_TO_HEIGHT : FIT_TO_WIDTH;

    QImage scaledPixmap = savedPixmap.toImage();
    if(config == FIT_TO_WIDTH)
      scaledPixmap = scaledPixmap.scaledToWidth(_screenSize.width());
    else
      scaledPixmap = scaledPixmap.scaledToHeight(_screenSize.height());

    logUser(LOG_INFO, "The ZoomMe recovery file was scaled! This may cause loss of quality...", "Scaling ZoomMe recover file...");
    logUser(LOG_TEXT, "", "  - Recovered screen size: %dx%d", savedScreenSize.width(), savedScreenSize.height());
    logUser(LOG_TEXT, "", "  - Actual screen size: %dx%d", _screenSize.width(), _screenSize.height());
    logUser(LOG_TEXT, "", "  - Recovered image size: %dx%d", savedPixmapSize.width(), savedPixmapSize.height());
    logUser(LOG_TEXT, "", "  - Scaled (actual) image size: %dx%d", scaledPixmap.width(), scaledPixmap.height());

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

  if(!_liveMode) showFullScreen();

  // Read the drawings
  // Rectangles
  for(int i=0; i<userRectsCount; i++){
    UserObjectData objectData;
    in >> objectData.startPoint >> objectData.endPoint >> objectData.pen >> objectData.highlight;

    objectData.startPoint.setX( marginLeft + objectData.startPoint.x() * scaleFactorX );
    objectData.startPoint.setY( marginTop  + objectData.startPoint.y() * scaleFactorY );
    objectData.endPoint.setX( marginLeft + objectData.endPoint.x() * scaleFactorX );
    objectData.endPoint.setY( marginTop  + objectData.endPoint.y() * scaleFactorY );

    _rects.add(objectData);
  }
  // Lines
  for(int i=0; i<userLinesCount; i++){
    UserObjectData objectData;
    in >> objectData.startPoint >> objectData.endPoint >> objectData.pen >> objectData.highlight;

    objectData.startPoint.setX( marginLeft + objectData.startPoint.x() * scaleFactorX );
    objectData.startPoint.setY( marginTop  + objectData.startPoint.y() * scaleFactorY );
    objectData.endPoint.setX( marginLeft + objectData.endPoint.x() * scaleFactorX );
    objectData.endPoint.setY( marginTop  + objectData.endPoint.y() * scaleFactorY );

    _lines.add(objectData);
  }
  // Arrows
  for(int i=0; i<userArrowsCount; i++){
    UserObjectData objectData;
    in >> objectData.startPoint >> objectData.endPoint >> objectData.pen >> objectData.highlight;

    objectData.startPoint.setX( marginLeft + objectData.startPoint.x() * scaleFactorX );
    objectData.startPoint.setY( marginTop  + objectData.startPoint.y() * scaleFactorY );
    objectData.endPoint.setX( marginLeft + objectData.endPoint.x() * scaleFactorX );
    objectData.endPoint.setY( marginTop  + objectData.endPoint.y() * scaleFactorY );

    _arrows.add(objectData);
  }
  // Ellipses
  for(int i=0; i<userEllipsesCount; i++){
    UserObjectData objectData;
    in >> objectData.startPoint >> objectData.endPoint >> objectData.pen >> objectData.highlight;

    objectData.startPoint.setX( marginLeft + objectData.startPoint.x() * scaleFactorX );
    objectData.startPoint.setY( marginTop  + objectData.startPoint.y() * scaleFactorY );
    objectData.endPoint.setX( marginLeft + objectData.endPoint.x() * scaleFactorX );
    objectData.endPoint.setY( marginTop  + objectData.endPoint.y() * scaleFactorY );

    _ellipses.add(objectData);
  }
  // Texts
  for(int i=0; i<userTextsCount; i++){
    UserTextData textData;
    in >> textData.data.startPoint >> textData.data.endPoint >> textData.data.pen >> textData.data.highlight;
    in >> textData.caretPos >> textData.text;

    textData.data.startPoint.setX( marginLeft + textData.data.startPoint.x() * scaleFactorX );
    textData.data.startPoint.setY( marginTop  + textData.data.startPoint.y() * scaleFactorY );
    textData.data.endPoint.setX( marginLeft + textData.data.endPoint.x() * scaleFactorX );
    textData.data.endPoint.setY( marginTop  + textData.data.endPoint.y() * scaleFactorY );

    _texts.add(textData);
  }
  // Free forms
  for(int i=0; i<userFreeFormsCount; i++){
    UserFreeFormData freeFormData;
    QList<QPoint> points;

    in >> points;
    in >> freeFormData.pen >> freeFormData.active >> freeFormData.highlight;

    for(int x=0; x<points.size(); x++){
      QPoint point = points.at(x);

      point.setX( marginLeft + point.x() * scaleFactorX );
      point.setY( marginTop  + point.y() * scaleFactorY );

      freeFormData.points.append(point);
    }

    _freeForms.add(freeFormData);
  }

  if(in.atEnd())
    logUser(LOG_SUCCESS, "", "Recovery algorithm finished successfully (reached End Of File)");
  else
    logUser(LOG_ERROR_AND_EXIT, "", "There is data left in the ZoomMe file that was not loaded by the recovery algorithm (because it ended before the EOF). Please check the saving and the recovery algorithm: There may be some variables missing in the recovery and not in the saving or some variables added in the saving but not in the recovery...");
}

void ZoomWidget::createVideoFFmpeg()
{
  QString resolution;
  resolution.append(QString::number(_canvas.pixmap.width()));
  resolution.append("x");
  resolution.append(QString::number(_canvas.pixmap.height()));

  // Read the video bytes and pipe it to FFmpeg...
  // Arguments for FFmpeg taken from:
  // https://github.com/tsoding/rendering-video-in-c-with-ffmpeg/blob/master/ffmpeg_linux.c
  QList<QString> arguments;

  // GLOBAL ARGS
  arguments << "-hide_banner"
            << "-loglevel"  << "warning"
            << "-y"
  // INPUT ARGS
            // No rawvideo because it's now compressed in jpeg
            // << "-f"         << "rawvideo"
            << "-pix_fmt"   << "yuv420p" // Indicates that the file contains JPEG images
            << "-s"         << resolution
            << "-r"         << QString::number(RECORD_FPS)
            << "-i"         << recordTempFile->fileName()
  // OUTPUT ARGS
            // Commented to add support to GIF, for example
            // << "-c:v"       << "libx264"
            // There's no audio, so no need for this flags
            // << "-c:a"       << "aac"
            // << "-ab"        << "200k"
            << "-vb"        << "2500k"
            // Commented because of a warning from FFmpeg
            // https://superuser.com/questions/1273920/deprecated-pixel-format-used-make-sure-you-did-set-range-correctly
            // << "-pix_fmt"   << "yuv420p"
            << getFilePath(FILE_VIDEO);

  // Start process
  ffmpeg.start("ffmpeg", arguments);

  updateCursorShape();

  const int timeout = 10000;
  if (!ffmpeg.waitForStarted(timeout)){
    logUser(LOG_ERROR, "Couldn't start FFmpeg. Maybe it is not installed...",
                       "Couldn't start ffmpeg or timeout occurred (%.1f sec.). Maybe FFmpeg is not installed. Killing the ffmpeg process...", ((float)timeout/1000.0));
    logUser(LOG_TEXT, "", "  - Error: %s", QSTRING_TO_STRING(ffmpeg.errorString()));
    logUser(LOG_TEXT, "", "  - Executed command: ffmpeg %s", QSTRING_TO_STRING(arguments.join(" ")));
    ffmpeg.kill();
    return;
  }

  if(!ffmpeg.waitForFinished(-1)){
    logUser(LOG_ERROR, "", "An error occurred with FFmpeg: %s", QSTRING_TO_STRING(ffmpeg.errorString()));
    return;
  }

  if(ffmpeg.exitStatus() == QProcess::CrashExit) {
    logUser(LOG_ERROR, "","FFmpeg crashed");
    return;
  }

  if(ffmpeg.exitCode() != 0) {
    logUser(LOG_ERROR, "", "FFmpeg failed. Exit code: %d", ffmpeg.exitCode());
    return;
  }

  logUser(LOG_SUCCESS, "", "Video encoding was successful! FFmpeg finished without any error...", QSTRING_TO_STRING(getFilePath(FILE_VIDEO)));
  QApplication::beep();
}

void ZoomWidget::saveFrameToFile()
{
  QImage image = _canvas.pixmap.toImage();

  // Save the image as jpeg into a byte array (is not a raw image, it's
  // compressed)
  QByteArray imageBytes;
  QBuffer buffer(&imageBytes); buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "JPEG", RECORD_QUALITY);

  recordTempFile->write(imageBytes);
}

QRect fixQRect(int x, int y, int width, int height)
{
  // The width and height of the rectangle must be positive, otherwise strange
  // things happen, like only showing the first word on texts, or draw an
  // ellipse instead of rectangles
  if(width < 0)  { x+=width;  width=abs(width);   }
  if(height < 0) { y+=height; height=abs(height); }

  return QRect(x, y, width, height);
}

void updateFontSize(QPainter *painter)
{
  QFont font;
  font.setPixelSize(painter->pen().width() * FONT_SIZE_FACTOR);
  painter->setFont(font);
}

void changePenWidth(QPainter *painter, int width)
{
  QPen pen = painter->pen();
  pen.setWidth(width);
  painter->setPen(pen);
}

ArrowHead ZoomWidget::getArrowHead(int x, int y, int width, int height)
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

  return ArrowHead {
    QPoint(originX, originY),
    QPoint(originX+leftLineX, originY+leftLineY),
    QPoint(originX+rightLineX, originY+rightLineY),
  };
}

bool ZoomWidget::isDrawingHovered(ZoomWidgetDrawMode drawType, int vectorPos)
{
  // Only if it's deleting or if it's trying to modify a text
  bool isDeleting = (_state == STATE_DELETING);
  if(!isDeleting && !isTextEditable(GET_CURSOR_POS()))
    return false;

  // This is the position of the form (in the current draw mode) in the vector,
  // that is behind the cursor.
  int posFormBehindCursor = cursorOverForm(GET_CURSOR_POS());

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

void ZoomWidget::drawButton(QPainter *screenPainter, Button button)
{
  QFont font;
  font.setPixelSize(16);
  screenPainter->setFont(font);

  // Background
  QColor color = QCOLOR_TOOL_BAR;
  color.setAlpha(55); // Transparency
  QPainterPath buttonBg;
  buttonBg.addRoundedRect(button.rect, POPUP_ROUNDNESS_FACTOR, POPUP_ROUNDNESS_FACTOR);
  screenPainter->fillPath(buttonBg, color);

  // Button
  const bool isUnderCursor = (button.rect.contains(GET_CURSOR_POS()));
  const bool isActive      = (isButtonActive(button) == BUTTON_ACTIVE);
  const bool isDisabled    = (isButtonActive(button) == BUTTON_DISABLED);

  screenPainter->setPen(QCOLOR_TOOL_BAR);
  if(isUnderCursor) {
    invertColorPainter(screenPainter);
  }
  if(isActive && !isUnderCursor) {
    screenPainter->setPen(QCOLOR_GREEN);
  }
  if(isDisabled) {
    screenPainter->setPen(QCOLOR_TOOL_BAR_DISABLED);
  }

  screenPainter->drawRoundedRect(button.rect, POPUP_ROUNDNESS_FACTOR, POPUP_ROUNDNESS_FACTOR);
  screenPainter->drawText(button.rect, Qt::AlignCenter | Qt::TextWrapAnywhere, button.name);
}

void ZoomWidget::drawToolBar(QPainter *screenPainter)
{
  // Color of the background
  QColor color = QCOLOR_BLACK;
  color.setAlpha(240); // Transparency

  // Increase a little bit the background of the tool bar for painting it (like
  // a padding)
  QRect bgRect = _toolBar.rect;
  bgRect.setX(bgRect.x() - _toolBar.margin/2);
  bgRect.setY(bgRect.y() - _toolBar.margin/2);
  bgRect.setWidth(bgRect.width() + _toolBar.margin/2);
  bgRect.setHeight(bgRect.height() + _toolBar.margin/2);

  // Paint
  QPainterPath background;
  background.addRoundedRect(bgRect, POPUP_ROUNDNESS_FACTOR, POPUP_ROUNDNESS_FACTOR);
  screenPainter->fillPath(background, color);

  // Draw buttons
  for(int i=0; i<_toolBar.buttons.size(); i++) {
    const Button btn = _toolBar.buttons.at(i);

    if(btn.action != ACTION_SPACER)
      drawButton(screenPainter, btn);
  }
}

void ZoomWidget::closePopupUnderCursor(const QPoint cursorPos)
{
  const qint64 time = QDateTime::currentMSecsSinceEpoch();

  int popupPos = -1;
  for(int i=0; i<_popupTray.popups.size(); i++) {
    const QRect p = getPopupRect(i);
    if( isCursorInsideHitBox(p.x(), p.y(), p.width(), p.height(), cursorPos, true) ) {
      popupPos = i;
      break;
    }
  }

  if(popupPos == -1) {
    logUser(LOG_ERROR, "", "Couldn't find the pop-up under the cursor");
    return;
  }

  Popup edit = _popupTray.popups.takeAt(popupPos);

  const int timeConsumed = time - edit.timeCreated;
  const int visibleEnd = edit.lifetime  - POPUP_SLIDE_OUT_MSEC;
  // Don't change the time if it's already sliding out
  if(timeConsumed < visibleEnd)
    edit.timeCreated = time - visibleEnd;

  _popupTray.popups.insert(popupPos, edit);
}

bool ZoomWidget::isPressingPopup(const QPoint cursorPos)
{
  if(_state != STATE_MOVING)
    return false;

  if(_screenOpts == SCREENOPTS_HIDE_ALL || _screenOpts == SCREENOPTS_HIDE_FLOATING)
    return false;

  for(int i=0; i<_popupTray.popups.size(); i++) {
    const QRect p = getPopupRect(i);
    if( isCursorInsideHitBox(p.x(), p.y(), p.width(), p.height(), cursorPos, true) )
      return true;
  }

  return false;
}

QRect ZoomWidget::getPopupRect(const int listPos)
{
  // Invert the position for the calculation of the 'y' axis: newest at the top
  const int popupLevel = (_popupTray.popups.size()-1) - listPos;

  return QRect(
        _popupTray.start.x(),
        _popupTray.start.y() + popupLevel * (_popupTray.margin+POPUP_HEIGHT),
        POPUP_WIDTH,
        POPUP_HEIGHT
      );
}

void ZoomWidget::drawPopup(QPainter *screenPainter, const int listPos)
{
  const Popup p = _popupTray.popups.at(listPos);
  const qint64 time = QDateTime::currentMSecsSinceEpoch();
  const int timeConsumed = time - p.timeCreated;
  const int fontSize   = 16;
  const int penWidth   = 5;
  const int progressCircleRadius = 4 + 2 * sin((float)timeConsumed/POPUP_CIRCLE_INTENSITY); // the sin of time/intensity
  // const int progressCircleRadius = 4;
  const int titleBorderWidth     = 1;
  const int titleHeight = 30;
  const int textPadding = 10;

  QRect popupRect = getPopupRect(listPos);
  float alphaPercentage = 1;

  // 0                lifetime
  // |==|----------|==|
  // |e | visible  | e| (e --> effect)
  const float endVisible = (float)p.lifetime - POPUP_SLIDE_OUT_MSEC;
  if(timeConsumed >= endVisible) {
    const float lifeInLastSection = timeConsumed - endVisible;
    float percentageInLastSection = lifeInLastSection / (float)POPUP_SLIDE_OUT_MSEC; // 0.0 to 1.0
    if(percentageInLastSection > 1) percentageInLastSection = 1; // Sometimes it overflows to 1.0XXXX

    // SLIDE OUT
    const int slideOut = (popupRect.x() + popupRect.width()) * percentageInLastSection;
    popupRect.setX(popupRect.x() - slideOut);
    popupRect.setWidth(popupRect.width() - slideOut);
    // FADE OUT
    alphaPercentage = 1-percentageInLastSection;
  }

  // MAIN COLOR AND TITLE TEXT
  QColor color;
  QString title;
  switch(p.urgency) {
    case LOG_INFO:    color = QCOLOR_CYAN;  title="INFORMATION"; break;
    case LOG_SUCCESS: color = QCOLOR_GREEN; title="SUCCESS";     break;
    case LOG_ERROR:   color = QCOLOR_RED;   title="ERROR";       break;

    case LOG_TEXT:
    case LOG_ERROR_AND_EXIT:
      logUser(LOG_ERROR_AND_EXIT, "", "An error happened. You shouldn't print a popup with LOG_ERROR_AND_EXIT or LOG_TEXT");
      break;
  }

  // PAINTER CONFIGURATIONS
  QFont font; font.setPixelSize(fontSize); screenPainter->setFont(font);
  color.setAlpha(255 * alphaPercentage);
  screenPainter->setPen(color);
  changePenWidth(screenPainter, penWidth);

  // BACKGROUND
  QPainterPath background;
  background.addRoundedRect(popupRect, POPUP_ROUNDNESS_FACTOR, POPUP_ROUNDNESS_FACTOR);
    // Contrast
  QColor contrast = QCOLOR_BLACK;
  contrast.setAlpha(175 * alphaPercentage); // Transparency
  screenPainter->fillPath(background, contrast);
    // Highlight
  color.setAlpha(65 * alphaPercentage); // Transparency
  screenPainter->fillPath(background, color);

  // MAIN TEXT AND BORDERS
  screenPainter->drawRoundedRect(popupRect, POPUP_ROUNDNESS_FACTOR, POPUP_ROUNDNESS_FACTOR);
  screenPainter->drawText(
        popupRect.x() + textPadding,
        popupRect.y() + textPadding + titleHeight,
        popupRect.width()  - 2*textPadding,
        popupRect.height() - 2*textPadding - titleHeight,
        Qt::AlignCenter | Qt::TextWordWrap,
        p.message
      );

  // TITLE
  const QPoint startDivider(
        popupRect.x(),
        popupRect.y() + titleHeight
      );
  const QPoint endDivider(
        startDivider.x() + popupRect.width(),
        startDivider.y()
      );
  const QPoint endDividerProgress(
        startDivider.x() + popupRect.width() * (1 - (float)timeConsumed/(float)p.lifetime),
        startDivider.y()
      );
    // Title text
  font.setBold(true); screenPainter->setFont(font);
  screenPainter->drawText(
        popupRect.x(),     popupRect.y(),
        popupRect.width(), titleHeight,
        Qt::AlignCenter | Qt::TextWordWrap,
        title
      );
    // Progress line and circle
  screenPainter->drawLine(startDivider, endDividerProgress);
  screenPainter->setBrush(screenPainter->pen().color()); // Fill
  screenPainter->drawEllipse(
        endDividerProgress.x() - progressCircleRadius,
        endDividerProgress.y() - progressCircleRadius,
        2 * progressCircleRadius,
        2 * progressCircleRadius
      );
  screenPainter->setBrush(QBrush()); // No fill
    // Static divider Line
  changePenWidth(screenPainter, titleBorderWidth);
  screenPainter->drawLine(endDividerProgress, endDivider);
}

void ZoomWidget::setPopupTrayPos()
{
  const qint64 time = QDateTime::currentMSecsSinceEpoch();

  // SLIDE IN
  int slideInTray = 0;
  for(int i=0; i<_popupTray.popups.size(); i++) {
    Popup p = _popupTray.popups.at(i);
    const int timeConsumed = time - p.timeCreated;

    // 0                lifetime
    // |==|----------|==|
    // |e | visible  | e| (e --> effect)
    if(timeConsumed <= POPUP_SLIDE_IN_MSEC) {
      const float percentageInFirstSection = (float)timeConsumed / (float)POPUP_SLIDE_IN_MSEC; // 0.0 to 1.0
      const int slideIn = (POPUP_HEIGHT + _popupTray.margin) * (1-percentageInFirstSection);
      slideInTray += slideIn;
    }
  }

  _popupTray.start = QPoint(
        _popupTray.margin,
        _popupTray.margin - slideInTray
      );
}

void ZoomWidget::drawPopupTray(QPainter *screenPainter)
{
  if(_screenOpts == SCREENOPTS_HIDE_ALL || _screenOpts == SCREENOPTS_HIDE_FLOATING)
    return;

  if(_popupTray.popups.size() == 0)
    return;

  setPopupTrayPos();

  for(int i=_popupTray.popups.size()-1; i>=0; i--)
    drawPopup(screenPainter, i);
}

void ZoomWidget::drawStatus(QPainter *screenPainter)
{
  if(_screenOpts == SCREENOPTS_HIDE_ALL || _screenOpts == SCREENOPTS_HIDE_FLOATING)
    return;

  const int lineHeight        = 25;
  const int margin            = 20;
  const int penWidth          = 5;
  const int fontSize          = 16;
  const int w                 = 180;
  const int initialLineHeight = lineHeight + 5; // lineHeight + margin

  int h = initialLineHeight;

  // Text to display
  QString text;

  // Line 1 -ALWAYS DISPLAYING-
  if(isDisabledMouseTracking())
    text.append(BLOCK_ICON);
  else
    text.append( (_canvas.scale == 1.0f) ? NO_ZOOM_ICON : ZOOM_ICON );
  text.append(" ");

  switch(_drawMode) {
    case DRAWMODE_LINE:      text.append("Line");        break;
    case DRAWMODE_RECT:      text.append("Rectangle");   break;
    case DRAWMODE_ARROW:     text.append("Arrow");       break;
    case DRAWMODE_ELLIPSE:   text.append("Ellipse");     break;
    case DRAWMODE_TEXT:      text.append("Text");        break;
    case DRAWMODE_FREEFORM:  text.append("Free Form");   break;
  }

  if(_highlight) {
    text.append(" ");
    text.append(HIGHLIGHT_ICON);
  }

  text.append(" (");
  text.append(QString::number(_activePen.width()));
  text.append(")");

  // Line 2
  switch(_state) {
    case STATE_MOVING:       break;
    case STATE_DRAWING:      break;
    case STATE_TYPING:       text.append("\n-- TYPING --");     h += lineHeight; break;
    case STATE_DELETING:     text.append("\n-- DELETING --");   h += lineHeight; break;
    case STATE_COLOR_PICKER: text.append("\n-- PICK COLOR --"); h += lineHeight; break;
    case STATE_TO_TRIM:
    case STATE_TRIMMING:     text.append("\n-- TRIMMING --");   h += lineHeight; break;
  };
  if(isTextEditable(GET_CURSOR_POS())){
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
  if(_exitConfirm) {
    text.append("\n");
    text.append(EXIT_ICON);
    text.append(" EXIT? ");
    text.append(EXIT_ICON);

    h += lineHeight;
  }

  // Position
  const int x = _screenSize.width() - w - margin;
  const int y = margin;

  // If the mouse is near the hit box, don't draw it
  QRect hitBox = QRect(x-margin, y-margin, w+margin*2, h+margin*2);
  if( isCursorInsideHitBox( hitBox.x(),
                            hitBox.y(),
                            hitBox.width(),
                            hitBox.height(),
                            GET_CURSOR_POS(),
                            true) ) {
    return;
  }

  const QRect rect = QRect(x, y, w, h);

  // Settings
  screenPainter->setPen(_activePen);
  QFont font; font.setPixelSize(fontSize); screenPainter->setFont(font);
  changePenWidth(screenPainter, penWidth);

  // Rounded background
  QPainterPath bgPath;
  bgPath.addRoundedRect(rect, POPUP_ROUNDNESS_FACTOR, POPUP_ROUNDNESS_FACTOR);

  // Background (highlight) to improve contrast
  QColor color = (_activePen.color() == QCOLOR_BLACK) ? QCOLOR_WHITE : QCOLOR_BLACK;
  color.setAlpha(175); // Transparency
  screenPainter->fillPath(bgPath, color);

  // Background (highlight) for current color
  color = _activePen.color();
  color.setAlpha(65); // Transparency
  screenPainter->fillPath(bgPath, color);

  // Border
  screenPainter->drawRoundedRect(rect, POPUP_ROUNDNESS_FACTOR, POPUP_ROUNDNESS_FACTOR);

  // Text
  screenPainter->drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, text);
}

void ZoomWidget::drawSavedForms(QPainter *pixmapPainter)
{
  if(_screenOpts == SCREENOPTS_HIDE_ALL)
    return;

  // Draw user rectangles.
  int x, y, w, h;
  for (int i = 0; i < _rects.size(); ++i) {
    pixmapPainter->setPen(_rects.at(i).pen);
    getRealUserObjectPos(_rects.at(i), &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_RECT, i))
      invertColorPainter(pixmapPainter);

    if(_rects.at(i).highlight) {
      QColor color = pixmapPainter->pen().color();
      color.setAlpha(HIGHLIGHT_ALPHA); // Transparency
      QPainterPath background;
      background.addRoundedRect(x, y, w, h, RECT_ROUNDNESS_FACTOR, RECT_ROUNDNESS_FACTOR);
      pixmapPainter->fillPath(background, color);
    }

    pixmapPainter->drawRoundedRect(fixQRect(x, y, w, h), RECT_ROUNDNESS_FACTOR, RECT_ROUNDNESS_FACTOR);
  }

  // Draw user lines.
  for (int i = 0; i < _lines.size(); ++i) {
    pixmapPainter->setPen(_lines.at(i).pen);
    getRealUserObjectPos(_lines.at(i), &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_LINE, i))
      invertColorPainter(pixmapPainter);

    // Draw a wider semi-transparent line behind the line as the highlight
    if(_lines.at(i).highlight) {
      QPen oldPen = pixmapPainter->pen();

      // Change the color and width of the pen
      QPen newPen = oldPen;
      QColor color = oldPen.color(); color.setAlpha(HIGHLIGHT_ALPHA); newPen.setColor(color);
      newPen.setWidth(newPen.width() * 4);
      pixmapPainter->setPen(newPen);

      pixmapPainter->drawLine(x, y, x+w, y+h);

      // Reset pen
      pixmapPainter->setPen(oldPen);
    }

    pixmapPainter->drawLine(x, y, x+w, y+h);
  }

  // Draw user arrows.
  for (int i = 0; i < _arrows.size(); ++i) {
    pixmapPainter->setPen(_arrows.at(i).pen);
    getRealUserObjectPos(_arrows.at(i), &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_ARROW, i))
      invertColorPainter(pixmapPainter);

    ArrowHead head = getArrowHead(x, y, w, h);

    // Draw a wider semi-transparent line behind the line as the highlight
    if(_arrows.at(i).highlight) {
      QPen oldPen = pixmapPainter->pen();

      // Change the color and width of the pen
      QPen newPen = oldPen;
      QColor color = oldPen.color(); color.setAlpha(HIGHLIGHT_ALPHA); newPen.setColor(color);
      newPen.setWidth(newPen.width() * 4);
      pixmapPainter->setPen(newPen);

      pixmapPainter->drawLine(x, y, x+w, y+h);
      pixmapPainter->drawLine(head.startPoint, head.rightLineEnd);
      pixmapPainter->drawLine(head.startPoint, head.leftLineEnd);

      // Reset pen
      pixmapPainter->setPen(oldPen);
    }

    pixmapPainter->drawLine(x, y, x+w, y+h);
    pixmapPainter->drawLine(head.startPoint, head.rightLineEnd);
    pixmapPainter->drawLine(head.startPoint, head.leftLineEnd);
  }

  // Draw user ellipses.
  for (int i = 0; i < _ellipses.size(); ++i) {
    pixmapPainter->setPen(_ellipses.at(i).pen);
    getRealUserObjectPos(_ellipses.at(i), &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_ELLIPSE, i))
      invertColorPainter(pixmapPainter);

    if(_ellipses.at(i).highlight) {
      QColor color = pixmapPainter->pen().color();
      color.setAlpha(HIGHLIGHT_ALPHA); // Transparency
      QPainterPath background;
      background.addEllipse(x, y, w, h);
      pixmapPainter->fillPath(background, color);
    }

    pixmapPainter->drawEllipse(x, y, w, h);
  }

  // Draw user FreeForms.
  // If the last one is currently active, draw it in the "active forms" switch
  int freeFormCount = (!_freeForms.isEmpty() && _freeForms.last().active)
                      ? _freeForms.size()-1
                      : _freeForms.size();
  for (int i = 0; i < freeFormCount; ++i) {
    pixmapPainter->setPen(_freeForms.at(i).pen);

    if(isDrawingHovered(DRAWMODE_FREEFORM, i))
      invertColorPainter(pixmapPainter);

    // Draw the free form with or without the highlight
    if(_freeForms.at(i).highlight) {
      QPolygon polygon(_freeForms.at(i).points);

      // Highlight
      QColor color = pixmapPainter->pen().color();
      color.setAlpha(HIGHLIGHT_ALPHA); // Transparency
      QPainterPath background;
      background.addPolygon(polygon);
      pixmapPainter->fillPath(background, color);

      pixmapPainter->drawPolygon(polygon);
    } else {
      for (int z = 0; z < _freeForms.at(i).points.size()-1; ++z) {
        QPoint current = _freeForms.at(i).points.at(z);
        QPoint next    = _freeForms.at(i).points.at(z+1);

        pixmapPainter->drawLine(current.x(), current.y(), next.x(), next.y());
      }
    }
  }

  // Draw user Texts.
  // If the last one is currently active (user is typing), draw it in the
  // "active text" `if` statement
  int textsCount = _state == STATE_TYPING ? _texts.size()-1 : _texts.size();
  for (int i = 0; i < textsCount; ++i) {
    pixmapPainter->setPen(_texts.at(i).data.pen);
    updateFontSize(pixmapPainter);
    getRealUserObjectPos(_texts.at(i).data, &x, &y, &w, &h, false);

    if(isDrawingHovered(DRAWMODE_TEXT, i))
      invertColorPainter(pixmapPainter);

    if(_texts.at(i).data.highlight) {
      QColor color = pixmapPainter->pen().color();
      color.setAlpha(HIGHLIGHT_ALPHA); // Transparency
      QPainterPath background;
      background.addRect(x, y, w, h);
      pixmapPainter->fillPath(background, color);
      pixmapPainter->drawRect(x, y, w, h);
    }

    QString text = _texts.at(i).text;
    pixmapPainter->drawText(fixQRect(x, y, w, h), Qt::AlignCenter | Qt::TextWordWrap, text);
  }
}

void ZoomWidget::drawFlashlightEffect(QPainter *painter, bool drawToScreen)
{
  if(_state == STATE_TRIMMING)
    return;

  const int radius = _flashlightRadius;
  QPoint c = GET_CURSOR_POS();

  if(!drawToScreen)
    c = screenPointToPixmapPos(c);

  QRect mouseFlashlightBorder = QRect(c.x()-radius, c.y()-radius, radius*2, radius*2);
  QPainterPath mouseFlashlight;
  mouseFlashlight.addEllipse( mouseFlashlightBorder );

  // painter->setPen(QColor(186,186,186,200));
  // painter->drawEllipse( mouseFlashlightBorder );

  QPainterPath pixmapPath;
  pixmapPath.addRect(_canvas.pixmap.rect());

  QPainterPath flashlightArea = pixmapPath.subtracted(mouseFlashlight);
  painter->fillPath(flashlightArea, QColor(  0,  0,  0, 190));
}

void ZoomWidget::drawTrimmed(QPainter *pixmapPainter)
{
  QRect rect(_startDrawPoint, _endDrawPoint);
  QPainterPath mouseFlashlight;
  mouseFlashlight.addRect(rect);

  QPainterPath pixmapPath;
  pixmapPath.addRect(_canvas.pixmap.rect());

  QPainterPath opaqueArea = pixmapPath.subtracted(mouseFlashlight);
  pixmapPainter->fillPath(opaqueArea, QColor(  0,  0,  0, 190));
}

void ZoomWidget::drawActiveForm(QPainter *painter, bool drawToScreen)
{
  if(_screenOpts == SCREENOPTS_HIDE_ALL)
    return;

  // If it's writing the text (active text)
  if(_state == STATE_TYPING) {
    UserTextData textObject = _texts.last();
    int x, y, w, h;
    painter->setPen(textObject.data.pen);
    updateFontSize(painter);
    getRealUserObjectPos(textObject.data, &x, &y, &w, &h, drawToScreen);

    QString text = textObject.text;
    if(text.isEmpty()) text="Type some text... \nThen press Enter to finish...";
    else text.insert(textObject.caretPos, '|');

    if(_highlight) {
      QColor color = textObject.data.pen.color();
      color.setAlpha(HIGHLIGHT_ALPHA); // Transparency
      QPainterPath background;
      background.addRect(x, y, w, h);
      painter->fillPath(background, color);

      invertColorPainter(painter);
      painter->drawRect(x, y, w, h);
    } else {
      changePenWidth(painter, 1);
      invertColorPainter(painter);
      painter->drawRoundedRect(fixQRect(x, y, w, h), RECT_ROUNDNESS_FACTOR, RECT_ROUNDNESS_FACTOR);
    }

    invertColorPainter(painter);
    painter->drawText(fixQRect(x, y, w, h), Qt::AlignCenter | Qt::TextWordWrap, text);
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

    // For the highlight
    QColor color = painter->pen().color();
    color.setAlpha(HIGHLIGHT_ALPHA); // Transparency
    QPainterPath background;

    switch(_drawMode) {
      case DRAWMODE_RECT:
        painter->drawRoundedRect(fixQRect(x, y, width, height), RECT_ROUNDNESS_FACTOR, RECT_ROUNDNESS_FACTOR);

        if(_highlight) {
          background.addRoundedRect(x, y, width, height, RECT_ROUNDNESS_FACTOR, RECT_ROUNDNESS_FACTOR);
          painter->fillPath(background, color);
        }
        break;
      case DRAWMODE_LINE:
        // Draw a wider semi-transparent line behind the line as the highlight
        if(_highlight) {
          QPen oldPen = painter->pen();

          // Change the color and width of the pen
          QPen newPen = oldPen;
          newPen.setColor(color);
          newPen.setWidth(newPen.width() * 4);
          painter->setPen(newPen);

          painter->drawLine(x, y, x+width, y+height);

          // Reset pen
          painter->setPen(oldPen);
        }

        painter->drawLine(x, y, width + x, height + y);
        break;
      case DRAWMODE_ARROW: {
          ArrowHead head = getArrowHead(x, y, width, height);

          // Draw a wider semi-transparent line behind the line as the highlight
          if(_highlight) {
            QPen oldPen = painter->pen();

            // Change the color and width of the pen
            QPen newPen = oldPen;
            newPen.setColor(color);
            newPen.setWidth(newPen.width() * 4);
            painter->setPen(newPen);

            painter->drawLine(x, y, x+width, y+height);
            painter->drawLine(head.startPoint, head.rightLineEnd);
            painter->drawLine(head.startPoint, head.leftLineEnd);

            // Reset pen
            painter->setPen(oldPen);
          }

          painter->drawLine(x, y, width + x, height + y);
          painter->drawLine(head.startPoint, head.rightLineEnd);
          painter->drawLine(head.startPoint, head.leftLineEnd);
          break;
        }
      case DRAWMODE_ELLIPSE:
        painter->drawEllipse(x, y, width, height);

        if(_highlight) {
          background.addEllipse(x, y, width, height);
          painter->fillPath(background, color);
        }
        break;
      case DRAWMODE_TEXT:
        {
          updateFontSize(painter);

          if(_highlight) {
            background.addRoundedRect(x, y, width, height, RECT_ROUNDNESS_FACTOR, RECT_ROUNDNESS_FACTOR);
            painter->fillPath(background, color);

            invertColorPainter(painter);
            painter->drawRoundedRect(x, y, width, height, RECT_ROUNDNESS_FACTOR, RECT_ROUNDNESS_FACTOR);
          } else {
            changePenWidth(painter, 1);
            invertColorPainter(painter);
            painter->drawRoundedRect(fixQRect(x, y, width, height), RECT_ROUNDNESS_FACTOR, RECT_ROUNDNESS_FACTOR);
          }
          invertColorPainter(painter);

          QString defaultText;
          defaultText.append("Sizing... (");
          defaultText.append(QString::number(abs(width)));
          defaultText.append("x");
          defaultText.append(QString::number(abs(height)));
          defaultText.append(")");
          painter->drawText(fixQRect(x, y, width, height), Qt::AlignCenter | Qt::TextWordWrap, defaultText);
          break;
        }
      case DRAWMODE_FREEFORM:
        if(_freeForms.isEmpty())
          break;
        if(!_freeForms.last().active)
          break;

        painter->setPen(_freeForms.last().pen);

        // Draw the free form with or without the highlight
        if(_freeForms.last().highlight) {
          QPolygon polygon;
          for (int i = 0; i < _freeForms.last().points.size(); ++i) {
            QPoint point = _freeForms.last().points.at(i);
            if(drawToScreen) point = pixmapPointToScreenPos(point);

            polygon << point;
          }

          background.addPolygon(polygon);
          painter->fillPath(background, color);

          painter->drawPolygon(polygon);
        } else {
          for (int i = 0; i < _freeForms.last().points.size()-1; ++i) {
            QPoint current = _freeForms.last().points.at(i);
            QPoint next    = _freeForms.last().points.at(i+1);

            if(drawToScreen) {
              current = pixmapPointToScreenPos(current);
              next = pixmapPointToScreenPos(next);
            }

            painter->drawLine(current.x(), current.y(), next.x(), next.y());
          }
        }
        break;
    }
  }
}

void ZoomWidget::paintEvent(QPaintEvent *event)
{
  (void) event;

  // Exit if the _sourcePixmap is not initialized (not ready)
  if(_sourcePixmap.isNull())
    logUser(LOG_ERROR_AND_EXIT, "", "The desktop pixmap is null. Can't paint over a null pixmap");

  _canvas.pixmap = _sourcePixmap;

  if(_liveMode)
    _canvas.pixmap.fill(Qt::transparent);

  if(_boardMode)
    _canvas.pixmap.fill(QCOLOR_BLACKBOARD);

  QPainter pixmapPainter(&_canvas.pixmap);
  QPainter screen; screen.begin(this);

  drawSavedForms(&pixmapPainter);
  if(_state == STATE_TRIMMING)
    drawTrimmed(&pixmapPainter);

  // By drawing the active form in the pixmap, it gives a better user feedback
  // (because the user can see how it would really look like when saved), but
  // when the flashlight effect is on, its better to draw the active form onto
  // the screen, on top of the flashlight effect, so that the user can see the
  // active form over the opaque background. This can cause some differences
  // with the final result (like the width of the pen and the size of the
  // arrow's head)
  // By the way, ¿Why would you draw when the flashlight effect is enabled? I
  // don't know why I'm allowing this... You can't even see the cursor!
  if(_flashlightMode && !IS_RECORDING) {
    drawDrawnPixmap(&screen);
    drawFlashlightEffect(&screen, true);
    drawActiveForm(&screen, true);
  } else {
    if(_flashlightMode)
      drawFlashlightEffect(&pixmapPainter, false);
    drawActiveForm(&pixmapPainter, false);
    drawDrawnPixmap(&screen);
  }

  drawStatus(&screen);
  drawPopupTray(&screen);
  if(isToolBarVisible())
    drawToolBar(&screen);

  // ONLY FOR DEBUG PURPOSE OF THE HIT BOX
  // int x, y, w, h;
  // for (int i = 0; i < _tests.size(); ++i) {
  //   screen.setPen(_tests.at(i).pen);
  //   getRealUserObjectPos(_tests.at(i), &x, &y, &w, &h, false);
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
    case DRAWMODE_LINE:      _lines.remove(formPosBehindCursor);     break;
    case DRAWMODE_RECT:      _rects.remove(formPosBehindCursor);     break;
    case DRAWMODE_ARROW:     _arrows.remove(formPosBehindCursor);    break;
    case DRAWMODE_ELLIPSE:   _ellipses.remove(formPosBehindCursor);  break;
    case DRAWMODE_TEXT:      _texts.remove(formPosBehindCursor);     break;
    case DRAWMODE_FREEFORM:  _freeForms.remove(formPosBehindCursor); break;
  }

  _state = STATE_MOVING;
  updateCursorShape();
  update();
}

void ZoomWidget::mousePressEvent(QMouseEvent *event)
{
  // The cursor pos is relative to the resolution of scaled monitor
  const QPoint cursorPos = event->pos();

  // Pre mouse processing
  if(isCursorOverButton(cursorPos)) {
    const int buttonPos = buttonBehindCursor(cursorPos);
    toggleAction(_toolBar.buttons.at(buttonPos).action);
    updateCursorShape();
    update();
    return;
  }

  if(isPressingPopup(cursorPos)) {
    closePopupUnderCursor(cursorPos);
    update();
    return;
  }

  // Drag the pixmap
  if(event->button() == DRAG_MOUSE_BUTTON && isDisabledMouseTracking()) {
    _canvas.dragging = true;
    updateCursorShape();
    update();
    return;
  }

  if(_screenOpts == SCREENOPTS_HIDE_ALL)
    return;

  // Mouse processing
  if(_state == STATE_COLOR_PICKER){
    _activePen.setColor(GET_COLOR_UNDER_CURSOR());
    _state = STATE_MOVING;
    update();
    updateCursorShape();
    return;
  }

  if (_state == STATE_TYPING && _texts.last().text.isEmpty())
    _texts.destroyLast();

  if(_state == STATE_DELETING)
    return removeFormBehindCursor(cursorPos);

  // If you're in text mode (without drawing nor writing) and you press a text
  // with shift pressed, you access it and you can modify it
  if(isTextEditable(cursorPos)) {
    _state = STATE_TYPING;
    updateCursorShape();

    int formPosBehindCursor = cursorOverForm(cursorPos);
    // Put the text at the top of the list, to edit it
    _texts.moveToTop(formPosBehindCursor);

    if(event->modifiers() == Qt::ShiftModifier)
      _canvas.freezePos = FREEZE_BY_TEXT;

    update();
    return;
  }

  if(_state == STATE_TO_TRIM)
    _state = STATE_TRIMMING;
  else
    _state = STATE_DRAWING;

  _startDrawPoint = screenPointToPixmapPos(cursorPos);
  _endDrawPoint = _startDrawPoint;
}

// If toImage it's true, then it's saved in a image file, otherwise, it gets
// saved in the clipboard
void ZoomWidget::saveImage(QPixmap pixmap, bool toImage)
{
  if(toImage) {
     QString path = getFilePath(FILE_IMAGE);
     if(pixmap.save(path)) {
       QApplication::beep();
       logUser(LOG_SUCCESS, "Image saved correctly!", "Image saved correctly: %s", QSTRING_TO_STRING(path));
     } else {
       logUser(LOG_ERROR, "", "Couldn't save the picture to: %s", QSTRING_TO_STRING(path));
     }
     return;
  }

  // Clipboard
#ifdef Q_OS_LINUX
  // Save the image in a temp folder and load it to the clipboard with
  // xclip, because with QClipboard in linux, the image gets deleted when
  // closing the app. Apparently in other systems the other way (copying
  // the image directly to the clipboard) work just fine
  QDir tempFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
  QString fileName(CLIPBOARD_TEMP_FILENAME); fileName.append(".png");
  QString path(tempFile.absoluteFilePath(fileName));

  if(!pixmap.save(path)) {
   logUser(LOG_ERROR, "", "Couldn't save the image to the temp location for the clipboard: %s", QSTRING_TO_STRING(path));
   return;
  }

  QProcess process;
  QString appName;
  if(QGuiApplication::platformName() == QString("wayland")) {
   appName = "wl-copy";

   process.setProgram("bash");
   QList<QString> procArgs;
   procArgs << "-c" << QString("wl-copy < " + path);
   process.setArguments(procArgs);
  } else { // X11
   appName = "xclip";
   process.setProgram(appName);

   QList<QString> procArgs;
   procArgs << "-selection" << "clipboard"
            << "-target"    << "image/png"
            << "-i"         << path;
   process.setArguments(procArgs);
  }

  logUser(LOG_TEXT, "", "Trying to save the image to the clipboard with %s...", QSTRING_TO_STRING(appName));
  process.start();
  process.setProcessChannelMode(process.ForwardedChannels);

  // Check for errors.
  if (!process.waitForStarted(5000)) {
   logUser(LOG_ERROR, "", "Couldn't start %s, maybe is not installed...", QSTRING_TO_STRING(appName));
   logUser(LOG_TEXT, "", "  - Error: %s", QSTRING_TO_STRING(process.errorString()));
   logUser(LOG_TEXT, "", "  - Executed command: %s %s", QSTRING_TO_STRING(process.program()), QSTRING_TO_STRING(process.arguments().join(" ")));
   process.kill();
  } else if(!process.waitForFinished(-1))
   logUser(LOG_ERROR, "", "An error occurred with %s: %s", QSTRING_TO_STRING(appName), QSTRING_TO_STRING(process.errorString()));
  else if(process.exitStatus() == QProcess::CrashExit)
   logUser(LOG_ERROR, "", "%s crashed", QSTRING_TO_STRING(appName));
  else if(process.exitCode() != 0)
   logUser(LOG_ERROR, "", "%s failed. Exit code: %d", QSTRING_TO_STRING(appName), process.exitCode());

  // If there's no errors, beep and exit
  else {
   logUser(LOG_SUCCESS, "","Saving image to clipboard with %s was successful!", QSTRING_TO_STRING(appName));
   QApplication::beep();
   return;
  }

  // If there's an error with 'xclip' or 'wl-copy', copy the image path
  // with Qt, not the image itself.
  //
  // Some apps will not fully recognize the image, because
  // there's not a standard way to save a path in linux afaik (in example,
  // Dolphin will copy the image, but Thunar and GIMP will not recognize
  // it).
  if(!clipboard) {
   logUser(LOG_ERROR, "", "There's no clipboard to save the image into");
   return;
  }

  logUser(LOG_TEXT, "", "Saving the image path to the clipboard");
  QMimeData *mimeData = new QMimeData();
  QList<QUrl> urlList; urlList.append(QUrl::fromLocalFile(path));
  mimeData->setUrls(urlList);
  clipboard->setMimeData(mimeData);
  logUser(LOG_SUCCESS, "", "Image path saved to clipboard successfully!");
  QApplication::beep();
#else
  // Copy the image into clipboard (this causes some problems with the
  // clipboard manager in Linux, because when the app exits, the image gets
  // deleted with it. The clipboard only save a pointer to that image)
  if(!clipboard) {
   logUser(LOG_ERROR, "", "There's no clipboard to save the image into");
   return;
  }

  logUser(LOG_TEXT, "", "Saving the image to the clipboard with Qt");
  clipboard->setImage(pixmap.toImage());
  logUser(LOG_SUCCESS, "", "Image saved to clipboard successfully!");
  QApplication::beep();
#endif
}

void ZoomWidget::mouseReleaseEvent(QMouseEvent *event)
{
  // The cursor pos is relative to the resolution of scaled monitor
  const QPoint cursorPos = event->pos();

  // Pre mouse processing
  if(_state == STATE_TRIMMING) {
    _endDrawPoint = screenPointToPixmapPos(cursorPos);
    const QPoint s = _startDrawPoint;
    const QPoint e = _endDrawPoint;

    QRect trimSize = fixQRect(s.x(), s.y(), e.x() - s.x(), e.y() - s.y());
    QPixmap trimmed = _canvas.pixmap.copy(trimSize);
    saveImage(trimmed, (_trimDestination == TRIM_SAVE_TO_IMAGE) ? true : false);

    _state = STATE_MOVING;
    updateCursorShape();
    update();
    return;
  }

  if(_canvas.dragging) {
    _canvas.dragging = false;
    updateCursorShape();
    update();
    return;
  }

  if(_screenOpts == SCREENOPTS_HIDE_ALL)
    return;

  // Mouse processing
  if (_state != STATE_DRAWING)
    return;

  _endDrawPoint = screenPointToPixmapPos(cursorPos);

  UserObjectData data;
  data.pen = _activePen;
  data.startPoint = _startDrawPoint;
  data.endPoint = _endDrawPoint;
  data.highlight = _highlight;
  switch(_drawMode) {
    case DRAWMODE_LINE:      _lines.add(data);      break;
    case DRAWMODE_RECT:      _rects.add(data);      break;
    case DRAWMODE_ARROW:     _arrows.add(data);     break;
    case DRAWMODE_ELLIPSE:   _ellipses.add(data);   break;
    case DRAWMODE_TEXT:
      {
        UserTextData textData;
        textData.data = data;
        textData.text = "";
        textData.caretPos = 0;
        _texts.add(textData);

        if (event->modifiers() == Qt::ShiftModifier)
          _canvas.freezePos = FREEZE_BY_TEXT;

        _state = STATE_TYPING;
        update();
        return;
      }
    case DRAWMODE_FREEFORM:
      {
        // BUG: If the Freeform is empty or the last one is inactive, it is because
        // the user had pressed the mouse but didn't move it, so the free form was
        // not created. This if statement fixes the segfault
        if(_freeForms.isEmpty())
          break;

        // The registration of the points of the FreeForms are in mouseMoveEvent()
        // This only indicates that the drawing is no longer being actively drawn
        UserFreeFormData data = _freeForms.last();
        _freeForms.destroyLast();
        data.active = false;
        data.highlight = _highlight;
        _freeForms.add(data);
        break;
      }
  }

  _state = STATE_MOVING;
  update();
}

void ZoomWidget::updateCursorShape()
{
  QCursor pointHand     = QCursor(Qt::PointingHandCursor);
  QCursor blank         = QCursor(Qt::BlankCursor);
  QCursor waiting       = QCursor(Qt::WaitCursor);
  QCursor denied        = QCursor(Qt::ForbiddenCursor);
  QCursor drag          = QCursor(Qt::ClosedHandCursor);
  QCursor cursorDefault = QCursor(Qt::CrossCursor);

  // Pick color
  QPixmap pickColorPixmap(":/resources/color-picker-16.png");
  if (pickColorPixmap.isNull()) logUser(LOG_ERROR, "", "Failed to load pixmap for the color-picker cursor");
  QCursor pickColor = QCursor(pickColorPixmap, 0, pickColorPixmap.height()-1);

  QPoint cursorPos = GET_CURSOR_POS();

  if(IS_FFMPEG_RUNNING)
    setCursor(waiting);

  else if(isCursorOverButton(cursorPos)) {
    const Button button = _toolBar.buttons.at(buttonBehindCursor(cursorPos));
    if(isActionDisabled(button.action))
      setCursor(denied);
    else
      setCursor(pointHand);

  } else if(_canvas.dragging)
    setCursor(drag);

  else if(_state == STATE_COLOR_PICKER)
    setCursor(pickColor);

  else if(_state == STATE_DELETING)
    setCursor(pointHand);

  else if(isTextEditable(cursorPos))
    setCursor(pointHand);

  else if(_flashlightMode && _state != STATE_TRIMMING)
    setCursor(blank);

  else
    setCursor(cursorDefault);
}

void ZoomWidget::mouseMoveEvent(QMouseEvent *event)
{
  // The cursor pos is relative to the resolution of scaled monitor
  const QPoint cursorPos = event->pos();
  const bool buttonPressed = (event->buttons() != Qt::NoButton);

  // If the app lost focus, request it again
  if(!QWidget::isActiveWindow())
    QWidget::activateWindow();

  updateCursorShape();

  updateAtMousePos(cursorPos);

  if(_canvas.dragging) {
    update();
    return;
  }

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
  if(_state == STATE_DRAWING && _drawMode == DRAWMODE_FREEFORM && buttonPressed) {
    const QPoint cursorInPixmap = screenPointToPixmapPos(cursorPos);

    if( _freeForms.isEmpty() || (!_freeForms.isEmpty() && !_freeForms.last().active) ) {
      UserFreeFormData data;
      data.pen = _activePen;
      data.active = true;
      data.highlight = _highlight;
      data.points.append(cursorInPixmap);
      _freeForms.add(data);

    // It's not empty and the last is active
    } else if(_freeForms.last().points.last() != cursorInPixmap) {
      UserFreeFormData data = _freeForms.last();
      _freeForms.destroyLast();
      data.points.append(cursorInPixmap);
      _freeForms.add(data);
    }
  }

  update();
}

// The mouse pos shouldn't be fixed to the hdpi scaling
void ZoomWidget::updateAtMousePos(QPoint mousePos)
{
  if(_canvas.freezePos == FREEZE_BY_TEXT && _state != STATE_TYPING)
    _canvas.freezePos = FREEZE_FALSE;

  if(!isDisabledMouseTracking())
    shiftPixmap(mousePos);

  if(_canvas.dragging)
    dragPixmap(mousePos - _lastMousePos);

  checkPixmapPos();

  _lastMousePos = mousePos;

  if (_state == STATE_DRAWING || _state == STATE_TRIMMING)
    _endDrawPoint = screenPointToPixmapPos(mousePos);
}

void ZoomWidget::wheelEvent(QWheelEvent *event)
{
  if (_state == STATE_DRAWING || _state == STATE_TYPING)
    return;

  const int sign = (event->angleDelta().y() > 0) ? 1 : -1;
  const bool shiftPressed = (event->modifiers() == Qt::ShiftModifier);

  // Adjust flashlight radius
  if(_flashlightMode && shiftPressed) {
    _flashlightRadius -= sign * SCALE_SENSIVITY * 50;

    if( _flashlightRadius < 20)
      _flashlightRadius=20;
    if( _flashlightRadius > 180)
      _flashlightRadius=180;

    update();
    return;
  }

  if(_liveMode)
    return;

  _canvas.scale += sign * SCALE_SENSIVITY;
  if (_canvas.scale < 1.0f)
    _canvas.scale = 1.0f;

  scalePixmapAt(GET_CURSOR_POS());
  checkPixmapPos();

  update();
}

QString ZoomWidget::getFilePath(FileType type)
{
  int fileIndex = 0;
  QString filePath;

  do {
    // Generate Name
    QString fileName;
    const QString date = QDateTime::currentDateTime().toString(DATE_FORMAT_FOR_FILE);
    fileName = (_fileConfig.name.isEmpty()) ? ("ZoomMe " + date) : _fileConfig.name;
    if(fileIndex != 0)
      fileName.append(FILE_INDEX_DIVIDER + QString::number(fileIndex));

    // Select extension
    fileName.append(".");
    switch(type) {
      case FILE_IMAGE:  fileName.append(_fileConfig.imageExt);  break;
      case FILE_VIDEO:  fileName.append(_fileConfig.videoExt);  break;
      case FILE_ZOOMME: fileName.append(_fileConfig.zoommeExt); break;
    }

    // Path
    filePath = _fileConfig.folder.absoluteFilePath(fileName);

    fileIndex++;
  } while(QFile(filePath).exists());

  return filePath;
}

void ZoomWidget::initFileConfig(QString path, QString name, QString imgExt, QString vidExt)
{
  // Path
  if(path.isEmpty()){
    QString picturesFolder = QStandardPaths::writableLocation(DEFAULT_FOLDER);
    _fileConfig.folder = (picturesFolder.isEmpty()) ? QDir::currentPath() : picturesFolder;
  } else {
    _fileConfig.folder = QDir(path);
    if(!_fileConfig.folder.exists())
      logUser(LOG_ERROR_AND_EXIT, "", "The given path doesn't exits or it's a file");
  }

  // Name
  _fileConfig.name = name;

  // Check if image extension is supported
  QList supportedExtensions = QImageWriter::supportedImageFormats();
  if( (!imgExt.isEmpty()) && (!supportedExtensions.contains(imgExt)) )
    logUser(LOG_ERROR_AND_EXIT, "", "Image extension not supported");

  // Extension
  const char* defaultImgExt = "png";
  _fileConfig.imageExt = (imgExt.isEmpty()) ? defaultImgExt : imgExt;
  const char* defaultVidExt = "mp4";
  _fileConfig.videoExt = (vidExt.isEmpty()) ? defaultVidExt : vidExt;
  const char* defaultZoommeExt = "zoomme";
  _fileConfig.zoommeExt = defaultZoommeExt;
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
    minimumSize *= _canvas.scale;

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
  // _tests.append(data);
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
  // _tests.clear();
  /////////////////////////
  int x, y, w, h;
  switch(_drawMode) {
    case DRAWMODE_LINE:
      for (int i = 0; i < _lines.size(); ++i) {
        getRealUserObjectPos(_lines.at(i), &x, &y, &w, &h, true);
        if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
          return i;
      }
      break;
    case DRAWMODE_RECT:
      for (int i = 0; i < _rects.size(); ++i) {
        getRealUserObjectPos(_rects.at(i), &x, &y, &w, &h, true);
        if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
          return i;
      }
      break;
    case DRAWMODE_ARROW:
      for (int i = 0; i < _arrows.size(); ++i) {
        getRealUserObjectPos(_arrows.at(i), &x, &y, &w, &h, true);
        if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
          return i;
      }
      break;
    case DRAWMODE_ELLIPSE:
      for (int i = 0; i < _ellipses.size(); ++i) {
        getRealUserObjectPos(_ellipses.at(i), &x, &y, &w, &h, true);
        if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
          return i;
      }
      break;
    case DRAWMODE_TEXT:
      for (int i = 0; i < _texts.size(); ++i) {
        getRealUserObjectPos(_texts.at(i).data, &x, &y, &w, &h, true);
        if(isCursorInsideHitBox(x, y, w, h, cursorPos, false))
          return i;
      }
      break;
    case DRAWMODE_FREEFORM:
      for (int i = 0; i < _freeForms.size(); ++i) {
        for(int z = 0; z < _freeForms.at(i).points.size()-1; ++z) {
          QPoint current = _freeForms.at(i).points.at(z);
          QPoint next    = _freeForms.at(i).points.at(z+1);

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
  const int key = event->key();
  const bool shiftPressed = (event->modifiers() == Qt::ShiftModifier);
  const bool controlPressed = (event->key() == Qt::Key_Control);

  if(key == Qt::Key_Shift && _state != STATE_TYPING)
    _canvas.freezePos = FREEZE_BY_SHIFT;

  if(controlPressed)
    _toolBar.show = true;

  if(_state == STATE_TYPING) {
    // If it's pressed Enter (without Shift) or Escape
    if ((!shiftPressed && key == Qt::Key_Return) || key == Qt::Key_Escape) {
      if( _texts.last().text.isEmpty() )
        _texts.destroyLast();
      _state = STATE_MOVING;
      update();
      return;
    }

    UserTextData textData = _texts.last();
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
    _texts.destroyLast();
    _texts.add(textData);
    update();
    return;
  }

  ZoomWidgetAction action = ACTION_SPACER;
  switch(key) {
    case Qt::Key_G: action = ACTION_COLOR_GREEN;   break;
    case Qt::Key_B: action = ACTION_COLOR_BLUE;    break;
    case Qt::Key_C: action = ACTION_COLOR_CYAN;    break;
    case Qt::Key_O: action = ACTION_COLOR_ORANGE;  break;
    case Qt::Key_M: action = ACTION_COLOR_MAGENTA; break;
    case Qt::Key_Y: action = ACTION_COLOR_YELLOW;  break;
    case Qt::Key_W: action = ACTION_COLOR_WHITE;   break;
    case Qt::Key_D: action = ACTION_COLOR_BLACK;   break;
    case Qt::Key_R:
                    if(shiftPressed)
                      action = ACTION_REDO;
                    else
                      action = ACTION_COLOR_RED;
                    break;

    case Qt::Key_Z: action = ACTION_LINE;          break;
    case Qt::Key_X: action = ACTION_RECTANGLE;     break;
    case Qt::Key_A: action = ACTION_ARROW;         break;
    case Qt::Key_T: action = ACTION_TEXT;          break;
    case Qt::Key_F: action = ACTION_FREEFORM;      break;
    case Qt::Key_H: action = ACTION_HIGHLIGHT;     break;

    case Qt::Key_E:
                    if(shiftPressed)
                      action = ACTION_SAVE_PROJECT;
                    else
                      action = ACTION_ELLIPSE;
                    break;

    case Qt::Key_S:
                    if(shiftPressed)
                      action = ACTION_SAVE_TO_CLIPBOARD;
                    else
                      action = ACTION_SAVE_TO_FILE;
                    break;

    case Qt::Key_Escape: action = ACTION_ESCAPE;         break;
    case Qt::Key_U:      action = ACTION_UNDO;           break;
    case Qt::Key_Q:      action = ACTION_CLEAR;          break;
    case Qt::Key_Tab:    action = ACTION_BLACKBOARD;     break;
    case Qt::Key_Space:  action = ACTION_SCREEN_OPTS;    break;
    case Qt::Key_Period: action = ACTION_FLASHLIGHT;     break;
    case Qt::Key_Comma:  action = ACTION_DELETE;         break;
    case Qt::Key_Minus:  action = ACTION_RECORDING;      break;
    case Qt::Key_P:      action = ACTION_PICK_COLOR;     break;

    case Qt::Key_1: action = ACTION_WIDTH_1;             break;
    case Qt::Key_2: action = ACTION_WIDTH_2;             break;
    case Qt::Key_3: action = ACTION_WIDTH_3;             break;
    case Qt::Key_4: action = ACTION_WIDTH_4;             break;
    case Qt::Key_5: action = ACTION_WIDTH_5;             break;
    case Qt::Key_6: action = ACTION_WIDTH_6;             break;
    case Qt::Key_7: action = ACTION_WIDTH_7;             break;
    case Qt::Key_8: action = ACTION_WIDTH_8;             break;
    case Qt::Key_9: action = ACTION_WIDTH_9;             break;

    default:
        updateCursorShape();
        update();
        return;
  }

  toggleAction(action);
}

void ZoomWidget::keyReleaseEvent(QKeyEvent *event)
{
  const bool shiftReleased = (event->key() == Qt::Key_Shift);
  const bool controlReleased = (event->key() == Qt::Key_Control);

  if(shiftReleased && _canvas.freezePos == FREEZE_BY_SHIFT) {
    _canvas.freezePos = FREEZE_FALSE;
    updateAtMousePos(GET_CURSOR_POS());
  }

  if(controlReleased)
    _toolBar.show = false;

  updateCursorShape();
  update();
}

void ZoomWidget::setLiveMode(bool liveMode)
{
  _liveMode = liveMode;
}

void ZoomWidget::grabFromClipboard(FitImage config)
{
  QImage image;

  if(!clipboard)
    logUser(LOG_ERROR_AND_EXIT, "", "The clipboard is uninitialized");

  image = clipboard->image();
  if(image.isNull())
    logUser(LOG_ERROR_AND_EXIT, "", "The clipboard doesn't contain an image or its format is not supported");

  grabImage(QPixmap::fromImage(image), config);
}

void ZoomWidget::createBlackboard(QSize size)
{
  if(size.width() < _screenSize.width()) {
    logUser(LOG_INFO, "", "The given width is less than the screen's width, so the width is now the screen's width");
    size.setWidth(_screenSize.width());
  }

  if(size.height() < _screenSize.height()) {
    logUser(LOG_INFO, "", "The given height is less than the screen's height, so the height is now the screen's height");
    size.setHeight(_screenSize.height());
  }

  _sourcePixmap = QPixmap(size);
  _sourcePixmap.fill(QCOLOR_BLACKBOARD);
  _canvas.size = size;
  _canvas.originalSize = size;

  showFullScreen();
}

void ZoomWidget::grabDesktop()
{
  QPixmap desktop = _desktopScreen->grabWindow(0);

  // The desktop pixmap is null if it couldn't capture the screenshot. For
  // example, in Wayland, it will be null because Wayland doesn't support screen
  // grabbing
  if(desktop.isNull()){
    if(_liveMode){
      _sourcePixmap = QPixmap(_screenSize);
      _sourcePixmap.fill(Qt::transparent);
      return;
    }

    if(QGuiApplication::platformName() == QString("wayland"))
      logUser(LOG_ERROR_AND_EXIT, "", "Couldn't grab the desktop. It seems you're using Wayland: try to use the '-l' flag (live mode)");
    else
      logUser(LOG_ERROR_AND_EXIT, "", "Couldn't grab the desktop");
  }

  // Paint the desktop over _sourcePixmap
  // Fixes the issue with hdpi scaling (now the size of the image is the real
  // resolution of the screen)
  _sourcePixmap = QPixmap(desktop.size());
  QPainter painter(&_sourcePixmap);
  painter.drawPixmap(0, 0, desktop.width(), desktop.height(), desktop);
  painter.end();

  if(!_liveMode) showFullScreen();
}

void ZoomWidget::grabImage(QPixmap img, FitImage config)
{
  if(img.isNull())
    logUser(LOG_ERROR_AND_EXIT, "", "Couldn't open the image");

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
  _sourcePixmap = QPixmap(width, height);
  _sourcePixmap.fill(QCOLOR_BLACKBOARD);
  QPainter painter(&_sourcePixmap);
  painter.drawPixmap(x, y, img);
  painter.end();

  _canvas.size = _sourcePixmap.size();
  _canvas.originalSize = _canvas.size;

  if(!_liveMode) showFullScreen();
}

void ZoomWidget::dragPixmap(QPoint delta)
{
  _canvas.pos += delta;
}

void ZoomWidget::shiftPixmap(const QPoint cursorPos)
{
  // This is the maximum value for shifting the pixmap
  const QSize availableMargin = -1 * (_canvas.size - _screenSize);

  // The percentage of the cursor position relative to the screen size
  const float percentageX = (float)cursorPos.x() / (float)_screenSize.width();
  const float percentageY = (float)cursorPos.y() / (float)_screenSize.height();

  _canvas.pos.setX(availableMargin.width() * percentageX);
  _canvas.pos.setY(availableMargin.height() * percentageY);
}

void ZoomWidget::scalePixmapAt(const QPointF pos)
{
  int old_w = _canvas.size.width();
  int old_h = _canvas.size.height();

  int new_w = _canvas.originalSize.width() * _canvas.scale;
  int new_h = _canvas.originalSize.height() * _canvas.scale;
  _canvas.size = QSize(new_w, new_h);

  int dw = new_w - old_w;
  int dh = new_h - old_h;

  int cur_x = pos.x() + abs(_canvas.pos.x());
  int cur_y = pos.y() + abs(_canvas.pos.y());

  float cur_px = -((float)cur_x / old_w);
  float cur_py = -((float)cur_y / old_h);

  _canvas.pos.setX(_canvas.pos.x() + dw*cur_px);
  _canvas.pos.setY(_canvas.pos.y() + dh*cur_py);
}

void ZoomWidget::checkPixmapPos()
{
  if (_canvas.pos.x() > 0) {
    _canvas.pos.setX(0);
  } else if ((_canvas.size.width() + _canvas.pos.x()) < width()) {
    _canvas.pos.setX(width() - _canvas.size.width());
  }

  if (_canvas.pos.y() > 0) {
    _canvas.pos.setY(0);
  } else if ((_canvas.size.height() + _canvas.pos.y()) < height()) {
    _canvas.pos.setY(height() - _canvas.size.height());
  }
}

// TODO FORMAT CURLY BRACKET
QPoint ZoomWidget::screenPointToPixmapPos(QPoint qpoint) {
  QPoint returnPoint = (qpoint - _canvas.pos)/_canvas.scale;

  returnPoint.setX( FIX_X_FOR_HDPI_SCALING(returnPoint.x()) );
  returnPoint.setY( FIX_Y_FOR_HDPI_SCALING(returnPoint.y()) );

  return returnPoint;
}

QPoint ZoomWidget::pixmapPointToScreenPos(QPoint qpoint) {
  qpoint.setX( GET_X_FROM_HDPI_SCALING(qpoint.x()) );
  qpoint.setY( GET_Y_FROM_HDPI_SCALING(qpoint.y()) );

  QPoint point = _canvas.pos + qpoint * _canvas.scale;

  return point;
}

QSize ZoomWidget::pixmapSizeToScreenSize(QSize qsize){
  qsize.setWidth(  GET_X_FROM_HDPI_SCALING(qsize.width() ) );
  qsize.setHeight( GET_Y_FROM_HDPI_SCALING(qsize.height()) );

  return qsize * _canvas.scale;
}

void ZoomWidget::drawDrawnPixmap(QPainter *painter)
{
  const int x = _canvas.pos.x();
  const int y = _canvas.pos.y();
  const int w = _canvas.size.width();
  const int h = _canvas.size.height();

  painter->drawPixmap(x, y, w, h, _canvas.pixmap);
}

bool ZoomWidget::isDisabledMouseTracking()
{
  if(DISABLE_MOUSE_TRACKING)
    return true;

  return (_canvas.freezePos == FREEZE_BY_TEXT)  ||
         (_canvas.freezePos == FREEZE_BY_SHIFT) ||
         (isToolBarVisible())                   ||
         (_canvas.dragging);
}

// The cursor pos shouldn't be fixed to HDPI scaling
bool ZoomWidget::isTextEditable(QPoint cursorPos)
{
  const bool isInEditTextMode = (_state == STATE_MOVING)     &&
                                (_drawMode == DRAWMODE_TEXT) &&
                                (_screenOpts != SCREENOPTS_HIDE_ALL);

  return (isInEditTextMode) &&
         (cursorOverForm(cursorPos) != -1);
}

// Function taken from https://github.com/tsoding/musializer/blob/master/src/nob.h
// inside the nob_log function
void ZoomWidget::logUser(Log_Urgency type, QString popupMsg, const char *fmt, ...)
{
  FILE *output;
  bool exitApp = false;
  char msg[1000] = {0};

  switch (type) {
    case LOG_TEXT:
    case LOG_SUCCESS:
    case LOG_INFO:
      output = stdout;
      fprintf(output, "[INFO] ");
      break;

    case LOG_ERROR_AND_EXIT:
      exitApp = true;
    case LOG_ERROR:
      output = stderr;
      fprintf(output, "[ERROR] ");
      break;
  }

  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 1000, fmt, args);
  va_end(args);

  fprintf(output, "%s\n", msg);

  if(exitApp)
    exit(EXIT_FAILURE);

  if(type == LOG_TEXT)
    return;

  // Popup
  if(popupMsg.isEmpty()) popupMsg=QString(msg);
  int lifetime = 0;
  switch (type) {
    case LOG_SUCCESS: lifetime = POPUP_SUCCESS_MSEC; break;
    case LOG_INFO:    lifetime = POPUP_INFO_MSEC;    break;
    case LOG_ERROR:   lifetime = POPUP_ERROR_MSEC;   break;

    case LOG_TEXT:
    case LOG_ERROR_AND_EXIT:
      logUser(LOG_ERROR_AND_EXIT, "", "An error happened. You shouldn't add a pop-up with LOG_ERROR_AND_EXIT or LOG_TEXT");
      break;
  }

  const qint64 time = QDateTime::currentMSecsSinceEpoch();
  _popupTray.popups.append(Popup{ time, lifetime, popupMsg, type });
  if(!_popupTray.updateTimer->isActive())
    _popupTray.updateTimer->start(POPUP_UPDATE_MSEC);
  update();
}

void ZoomWidget::updateForPopups()
{
  if(_popupTray.popups.size() == 0)
    _popupTray.updateTimer->stop();

  const qint64 time = QDateTime::currentMSecsSinceEpoch();

  // Remove old pop-ups
  for(int i=_popupTray.popups.size()-1; i>=0; i--) {
    Popup p = _popupTray.popups.at(i);

    if((time-p.timeCreated) >= p.lifetime)
      _popupTray.popups.removeAt(i);
  }

  update();
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
