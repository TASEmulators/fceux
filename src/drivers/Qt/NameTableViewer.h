// NameTableViewer.h

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
#include <QScrollArea>
#include <QCloseEvent>

struct  ppuNameTablePixel_t
{
	int  x;
	int  y;
	QColor color;

	ppuNameTablePixel_t(void)
	{
		x = y = 0;
	}
};

struct ppuNameTableTile_t
{
	struct ppuNameTablePixel_t pixel[8][8];

	int  x;
	int  y;
	int  pTblAdr;
	int  pTbl;
	int  pal;

	ppuNameTableTile_t(void)
	{
		x = y = 0;
		pTbl  = 0;
		pal   = 0;
		pTblAdr = 0;
	}
};

struct ppuNameTable_t
{
	struct ppuNameTableTile_t tile[30][32];

	int  x;
	int  y;
	int  w;
	int  h;

	ppuNameTable_t(void)
	{
		x = y = 0;
		w = h = 8;
	}
};

class ppuNameTableViewerDialog_t;

class ppuNameTableView_t : public QWidget
{
	Q_OBJECT

	public:
		ppuNameTableView_t( QWidget *parent = 0);
		~ppuNameTableView_t(void);

		void setViewScale( int reqScale );
		int  getViewScale( void ){ return viewScale; };

		void setScrollPointer( QScrollArea *sa );
		void setHoverFocus( bool hoverFocus );
		bool getHoverFocus( void ){ return hover2Focus; };

		void calcPixelLocations(void);

		int     getSelTable(void){ return selTable; };
		QPoint  getSelTile(void){ return selTile; };

		void    updateTable(int idx);

		QColor tileSelColor;
		QColor tileGridColor;
		QColor attrGridColor;
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		void keyPressEvent(QKeyEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mousePressEvent(QMouseEvent * event);
		void contextMenuEvent(QContextMenuEvent *event);
		void computeNameTableProperties( int NameTable, int TileX, int TileY );
		int  convertXY2TableTile( int x, int y, int *tableIdxOut, int *tileXout, int *tileYout );
		int  calcTableTileAddr( int table, int tileX, int tileY );

		ppuNameTableViewerDialog_t *parent;
		int viewWidth;
		int viewHeight;
		int viewScale;
		int ppuAddr;
		int atrbAddr;
		int tileAddr;
		int palAddr;
		int selTable;
		QPoint selTile;
		QPoint selTileLoc;
		QRect  viewRect;
		QScrollArea  *scrollArea;

		bool  ensureVis;
		bool  hover2Focus;
	private slots:
		void openTilePpuViewer(void);
		void openPpuAddrHexEdit(void);
		void openTileAddrHexEdit(void);
		void openAtrbAddrHexEdit(void);
};

class ppuNameTableTileView_t : public QWidget
{
	Q_OBJECT

	public:
		ppuNameTableTileView_t( QWidget *parent = 0);
		~ppuNameTableTileView_t(void);

		void setTile( int table, int x, int y );

	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);

		int viewWidth;
		int viewHeight;
		int selTable;
		int tileX;
		int tileY;
};

class ppuNameTablePaletteView_t : public QWidget
{
	Q_OBJECT

	public:
		ppuNameTablePaletteView_t( QWidget *parent = 0);
		~ppuNameTablePaletteView_t(void);

		void setTile( int table, int x, int y );
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
		int  selTable;
		int  tileX;
		int  tileY;
};

class ppuNameTableViewerDialog_t : public QDialog
{
   Q_OBJECT

	public:
		ppuNameTableViewerDialog_t(QWidget *parent = 0);
		~ppuNameTableViewerDialog_t(void);

		void setPropertyLabels( int TileID, int TileX, int TileY, int NameTable, int PPUAddress, int AttAddress, int Attrib, int palAddr );

	protected:
		void closeEvent(QCloseEvent *bar);

		void openColorPicker( QColor *c );
		void changeRate( int divider );

		ppuNameTableView_t *ntView;
		ppuNameTableTileView_t *selTileView;
		ppuNameTablePaletteView_t *selTilePalView;
		QScrollArea *scrollArea;
		QCheckBox *showScrollLineCbox;
		QCheckBox *showTileGridCbox;
		QCheckBox *showAttrGridCbox;
		QCheckBox *showAttrbCbox;
		QCheckBox *ignorePaletteCbox;
		QSpinBox  *scanLineEdit;
		QTimer    *updateTimer;
		QLineEdit *ppuAddrLbl;
		QLineEdit *nameTableLbl;
		QLineEdit *tileLocLbl;
		QLineEdit *tileIdxLbl;
		QLineEdit *tileAddrLbl;
		QLineEdit *attrDataLbl;
		QLineEdit *attrAddrLbl;
		QLineEdit *palAddrLbl;
		QAction *showScrollLineAct;
		QAction *showTileGridAct;
		QAction *showAttrGridAct;
		QAction *showAttributesAct;
		QAction *ignPalAct;
		QAction *zoomAct[4];
		QAction *rateAct[5];
		QAction *focusAct[2];
		QLabel  *mirrorLbl;

		int      cycleCount;

	public slots:
		void closeWindow(void);
		void refreshMenuSelections(void);
	private slots:
		void periodicUpdate(void);
		void updateMirrorText(void);
		void showAttrbChanged(int state);
		void ignorePaletteChanged(int state);
		void showTileGridChanged(int state);
		void showAttrGridChanged(int state);
		void showScrollLinesChanged(int state);
		void scanLineChanged(int value);
		void menuScrollLinesChanged( bool checked ); 
		void menuTileGridLinesChanged( bool checked ); 
		void menuAttrGridLinesChanged( bool checked ); 
		void menuAttributesChanged( bool checked );
		void menuIgnPalChanged( bool checked );
		void setTileSelectorColor(void);
		void setTileGridColor(void);
		void setAttrGridColor(void);
		void setClickFocus(void);
		void setHoverFocus(void);
		void changeZoom1x(void);
		void changeZoom2x(void);
		void changeZoom3x(void);
		void changeZoom4x(void);
		void changeRate1(void);
		void changeRate2(void);
		void changeRate4(void);
		void changeRate8(void);
		void changeRate16(void);
		void forceRefresh(void);
};

int openNameTableViewWindow( QWidget *parent );

