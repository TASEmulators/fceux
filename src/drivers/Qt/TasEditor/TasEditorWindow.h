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
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QFont>

class TasEditorWindow;

class QPianoRoll : public QWidget
{
	Q_OBJECT

	public:
		QPianoRoll(QWidget *parent = 0);
		~QPianoRoll(void);

		void setScrollBars( QScrollBar *h, QScrollBar *v );

		QFont getFont(void){ return font; };

	protected:
		void calcFontData(void);

	private:
		TasEditorWindow *parent;
		QFont       font;
		QScrollBar *hbar;
		QScrollBar *vbar;

		int pxCharWidth;
		int pxCharHeight;
		int pxCursorHeight;
		int pxLineSpacing;
		int pxLineLead;
		int viewLines;
		int viewWidth;
		int viewHeight;
};

class TasEditorWindow : public QDialog
{
	Q_OBJECT

	public:
		TasEditorWindow(QWidget *parent = 0);
		~TasEditorWindow(void);

		QPianoRoll  *pianoRoll;

	protected:
		void closeEvent(QCloseEvent *event);

		QMenuBar  *buildMenuBar(void);
		void buildPianoRollDisplay(void);
		void buildSideControlPanel(void);

		QMenu     *recentMenu;

		QSplitter  *mainHBox;
		QWidget    *pianoRollContainerWidget;
		QWidget    *controlPanelContainerWidget;
		QScrollBar *pianoRollHBar;
		QScrollBar *pianoRollVBar;
		QLabel     *upperMarkerLabel;
		QLabel     *lowerMarkerLabel;
		QLineEdit  *upperMarkerName;
		QLineEdit  *lowerMarkerName;

		QVBoxLayout *ctlPanelMainVbox;
		QGroupBox  *playbackGBox;
		QGroupBox  *recorderGBox;
		QGroupBox  *splicerGBox;
		QGroupBox  *luaGBox;
		QGroupBox  *bookmarksGBox;
		QGroupBox  *historyGBox;

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

		QPushButton *runLuaBtn;
		QCheckBox   *autoLuaCBox;

		QTreeWidget *bkbrTree;
		QTreeWidget *histTree;

		QPushButton *prevMkrBtn;
		QPushButton *nextMkrBtn;
		QPushButton *similarBtn;
		QPushButton *moreBtn;
	private:

		int initModules(void);

	public slots:
		void closeWindow(void);
	private slots:
};

bool tasWindowIsOpen(void);

void tasWindowSetFocus(bool val);

bool isTaseditorRecording(void);
bool recordInputByTaseditor(void);

extern TasEditorWindow *tasWin;
