// TasEditorWindow.h
//

#pragma once

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

struct NewProjectParameters
{
	int inputType;
	bool copyCurrentInput;
	bool copyCurrentMarkers;
	std::wstring authorName;
};

class QPianoRoll : public QWidget
{
	Q_OBJECT

	public:
		QPianoRoll(QWidget *parent = 0);
		~QPianoRoll(void);

		void setScrollBars( QScrollBar *h, QScrollBar *v );

		QFont getFont(void){ return font; };

		int   getDragMode(void){ return dragMode; };

		bool  lineIsVisible( int lineNum );

		void  handleColumnSet(int column, bool altPressed);
		void  centerListAroundLine(int rowIndex);
		void  ensureTheLineIsVisible( int lineNum );
		void  followMarker(int markerID);
		void  followSelection(void);
		void  followPlaybackCursor(void);
		void  followPauseframe(void);
		void  followUndoHint(void);

	protected:
		void calcFontData(void);
		void resizeEvent(QResizeEvent *event);
		void paintEvent(QPaintEvent *event);
		void mousePressEvent(QMouseEvent * event);
		void mouseReleaseEvent(QMouseEvent * event);
		void mouseMoveEvent(QMouseEvent * event);
		void mouseDoubleClickEvent(QMouseEvent * event);
		void keyPressEvent(QKeyEvent *event);
		void keyReleaseEvent(QKeyEvent *event);
		void focusInEvent(QFocusEvent *event);
		void focusOutEvent(QFocusEvent *event);

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

		int numCtlr;
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
		int markerDragFrameNumber;
		int markerDragCountdown;
		int drawingStartTimestamp;
		int mouse_x;
		int mouse_y;

		bool useDarkTheme;

	public slots:
		void hbarChanged(int val);
		void vbarChanged(int val);
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

		void loadClipboard(const char *txt);
		void toggleInput(int start, int end, int joy, int button, int consecutivenessTag);
		void setInputUsingPattern(int start, int end, int joy, int button, int consecutivenessTag);
		bool handleColumnSet(void);
		bool handleColumnSetUsingPattern(void);
		bool handleInputColumnSet(int joy, int button);
		bool handleInputColumnSetUsingPattern(int joy, int button);
		void setMarkers(void);
		void removeMarkers(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QMenuBar  *buildMenuBar(void);
		void buildPianoRollDisplay(void);
		void buildSideControlPanel(void);
		void initPatterns(void);

		QMenu     *recentMenu;
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
		QGroupBox  *historyGBox;
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

		std::vector<std::string> patternsNames;
		std::vector<std::vector<uint8_t>> patterns;
	private:

		int initModules(void);
		bool loadProject(const char* fullname);
		bool saveProject(bool save_compact = false);
		bool saveProjectAs(bool save_compact = false);
		bool askToSaveProject(void);

	public slots:
		void closeWindow(void);
		void frameUpdate(void);
		void updateCheckedItems(void);
		void updateHistoryItems(void);
	private slots:
		void openProject(void);
		void saveProjectCb(void);
		void saveProjectAsCb(void);
		void saveProjectCompactCb(void);
		void createNewProject(void);
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
		void tabViewChanged(int);

	friend class RECORDER;
	friend class SPLICER;
	friend class SELECTION;
	friend class PLAYBACK;
	friend class HISTORY;
	friend class MARKERS_MANAGER;
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
