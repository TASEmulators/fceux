// RamSearch.h
//

#pragma once

#include <list>
#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QScrollBar>

#include "Qt/main.h"

class QRamSearchView : public QWidget
{
	Q_OBJECT

	public:
		QRamSearchView(QWidget *parent = 0);
		~QRamSearchView(void);

		void setScrollBars( QScrollBar *hbar, QScrollBar *vbar );

      int  getSelAddr(void){ return selAddr; }
	protected:
		void paintEvent(QPaintEvent *event);
		void keyPressEvent(QKeyEvent *event);
   	//void keyReleaseEvent(QKeyEvent *event);
		void mousePressEvent(QMouseEvent * event);
		void resizeEvent(QResizeEvent *event);
		void wheelEvent(QWheelEvent *event);

      int  convPixToLine( QPoint p );
		void calcFontData(void);

		QFont       font;
		QScrollBar *vbar;
		QScrollBar *hbar;

		int  lineOffset;
		int  maxLineOffset;
		int  pxCharWidth;
		int  pxCharHeight;
		int  pxLineSpacing;
		int  pxLineLead;
		int  pxCursorHeight;
		int  pxLineXScroll;
		int  pxLineWidth;
		int  pxColWidth[4];
		int  viewLines;
		int  viewWidth;
		int  viewHeight;
      int  selAddr;
      int  selLine;
		int  wheelPixelCounter;
};

class RamSearchDialog_t : public QDialog
{
   Q_OBJECT

	public:
		RamSearchDialog_t(QWidget *parent = 0);
		~RamSearchDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QRamSearchView *ramView;
		QScrollBar  *vbar;
		QScrollBar  *hbar;
		QTimer      *updateTimer;
		QPushButton *searchButton;
		QPushButton *resetButton;
		QPushButton *clearChangeButton;
		QPushButton *undoButton;
		QPushButton *elimButton;
		QPushButton *watchButton;
		QPushButton *addCheatButton;
		QPushButton *hexEditButton;

		QRadioButton *lt_btn;
		QRadioButton *gt_btn;
		QRadioButton *le_btn;
		QRadioButton *ge_btn;
		QRadioButton *eq_btn;
		QRadioButton *ne_btn;
		QRadioButton *df_btn;
		QRadioButton *md_btn;

		QRadioButton *pv_btn;
		QRadioButton *sv_btn;
		QRadioButton *sa_btn;
		QRadioButton *nc_btn;

		QRadioButton *ds1_btn;
		QRadioButton *ds2_btn;
		QRadioButton *ds4_btn;

		QRadioButton *signed_btn;
		QRadioButton *unsigned_btn;
		QRadioButton *hex_btn;

		QLineEdit    *diffByEdit;
		QLineEdit    *moduloEdit;
		QLineEdit    *specValEdit;
		QLineEdit    *specAddrEdit;
		QLineEdit    *numChangeEdit;

		QCheckBox    *searchROMCbox;
		QCheckBox    *misalignedCbox;
		QCheckBox    *autoSearchCbox;

		int  fontCharWidth;
		int  frameCounterLastPass;
		unsigned int cycleCounter;


	private:
		void updateRamValues(void);
		void calcRamList(void);
      void SearchRelative(void);
      void SearchSpecificValue(void);
      void SearchSpecificAddress(void);
      void SearchNumberChanges(void);

   public slots:
      void closeWindow(void);
	private slots:
		void runSearch(void);
		void resetSearch(void);
		void undoSearch(void);
		void clearChangeCounts(void);
		void eliminateSelAddr(void);
		void hexEditSelAddr(void);
		void addCheatClicked(void);
		void addRamWatchClicked(void);
		void periodicUpdate(void);
		void hbarChanged(int val);
		void vbarChanged(int val);
		void searchROMChanged(int state);
		void misalignedChanged(int state);
		void ds1Clicked(void);
		void ds2Clicked(void);
		void ds4Clicked(void);
		void signedTypeClicked(void);
		void unsignedTypeClicked(void);
		void hexTypeClicked(void);
		void opLtClicked(void);
		void opGtClicked(void);
		void opLeClicked(void);
		void opGeClicked(void);
		void opEqClicked(void);
		void opNeClicked(void);
		void opDfClicked(void);
		void opMdClicked(void);
		void pvBtnClicked(void);
		void svBtnClicked(void);
		void saBtnClicked(void);
		void ncBtnClicked(void);

};

void openRamSearchWindow(QWidget *parent);
