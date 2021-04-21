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
#include <QRadioButton>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QSlider>
#include <QSpinBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QCloseEvent>
#include <QPropertyAnimation>

#include "Qt/main.h"

struct ppuPatternTable_t
{
	struct 
	{
		struct 
		{
			QColor color;
			char   val;
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

		void updateCycleCounter(void);
		void updateSelTileLabel(void);
		void setPattern( ppuPatternTable_t *p );
		void setTileLabel( QLabel *l );
		void setTileCoord( int x, int y );
		void setHoverFocus( bool h );
		QPoint convPixToTile( QPoint p );

		bool getDrawTileGrid(void){ return drawTileGrid; };
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		void keyPressEvent(QKeyEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent * event);
		void contextMenuEvent(QContextMenuEvent *event);

		void openColorPicker( QColor *c );

		int patternIndex;
		int viewWidth;
		int viewHeight;
		int cycleCount;
		int mode;
		bool drawTileGrid;
		bool hover2Focus;
		QLabel *tileLabel;
		QPoint  selTile;
		QColor  selTileColor;
		QColor  gridColor;
		ppuPatternTable_t *pattern;
   public slots:
	void toggleTileGridLines(void);
   	void setTileSelectorColor(void);
   	void setTileGridColor(void);
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
	void openTileEditor(void);
	void cycleNextPalette(void);
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

class ppuTileView_t : public QWidget
{
	Q_OBJECT

	public:
		ppuTileView_t( int patternIndex, QWidget *parent = 0);
		~ppuTileView_t(void);

		int  getPatternIndex(void){ return patternIndex; };
		void setPattern( ppuPatternTable_t *p );
		void setPaletteNES( int palIndex );
		void setTileLabel( QLabel *l );
		void setTile( QPoint *t );
		QPoint convPixToCell( QPoint p );
		QPoint getSelPix(void){ return selPix; };
		void setSelCell( QPoint &p );
		void moveSelBoxUpDown(int i);
		void moveSelBoxLeftRight(int i);
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		void keyPressEvent(QKeyEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent * event);
		void contextMenuEvent(QContextMenuEvent *event);

		int patternIndex;
		int paletteIndex;
		int viewWidth;
		int viewHeight;
		int boxWidth;
		int boxHeight;
		bool drawTileGrid;
		QLabel *tileLabel;
		QPoint  selTile;
		QPoint  selPix;
		ppuPatternTable_t *pattern;
   private slots:
      //void showTileMode(void);
      //void exitTileMode(void);
      //void selPalette0(void);
      //void selPalette1(void);
      //void selPalette2(void);
      //void selPalette3(void);
      //void selPalette4(void);
      //void selPalette5(void);
      //void selPalette6(void);
      //void selPalette7(void);
      //void selPalette8(void);
      //void cycleNextPalette(void);
      //void toggleTileGridLines(void);
};

class ppuTileEditColorPicker_t : public QWidget
{
	Q_OBJECT

	public:
		ppuTileEditColorPicker_t(QWidget *parent = 0);
		~ppuTileEditColorPicker_t(void);

		void  setColor( int colorIndex );
		void  setPaletteNES( int palIndex );

		QPoint convPixToTile( QPoint p );

		static const int NUM_COLORS = 4;

	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent * event);
		int viewWidth;
		int viewHeight;
		int boxWidth;
		int boxHeight;
		int selColor;
		int pxCharWidth;
		int pxCharHeight;
		int paletteIndex;
		//int paletteIY;
		//int paletteIX;

		QFont   font;
};

class ppuTileEditor_t : public QDialog
{
   Q_OBJECT

	public:
		ppuTileEditor_t(int patternIndex, QWidget *parent = 0);
		~ppuTileEditor_t(void);

		void setTile( QPoint *t );
		void setCellValue( int y, int x, int val );

		ppuTileView_t  *tileView;
		ppuTileEditColorPicker_t *colorPicker;

	protected:

		void keyPressEvent(QKeyEvent *event);
		void closeEvent(QCloseEvent *bar);

		QTimer     *updateTimer;
	private:
		QLabel    *tileIdxLbl;
		QComboBox *palSelBox;
		int     palIdx;
		int     tileAddr;

	public slots:
		void closeWindow(void);
	private slots:
		void periodicUpdate(void);
		void paletteChanged(int index);
		void showKeyAssignments(void);
};

class ppuViewerDialog_t : public QDialog
{
   Q_OBJECT

	public:
		ppuViewerDialog_t(QWidget *parent = 0);
		~ppuViewerDialog_t(void);

		ppuPatternView_t *patternView[2];
		ppuPalatteView_t *paletteView;
	protected:

		void closeEvent(QCloseEvent *bar);
	private:

		QGroupBox  *patternFrame[2];
		QGroupBox  *paletteFrame;
		QLabel     *tileLabel[2];
		QCheckBox  *sprite8x16Cbox[2];
		QCheckBox  *maskUnusedCbox;
		QCheckBox  *invertMaskCbox;
		QSlider    *refreshSlider;
		QSpinBox   *scanLineEdit;
		QTimer     *updateTimer;

		int         cycleCount;

	public slots:
		void closeWindow(void);
	private slots:
		void periodicUpdate(void);
		void sprite8x16Changed0(int state);
		void sprite8x16Changed1(int state);
		void refreshSliderChanged(int value);
		void scanLineChanged(int value);
		void setClickFocus(void);
		void setHoverFocus(void);
};

struct oamSpriteData_t
{
	struct 
	{
		struct 
		{
			QColor color;
			char   val;
		} pixel[8][8];

	} tile[2];

	uint8_t tNum;
	uint8_t bank;
	uint8_t pal;
	uint8_t pri;
	uint8_t hFlip;
	uint8_t vFlip;
	int     chrAddr;
	int     x;
	int     y;

};

struct oamPatternTable_t
{
	struct oamSpriteData_t sprite[64];

	bool  mode8x16;
	int  w;
	int  h;
};

class oamPatternView_t : public QWidget
{
	Q_OBJECT

	public:
		oamPatternView_t( QWidget *parent = 0);
		~oamPatternView_t(void);

		QPoint convPixToTile( QPoint p );

		int  getSpriteIndex(void);
		void setHover2Focus(bool val);
		void setGridVisibility(bool val);
		bool getGridVisibility(void){ return showGrid; };
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		void keyPressEvent(QKeyEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent * event);
		void contextMenuEvent(QContextMenuEvent *event);
		int  heightForWidth(int w) const;
		QSize  minimumSizeHint(void) const;
		QSize  maximumSizeHint(void) const;
		QSize  sizeHint(void) const;

		int  viewWidth;
		int  viewHeight;

		bool hover2Focus;
		bool showGrid;

		QColor selSpriteBoxColor;
		QPoint selSprite;
		int    spriteIdx;
	private:

	private slots:
		void openTilePpuViewer(void);

};

class oamTileView_t : public QWidget
{
	Q_OBJECT

	public:
		oamTileView_t( QWidget *parent = 0);
		~oamTileView_t(void);

		void setIndex( int val );
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		int  heightForWidth(int w) const;
		QSize  minimumSizeHint(void) const;
		QSize  maximumSizeHint(void) const;
		QSize  sizeHint(void) const;

	private:
		int  viewWidth;
		int  viewHeight;
		int  spriteIdx;
};

class oamPaletteView_t : public QWidget
{
	Q_OBJECT

	public:
		oamPaletteView_t( QWidget *parent = 0);
		~oamPaletteView_t(void);

		void setIndex( int val );
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		int  heightForWidth(int w) const;
		QSize  minimumSizeHint(void) const;
		QSize  maximumSizeHint(void) const;
		QSize  sizeHint(void) const;

	private:
		int  viewWidth;
		int  viewHeight;
		int  palIdx;
};

class oamPreview_t : public QWidget
{
	Q_OBJECT

	public:
		oamPreview_t( QWidget *parent = 0);
		~oamPreview_t(void);

		void setIndex(int val);
		void setMinScale(int val);
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		int  heightForWidth(int w) const;
		QSize  minimumSizeHint(void) const;
		QSize  maximumSizeHint(void) const;
		QSize sizeHint(void) const;

	private:
		int  viewWidth;
		int  viewHeight;
		int  selSprite;
		int  cx;
		int  cy;
};

class spriteViewerDialog_t : public QDialog
{
   Q_OBJECT

	public:
		spriteViewerDialog_t(QWidget *parent = 0);
		~spriteViewerDialog_t(void);

		oamPatternView_t  *oamView;
		oamPaletteView_t  *palView;
		oamTileView_t     *tileView;
		oamPreview_t      *preView;

	protected:

		void closeEvent(QCloseEvent *bar);
	private:
		QTimer *updateTimer;
		QRadioButton *useSprRam;
		QRadioButton *useCpuPag;
		QSpinBox     *cpuPagIdx;
		QSpinBox     *scanLineEdit;
		QLineEdit    *spriteIndexBox;
		QLineEdit    *tileIndexBox;
		QLineEdit    *tileAddrBox;
		QLineEdit    *palAddrBox;
		QLineEdit    *posBox;
		QCheckBox    *hFlipBox;
		QCheckBox    *vFlipBox;
		QCheckBox    *bgPrioBox;
		QCheckBox    *showPosHex;
		QGroupBox    *previewFrame;

	public slots:
		void closeWindow(void);
	private slots:
		void periodicUpdate(void);
		void setClickFocus(void);
		void setHoverFocus(void);
		void toggleGridVis(void);
		void scanLineChanged(int value);
};

int openPPUViewWindow( QWidget *parent );
int openOAMViewWindow( QWidget *parent );
void setPPUSelPatternTile( int table, int x, int y );

