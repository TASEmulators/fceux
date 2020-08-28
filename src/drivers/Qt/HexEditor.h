// GamePadConf.h
//

#pragma once

#include <list>
#include <vector>

#include <QWidget>
#include <QDialog>
#include <QTimer>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QFrame>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QKeyEvent>

struct memByte_t
{
	unsigned char data;
	unsigned char color;
	unsigned char actv;
};

struct memBlock_t
{
	memBlock_t(void);
	~memBlock_t(void);

	void init(void);
	int  size(void){ return _size; }
	int  numLines(void){ return _maxLines; }
	int  reAlloc( int newSize );
	void setAccessFunc( int (*newMemAccessFunc)( unsigned int offset) );

	struct memByte_t *buf;
	int  _size;
   int  _maxLines;
	int (*memAccessFunc)( unsigned int offset);
};

class HexBookMark
{
	public:
		HexBookMark(void);
		~HexBookMark(void);

	int  addr;
	int  mode;
	char desc[64];
};

class HexBookMarkManager_t
{
	public:
		HexBookMarkManager_t(void);
		~HexBookMarkManager_t(void);

		void removeAll(void);
		int  addBookMark( int addr, int mode, const char *desc );
		int  size(void);
		HexBookMark *getBookMark( int index );
		int  saveToFile(void);
		int  loadFromFile(void);
	private:
		void updateVector(void);
		std::list <HexBookMark*> ls;
		std::vector <HexBookMark*> v;
};

class QHexEdit;

class HexBookMarkMenuAction : public QAction
{
   Q_OBJECT

	public:
		HexBookMarkMenuAction( QString desc, QWidget *parent = 0);
		~HexBookMarkMenuAction(void);

		QHexEdit    *qedit;
		HexBookMark *bm;
	public slots:
		void activateCB(void);

};

class HexEditorDialog_t;

class QHexEdit : public QWidget
{
   Q_OBJECT

	public:
		QHexEdit(QWidget *parent = 0);
		~QHexEdit(void);

		int  getMode(void){ return viewMode; };
		void setMode( int mode );
		void setLine( int newLineOffset );
		void setAddr( int newAddrOffset );
		void setScrollBars( QScrollBar *h, QScrollBar *v );
		void setHorzScroll( int value );
		void setHighlightActivity( int enable );
		void setHighlightReverseVideo( int enable );
		void setForeGroundColor( QColor fg );
		void setBackGroundColor( QColor bg );
		void memModeUpdate(void);
		int  checkMemActivity(void);

		enum {
			MODE_NES_RAM = 0,
			MODE_NES_PPU,
			MODE_NES_OAM,
			MODE_NES_ROM
		};
		static const int HIGHLIGHT_ACTIVITY_NUM_COLORS = 16;
	protected:
		void paintEvent(QPaintEvent *event);
		void keyPressEvent(QKeyEvent *event);
   	void keyReleaseEvent(QKeyEvent *event);
		void mousePressEvent(QMouseEvent * event);
		void resizeEvent(QResizeEvent *event);
		void contextMenuEvent(QContextMenuEvent *event);

		void calcFontData(void);
		void resetCursor(void);
		QPoint convPixToCursor( QPoint p );
		int convPixToAddr( QPoint p );

		QFont      font;

		memBlock_t  mb;
		int (*memAccessFunc)( unsigned int offset);

		QScrollBar *vbar;
		QScrollBar *hbar;
		QColor      highLightColor[ HIGHLIGHT_ACTIVITY_NUM_COLORS ];
		QColor      rvActvTextColor[ HIGHLIGHT_ACTIVITY_NUM_COLORS ];

		HexEditorDialog_t *parent;

		uint64_t total_instructions_lp;

      int viewMode;
		int lineOffset;
		int pxCharWidth;
		int pxCharHeight;
		int pxCursorHeight;
		int pxLineSpacing;
		int pxLineLead;
		int pxLineWidth;
		int pxLineXScroll;
		int pxXoffset;
		int pxYoffset;
		int pxHexOffset;
		int pxHexAscii;
		int cursorPosX;
		int cursorPosY;
		int cursorBlinkCount;
		int viewLines;
		int viewWidth;
		int viewHeight;
      int maxLineOffset;
      int editAddr;
      int editValue;
      int editMask;
		int jumpToRomValue;
		int ctxAddr;

		bool cursorBlink;
		bool reverseVideo;
		bool actvHighlightEnable;

	private slots:
		void jumpToROM(void);
		void addBookMarkCB(void);

};

class HexEditorDialog_t : public QDialog
{
   Q_OBJECT

	public:
		HexEditorDialog_t(QWidget *parent = 0);
		~HexEditorDialog_t(void);

		void gotoAddress(int newAddr);
		void populateBookmarkMenu(void);
	protected:

		void closeEvent(QCloseEvent *bar);

		QScrollBar *vbar;
		QScrollBar *hbar;
		QHexEdit   *editor;
		QTimer     *periodicTimer;
		QMenu      *bookmarkMenu;

	private:

   public slots:
      void closeWindow(void);
	private slots:
		void updatePeriodic(void);
		void vbarMoved(int value);
		void vbarChanged(int value);
		void hbarChanged(int value);
		void saveRomFile(void);
		void saveRomFileAs(void);
		void setViewRAM(void);
		void setViewPPU(void);
		void setViewOAM(void);
		void setViewROM(void);
		void actvHighlightCB(bool value); 
		void actvHighlightRVCB(bool value); 
		void pickForeGroundColor(void);
		void pickBackGroundColor(void);
		void removeAllBookmarks(void);
};

void hexEditorLoadBookmarks(void);
void hexEditorSaveBookmarks(void);
