// ppuViewer.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QSlider>
#include <QLineEdit>
#include <QGroupBox>
#include <QCloseEvent>

#include "Qt/main.h"

struct ppuPatternTable_t
{
	struct 
	{
		struct 
		{
			QColor color;
		} pixel[8][8];

		int  x;
		int  y;

	} tile[16][16];

	int  w;
	int  h;
};

class ppuPatternView_t : public QWidget
{
	Q_OBJECT

	public:
		ppuPatternView_t( int patternIndex, QWidget *parent = 0);
		~ppuPatternView_t(void);

		void setPattern( ppuPatternTable_t *p );
		void setTileLabel( QLabel *l );
		QPoint convPixToTile( QPoint p );
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
      void keyPressEvent(QKeyEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent * event);
      void contextMenuEvent(QContextMenuEvent *event);

		int patternIndex;
		int viewWidth;
		int viewHeight;
      int mode;
      bool drawTileGrid;
		QLabel *tileLabel;
      QPoint  selTile;
		ppuPatternTable_t *pattern;
   private slots:
      void showTileMode(void);
      void exitTileMode(void);
      void selPalette0(void);
      void selPalette1(void);
      void selPalette2(void);
      void selPalette3(void);
      void selPalette4(void);
      void selPalette5(void);
      void selPalette6(void);
      void selPalette7(void);
      void selPalette8(void);
      void cycleNextPalette(void);
      void toggleTileGridLines(void);
};

class ppuPalatteView_t : public QWidget
{
	Q_OBJECT

	public:
		ppuPalatteView_t(QWidget *parent = 0);
		~ppuPalatteView_t(void);

		void setTileLabel( QGroupBox *l );
		QPoint convPixToTile( QPoint p );
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent * event);
		int viewWidth;
		int viewHeight;
		int boxWidth;
		int boxHeight;
		QGroupBox *frame;
};

class ppuViewerDialog_t : public QDialog
{
   Q_OBJECT

	public:
		ppuViewerDialog_t(QWidget *parent = 0);
		~ppuViewerDialog_t(void);

	protected:

		ppuPatternView_t *patternView[2];
		ppuPalatteView_t *paletteView;

      void closeEvent(QCloseEvent *bar);
	private:

		QGroupBox  *patternFrame[2];
		QGroupBox  *paletteFrame;
		QLabel     *tileLabel[2];
		QCheckBox  *sprite8x16Cbox[2];
		QCheckBox  *maskUnusedCbox;
		QCheckBox  *invertMaskCbox;
		QSlider    *refreshSlider;
		QLineEdit  *scanLineEdit;
		QTimer     *updateTimer;

   public slots:
      void closeWindow(void);
	private slots:
		void periodicUpdate(void);
		void sprite8x16Changed0(int state);
		void sprite8x16Changed1(int state);
		void refreshSliderChanged(int value);
		void scanLineChanged( const QString &s );
};

int openPPUViewWindow( QWidget *parent );

