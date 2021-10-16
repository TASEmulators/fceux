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

		QMenu     *recentMenu;

		QSplitter  *mainHBox;
		QWidget    *pianoRollContainerWidget;
		QScrollBar *pianoRollHBar;
		QScrollBar *pianoRollVBar;
		QLabel     *upperMarkerLabel;
		QLabel     *lowerMarkerLabel;
		QLineEdit  *upperMarkerName;
		QLineEdit  *lowerMarkerName;
	private:

	public slots:
		void closeWindow(void);
	private slots:
};
