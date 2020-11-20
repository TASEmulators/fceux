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
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
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
		void openGotoAddrDialog(void);
		int  checkMemActivity(void);
		int  getAddr(void){ return cursorAddr; };
		int  FreezeRam( const char *name, uint32_t a, uint8_t v, int c, int s, int type );
		void loadHighlightToClipboard(void);
		void pasteFromClipboard(void);
		void clearHighlight(void);
		int  findPattern( std::vector <unsigned char> &varray, int dir );
		void requestUpdate(void);

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
		void mouseReleaseEvent(QMouseEvent * event);
		void mouseMoveEvent(QMouseEvent * event);
		void wheelEvent(QWheelEvent *event);
		void resizeEvent(QResizeEvent *event);
		void contextMenuEvent(QContextMenuEvent *event);

		void calcFontData(void);
		void resetCursor(void);
		bool textIsHighlighted(void);
		void setHighlightEndCoord( int x, int y );
		QPoint convPixToCursor( QPoint p );
		int convPixToAddr( QPoint p );
		bool frzRamAddrValid( int addr );
		void loadClipboard( const char *txt );

		QFont      font;

		int  getRomAddrColor( int addr, QColor &fg, QColor &bg );

		memBlock_t  mb;
		int (*memAccessFunc)( unsigned int offset);

		QScrollBar *vbar;
		QScrollBar *hbar;
		QColor      highLightColor[ HIGHLIGHT_ACTIVITY_NUM_COLORS ];
		QColor      rvActvTextColor[ HIGHLIGHT_ACTIVITY_NUM_COLORS ];
		QClipboard *clipboard;

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
		int cursorAddr;
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
		int frzRamAddr;
		int frzRamVal;
		int frzRamMode;
		int frzIdx;
		int wheelPixelCounter;
		int txtHlgtAnchorChar;
		int txtHlgtAnchorLine;
		int txtHlgtStartChar;
		int txtHlgtStartLine;
		int txtHlgtStartAddr;
		int txtHlgtEndChar;
		int txtHlgtEndLine;
		int txtHlgtEndAddr;

		bool cursorBlink;
		bool reverseVideo;
		bool actvHighlightEnable;
		bool mouseLeftBtnDown;
		bool updateRequested;

	private slots:
		void jumpToROM(void);
		void addBookMarkCB(void);
		void addDebugSym(void);
		void addRamReadBP(void);
		void addRamWriteBP(void);
		void addRamExecuteBP(void);
		void addPpuReadBP(void);
		void addPpuWriteBP(void);
		void frzRamSet(void);
		void frzRamUnset(void);
		void frzRamToggle(void);
		void frzRamUnsetAll(void);

};

class HexEditorFindDialog_t : public QDialog
{
   Q_OBJECT
	public:
		HexEditorFindDialog_t(QWidget *parent = 0);
		~HexEditorFindDialog_t(void);

		QLineEdit    *searchBox;
		QRadioButton *upBtn;
		QRadioButton *dnBtn;
		QRadioButton *hexBtn;
		QRadioButton *txtBtn;
	protected:
		void closeEvent(QCloseEvent *bar);

		HexEditorDialog_t *parent;

	public slots:
		void closeWindow(void);
		void runSearch(void);
};

class HexEditorDialog_t : public QDialog
{
   Q_OBJECT

	public:
		HexEditorDialog_t(QWidget *parent = 0);
		~HexEditorDialog_t(void);

		void gotoAddress(int newAddr);
		void populateBookmarkMenu(void);
		void setWindowTitle(void);
		void openDebugSymbolEditWindow( int addr );

		QHexEdit   *editor;
		HexEditorFindDialog_t  *findDialog;

	protected:
		void closeEvent(QCloseEvent *bar);

		QScrollBar *vbar;
		QScrollBar *hbar;
		QTimer     *periodicTimer;
		QMenu      *bookmarkMenu;
		QAction    *viewRAM;
		QAction    *viewPPU;
		QAction    *viewOAM;
		QAction    *viewROM;
		QAction    *gotoAddrAct;
		QAction    *undoEditAct;

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
		void openGotoAddrDialog(void);
		void copyToClipboard(void);
		void pasteFromClipboard(void);
		void openFindDialog(void);
		void undoRomPatch(void);
};

int hexEditorNumWindows(void);
void hexEditorRequestUpdateAll(void);
void hexEditorUpdateMemoryValues(void);
void hexEditorLoadBookmarks(void);
void hexEditorSaveBookmarks(void);
int hexEditorOpenFromDebugger( int mode, int addr );
