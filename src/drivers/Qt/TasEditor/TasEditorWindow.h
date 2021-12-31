// TasEditorWindow.h
//

#pragma once

#include <time.h>
#include <string>
#include <list>
#include <map>

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QTreeView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCloseEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QFont>
#include <QPainter>
#include <QShortcut>
#include <QTabWidget>
#include <QStackedWidget>
#include <QClipboard>

#include "Qt/config.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/TasEditor/taseditor_config.h"
#include "Qt/TasEditor/taseditor_project.h"
#include "Qt/TasEditor/greenzone.h"
#include "Qt/TasEditor/selection.h"
#include "Qt/TasEditor/markers_manager.h"
#include "Qt/TasEditor/snapshot.h"
#include "Qt/TasEditor/bookmarks.h"
#include "Qt/TasEditor/branches.h"
#include "Qt/TasEditor/history.h"
#include "Qt/TasEditor/playback.h"
#include "Qt/TasEditor/recorder.h"
#include "Qt/TasEditor/taseditor_lua.h"
#include "Qt/TasEditor/splicer.h"
//#include "Qt/TasEditor/editor.h"
//#include "Qt/TasEditor/popup_display.h"


class TasEditorWindow;

// Piano Roll Definitions
enum PIANO_ROLL_COLUMNS
{
	COLUMN_ICONS,
	COLUMN_FRAMENUM,
	COLUMN_JOYPAD1_A,
	COLUMN_JOYPAD1_B,
	COLUMN_JOYPAD1_S,
	COLUMN_JOYPAD1_T,
	COLUMN_JOYPAD1_U,
	COLUMN_JOYPAD1_D,
	COLUMN_JOYPAD1_L,
	COLUMN_JOYPAD1_R,
	COLUMN_JOYPAD2_A,
	COLUMN_JOYPAD2_B,
	COLUMN_JOYPAD2_S,
	COLUMN_JOYPAD2_T,
	COLUMN_JOYPAD2_U,
	COLUMN_JOYPAD2_D,
	COLUMN_JOYPAD2_L,
	COLUMN_JOYPAD2_R,
	COLUMN_JOYPAD3_A,
	COLUMN_JOYPAD3_B,
	COLUMN_JOYPAD3_S,
	COLUMN_JOYPAD3_T,
	COLUMN_JOYPAD3_U,
	COLUMN_JOYPAD3_D,
	COLUMN_JOYPAD3_L,
	COLUMN_JOYPAD3_R,
	COLUMN_JOYPAD4_A,
	COLUMN_JOYPAD4_B,
	COLUMN_JOYPAD4_S,
	COLUMN_JOYPAD4_T,
	COLUMN_JOYPAD4_U,
	COLUMN_JOYPAD4_D,
	COLUMN_JOYPAD4_L,
	COLUMN_JOYPAD4_R,
	COLUMN_FRAMENUM2,

	TOTAL_COLUMNS
};

#define HEADER_LIGHT_MAX 10
#define HEADER_LIGHT_HOLD 5
#define HEADER_LIGHT_MOUSEOVER_SEL 3
#define HEADER_LIGHT_MOUSEOVER 0
#define HEADER_LIGHT_UPDATE_TICK  (CLOCKS_PER_SEC / 25)	// 25FPS

struct NewProjectParameters
{
	int inputType;
	bool copyCurrentInput;
	bool copyCurrentMarkers;
	std::wstring authorName;
};

class bookmarkPreviewPopup : public QDialog
{
   Q_OBJECT
	public:
	   bookmarkPreviewPopup( int index, QWidget *parent = nullptr );
	   ~bookmarkPreviewPopup(void);

	   int reloadImage(int index);

	   static int currentIndex(void);

	   static bookmarkPreviewPopup *currentInstance(void);

	private:
		int loadImage(int index);

		int alpha;
		int imageIndex;
		bool actv;
		unsigned char *screenShotRaster;
		QTimer *timer;
		QLabel *imgLbl, *descLbl;

		static bookmarkPreviewPopup *instance;

	public slots:
		void periodicUpdate(void);
		void imageIndexChanged(int);
};

class markerDragPopup : public QDialog
{
	Q_OBJECT

	public:
		markerDragPopup( QWidget *parent = nullptr );
		~markerDragPopup( void );

		void setInitialPosition( QPoint p );
		void setRowIndex( int row );
		void setBgColor( QColor c );
		void throwAway(void);
		void dropAccept(void);
		void dropAbort(void);
	protected:
		bool eventFilter(QObject *obj, QEvent *event) override;
		void paintEvent(QPaintEvent *event) override;

		int alpha;
		int rowIndex;
		int liveCount;
		QColor bgColor;
		QPoint initialPos;
		QTimer *timer;

		bool released;
		bool dropAccepted;
		bool dropAborted;
		bool thrownAway;

	private slots:
		void fadeAway(void);
};

class QPianoRoll : public QWidget
{
	Q_OBJECT

	public:
		QPianoRoll(QWidget *parent = 0);
		~QPianoRoll(void);

		void reset(void);
		void save(EMUFILE *os, bool really_save);
		bool load(EMUFILE *is, unsigned int offset);
		void setScrollBars( QScrollBar *h, QScrollBar *v );

		QFont getFont(void){ return font; };

		int   getDragMode(void){ return dragMode; };

		bool  lineIsVisible( int lineNum );
		bool  checkIfTheresAnIconAtFrame(int frame);

		void  updateLinesCount(void);
		void  handleColumnSet(int column, bool altPressed);
		void  centerListAroundLine(int rowIndex);
		void  ensureTheLineIsVisible( int lineNum );
		void  followPlaybackCursorIfNeeded(bool followPauseframe);
		void  followMarker(int markerID);
		void  followSelection(void);
		void  followPlaybackCursor(void);
		void  followPauseframe(void);
		void  followUndoHint(void);
		void  setLightInHeaderColumn(int column, int level);
		void  periodicUpdate(void);

		void  setFont( QFont &font );

		QColor      gridColor;
	protected:
		void calcFontData(void);
		void resizeEvent(QResizeEvent *event) override;
		void paintEvent(QPaintEvent *event) override;
		void mousePressEvent(QMouseEvent * event) override;
		void mouseReleaseEvent(QMouseEvent * event) override;
		void mouseMoveEvent(QMouseEvent * event) override;
		void mouseDoubleClickEvent(QMouseEvent * event) override;
		void wheelEvent(QWheelEvent *event) override;
		void keyPressEvent(QKeyEvent *event) override;
		void keyReleaseEvent(QKeyEvent *event) override;
		void focusInEvent(QFocusEvent *event) override;
		void focusOutEvent(QFocusEvent *event) override;
		void contextMenuEvent(QContextMenuEvent *event) override;
		void dragEnterEvent(QDragEnterEvent *event) override;
		void dropEvent(QDropEvent *event) override;

		void crossGaps(int zDelta);
		void startDraggingPlaybackCursor(void);
		void startDraggingMarker(int mouseX, int mouseY, int rowIndex, int columnIndex);
		void startSelectingDrag(int start_frame);
		void startDeselectingDrag(int start_frame);
		void handlePlaybackCursorDragging(void);
		void finishDrag(void);
		void updateDrag(void);

		void drawArrow( QPainter *painter, int xl, int yl, int value );

		int    calcColumn( int px );
		QPoint convPixToCursor( QPoint p );

	private:
		TasEditorWindow *parent;
		QFont       font;
		QScrollBar *hbar;
		QScrollBar *vbar;
		QColor      windowColor;
		QColor      headerLightsColors[11];
		QColor      hotChangesColors[16];

		markerDragPopup *mkrDrag;

		int8_t headerColors[TOTAL_COLUMNS];

		int numCtlr;
		int numColumns;
		int pxCharWidth;
		int pxCharHeight;
		int pxCursorHeight;
		int pxLineXScroll;
		int pxLineWidth;
		int pxLineSpacing;
		int pxLineLead;
		int pxLineTextOfs;
		int pxWidthCol1;
		int pxWidthFrameCol;
		int pxWidthCtlCol;
		int pxWidthBtnCol;
		int pxFrameColX;
		int pxFrameCtlX[4];
		int viewLines;
		int viewWidth;
		int viewHeight;
		int lineOffset;
		int maxLineOffset;
		int numInputDevs;
		int dragMode;
		int dragSelectionStartingFrame;
		int dragSelectionEndingFrame;
		int realRowUnderMouse;
		int rowUnderMouse;
		int columnUnderMouse;
		int rowUnderMouseAtPress;
		int columnUnderMouseAtPress;
		int markerDragFrameNumber;
		int markerDragCountdown;
		int wheelPixelCounter;
		int wheelAngleCounter;
		int headerItemUnderMouse;
		int scroll_x;
		int scroll_y;
		int mouse_x;
		int mouse_y;
		int gridPixelWidth;
		clock_t drawingStartTimestamp;
		clock_t nextHeaderUpdateTime;

		int playbackCursorPos;

		bool useDarkTheme;
		bool rightButtonDragMode;

	public slots:
		void hbarChanged(int val);
		void vbarChanged(int val);
		void vbarActionTriggered(int act);
		void setupMarkerDrag(void);
};

class  PianoRollScrollBar : public QScrollBar
{
	Q_OBJECT

	public:
		PianoRollScrollBar( QWidget *parent );
		~PianoRollScrollBar(void);

	protected:
		void wheelEvent(QWheelEvent *event) override;

		int wheelPixelCounter;
		int wheelAngleCounter;
		int pxLineSpacing;
};

class  TasRecentProjectAction : public QAction
{
	Q_OBJECT

	public:
		TasRecentProjectAction( QString title, QWidget *parent = 0);
		~TasRecentProjectAction(void);

		std::string  path;

	public slots:
		void activateCB(void);

};

class TasFindNoteWindow : public QDialog
{
	Q_OBJECT

	public:
		TasFindNoteWindow(QWidget *parent = 0);
		~TasFindNoteWindow(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QLineEdit     *searchPattern;
		QCheckBox     *matchCase;
		QRadioButton  *up;
		QRadioButton  *down;
		QPushButton   *nextBtn;
		QPushButton   *closeBtn;

	public slots:
		void closeWindow(void);

	private slots:
		void matchCaseChanged(bool);
		void upDirectionSelected(void);
		void downDirectionSelected(void);
		void findNextClicked(void);
		void searchPatternChanged(const QString &);
};

class TasEditorWindow : public QDialog
{
	Q_OBJECT

	public:
		TasEditorWindow(QWidget *parent = 0);
		~TasEditorWindow(void);

		QPianoRoll  *pianoRoll;

		TASEDITOR_PROJECT project;
		TASEDITOR_CONFIG taseditorConfig;
		TASEDITOR_LUA taseditor_lua;
		MARKERS_MANAGER markersManager;
		BOOKMARKS bookmarks;
		//PIANO_ROLL pianoRoll;
		SPLICER splicer;
		//EDITOR editor;
		GREENZONE greenzone;
		SELECTION selection;
		PLAYBACK  playback;
		RECORDER  recorder;
		HISTORY   history;
		BRANCHES  branches;

		void initHotKeys(void);
		void updateCaption(void);
		bool loadProject(const char* fullname);
		void importMovieFile( const char *path );
		void loadClipboard(const char *txt);
		void toggleInput(int start, int end, int joy, int button, int consecutivenessTag);
		void setInputUsingPattern(int start, int end, int joy, int button, int consecutivenessTag);
		bool handleColumnSet(void);
		bool handleColumnSetUsingPattern(void);
		bool handleInputColumnSet(int joy, int button);
		bool handleInputColumnSetUsingPattern(int joy, int button);
		bool updateHistoryItems(void);

		QPoint getPreviewPopupCoordinates(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QMenuBar  *buildMenuBar(void);
		void buildPianoRollDisplay(void);
		void buildSideControlPanel(void);
		void initPatterns(void);

		QMenu     *recentProjectMenu;
		QAction   *followUndoAct;
		QAction   *followMkrAct;
		QAction   *enaHotChgAct;
		QAction   *dpyBrnchDescAct;
		QAction   *dpyBrnchScrnAct;
		QAction   *enaGrnznAct;
		QAction   *afPtrnSkipLagAct;
		QAction   *adjInputLagAct;
		QAction   *drawInputDragAct;
		QAction   *cmbRecDrawAct;
		QAction   *use1PforRecAct;
		QAction   *useInputColSetAct;
		QAction   *bindMkrInputAct;
		QAction   *emptyNewMkrNotesAct;
		QAction   *oldCtlBrnhSchemeAct;
		QAction   *brnchRestoreMovieAct;
		QAction   *hudInScrnBranchAct;
		QAction   *pauseAtEndAct;
		QAction   *showToolTipsAct;
		QAction   *autoLuaAct;

		QSplitter  *mainHBox;
		QFrame     *pianoRollFrame;
		QWidget    *pianoRollContainerWidget;
		QWidget    *controlPanelContainerWidget;
		QScrollBar *pianoRollHBar;
		QScrollBar *pianoRollVBar;
		QPushButton     *upperMarkerLabel;
		QPushButton     *lowerMarkerLabel;
		UpperMarkerNoteEdit  *upperMarkerNote;
		LowerMarkerNoteEdit  *lowerMarkerNote;
		QTabWidget *bkmkBrnchStack;

		QVBoxLayout *ctlPanelMainVbox;
		QGroupBox  *playbackGBox;
		QGroupBox  *recorderGBox;
		QGroupBox  *splicerGBox;
		//QGroupBox  *luaGBox;
		//QGroupBox  *historyGBox;
		QFrame     *bbFrame;

		QPushButton *rewindMkrBtn;
		QPushButton *rewindFrmBtn;
		QPushButton *playPauseBtn;
		QPushButton *advFrmBtn;
		QPushButton *advMkrBtn;
		QCheckBox   *followCursorCbox;
		QCheckBox   *turboSeekCbox;
		QCheckBox   *autoRestoreCbox;

		QCheckBox    *recRecordingCbox;
		QCheckBox    *recSuperImposeCbox;
		QCheckBox    *recUsePatternCbox;
		QRadioButton *recAllBtn;
		QRadioButton *rec1PBtn;
		QRadioButton *rec2PBtn;
		QRadioButton *rec3PBtn;
		QRadioButton *rec4PBtn;

		QLabel      *selectionLbl;
		QLabel      *clipboardLbl;

		//QPushButton *runLuaBtn;
		//QCheckBox   *autoLuaCBox;

		QTreeWidget *histTree;

		QPushButton *prevMkrBtn;
		QPushButton *nextMkrBtn;
		QPushButton *similarBtn;
		QPushButton *moreBtn;

		QClipboard *clipboard;

		QShortcut  *hotkeyShortcut[HK_MAX];

		std::vector<std::string> patternsNames;
		std::vector<std::vector<uint8_t>> patterns;
		std::list <std::string*> projList;

		bool mustCallManualLuaFunction;
		bool recentProjectMenuReset;
	private:

		int initModules(void);
		bool saveProject(bool save_compact = false);
		bool saveProjectAs(bool save_compact = false);
		bool askToSaveProject(void);
		void updateToolTips(void);

		void clearProjectList(void);
		void buildRecentProjectMenu(void);
		void saveRecentProjectMenu(void);
		void addRecentProject(const char *prog);


	public slots:
		void closeWindow(void);
		void frameUpdate(void);
		void updateCheckedItems(void);
		void updateRecordStatus(void);
	private slots:
		void openProject(void);
		void saveProjectCb(void);
		void saveProjectAsCb(void);
		void saveProjectCompactCb(void);
		void createNewProject(void);
		void importMovieFile(void);
		void exportMovieFile(void);
		void openOnlineDocs(void);
		void recordingChanged(int);
		void recordInputChanged(int);
		void superImposedChanged(int);
		void usePatternChanged(int);
		void playbackPauseCB(void);
		void playbackFrameRewind(void);
		void playbackFrameForward(void);
		void playbackFrameRewindFull(void);
		void playbackFrameForwardFull(void);
		void scrollSelectionUpOne(void);
		void scrollSelectionDnOne(void);
		void editUndoCB(void);
		void editRedoCB(void);
		void editUndoSelCB(void);
		void editRedoSelCB(void);
		void editDeselectAll(void);
		void editSelectAll(void);
		void editSelBtwMkrs(void);
		void editReselectClipboard(void);
		void editCutCB(void);
		void editCopyCB(void);
		void editPasteCB(void);
		void editPasteInsertCB(void);
		void editClearCB(void);
		void editDeleteCB(void);
		void editCloneCB(void);
		void editInsertCB(void);
		void editInsertNumFramesCB(void);
		void editTruncateMovieCB(void);
		void openFindNoteWindow(void);
		void dpyBrnchScrnChanged(bool);
		void dpyBrnchDescChanged(bool);
		void enaHotChgChanged(bool);
		void followUndoActChanged(bool);
		void followMkrActChanged(bool);
		void enaGrnznActChanged(bool);
		void afPtrnSkipLagActChanged(bool);
		void adjInputLagActChanged(bool);
		void drawInputDragActChanged(bool);
		void cmbRecDrawActChanged(bool);
		void use1PforRecActChanged(bool);
		void useInputColSetActChanged(bool);
		void bindMkrInputActChanged(bool);
		void emptyNewMkrNotesActChanged(bool);
		void oldCtlBrnhSchemeActChanged(bool);
		void brnchRestoreMovieActChanged(bool);
		void hudInScrnBranchActChanged(bool);
		void pauseAtEndActChanged(bool);
		void showToolTipsActChanged(bool);
		void upperMarkerLabelClicked(void);
		void lowerMarkerLabelClicked(void);
		void histTreeItemActivated(QTreeWidgetItem*,int);
		void playbackFollowCursorCb(bool);
		void playbackAutoRestoreCb(bool);
		void playbackTurboSeekCb(bool);
		void openProjectSaveOptions(void);
		void setGreenzoneCapacity(void);
		void setMaxUndoCapacity(void);
		void setCurrentPattern(int);
		void tabViewChanged(int);
		void findSimilarNote(void);
		void findNextSimilarNote(void);
		void jumpToPreviousMarker(void);
		void jumpToNextMarker(void);
		void openAboutWindow(void);
		void autoLuaRunChanged(bool);
		void manLuaRun(void);
		void setMarkers(void);
		void removeMarkers(void);
		void ungreenzoneSelectedFrames(void);
		void activateHotkey( int hkIdx, QShortcut *shortcut );
		void changePianoRollFontCB(void);
		void changeBookmarksFontCB(void);
		void changeBranchesFontCB(void);

	friend class RECORDER;
	friend class SPLICER;
	friend class SELECTION;
	friend class PLAYBACK;
	friend class HISTORY;
	friend class MARKERS_MANAGER;
	friend class TASEDITOR_PROJECT;
	friend class QPianoRoll;
};

extern TASEDITOR_PROJECT *project;
extern TASEDITOR_CONFIG *taseditorConfig;
extern TASEDITOR_LUA *taseditor_lua;
extern MARKERS_MANAGER *markersManager;
extern BOOKMARKS *bookmarks;
extern GREENZONE *greenzone;
extern PLAYBACK *playback;
extern RECORDER *recorder;
extern SPLICER *splicer;
extern HISTORY *history;
extern SELECTION *selection;
extern BRANCHES *branches;

bool tasWindowIsOpen(void);

void tasWindowSetFocus(bool val);

bool isTaseditorRecording(void);
void recordInputByTaseditor(void);

extern TasEditorWindow *tasWin;
