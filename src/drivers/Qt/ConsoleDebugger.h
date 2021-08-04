// ConsoleDebugger.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QRadioButton>
#include <QFont>
#include <QLabel>
#include <QTimer>
#include <QFrame>
#include <QSpinBox>
#include <QGroupBox>
#include <QTreeView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QLineEdit>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QClipboard>
#include <QScrollBar>
#include <QTabBar>
#include <QTabWidget>
#include <QSplitter>
#include <QToolBar>
#include <QMenuBar>
#include <QDropEvent>
#include <QDragEnterEvent>

#include "Qt/main.h"
#include "Qt/SymbolicDebug.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/ColorMenu.h"
#include "../../debug.h"

struct dbg_asm_entry_t
{
	int  addr;
	int  bank;
	int  rom;
	int  size;
	int  line;
	uint8  opcode[3];
	std::string  text;
	debugSymbol_t  sym;
	int  bpNum;

	enum
	{
		ASM_TEXT = 0,
		SYMBOL_NAME,
		SYMBOL_COMMENT
	} type;

	dbg_asm_entry_t(void)
	{
		addr = 0; bank = -1; rom = -1; 
		size = 0; line =  0; type = ASM_TEXT;
		bpNum = -1;

		for (int i=0; i<3; i++)
		{
			opcode[i] = 0;
		}
	}
};

struct dbg_nav_entry_t
{
	int  addr;

	dbg_nav_entry_t(void)
	{
		addr = 0;
	}
};

class debuggerBookmark_t
{
	public:
		int  addr;
		std::string  name;

		debuggerBookmark_t(void)
		{
			addr = 0;
		}
};

class debuggerBookmarkManager_t
{
	public:
		debuggerBookmarkManager_t(void);
		~debuggerBookmarkManager_t(void);

		int addBookmark( int addr, const char *name = NULL );
		int editBookmark( int addr, const char *name );
		int deleteBookmark( int addr );

		int  size(void);
		void clear(void);
		debuggerBookmark_t *begin(void);
		debuggerBookmark_t *next(void);
		debuggerBookmark_t *getAddr( int addr );
	private:
		std::map <int, debuggerBookmark_t*> bmMap;
		std::map <int, debuggerBookmark_t*>::iterator internal_iter;
};

class ConsoleDebugger;

class QAsmView : public QWidget
{
   Q_OBJECT

	public:
		QAsmView(QWidget *parent = 0);
		~QAsmView(void);

		void setScrollBars( QScrollBar *h, QScrollBar *v );
		void updateAssemblyView(void);
		void asmClear(void);
		int  getAsmLineFromAddr(int addr);
		int  getAsmAddrFromLine(int line);
		void setLine(int lineNum);
		void setXScroll(int value);
		void scrollToPC(void);
		void scrollToLine( int line );
		void scrollToAddr( int addr );
		void gotoLine( int line );
		void gotoAddr( int addr );
		void gotoPC(void);
		void setSelAddrToLine( int line );
		void setDisplayROMoffsets( bool value );
		void setSymbolDebugEnable( bool value );
		void setRegisterNameEnable( bool value );
		void setDisplayByteCodes( bool value );
		int  getCtxMenuLine(void){ return ctxMenuLine; };
		int  getCtxMenuAddr(void){ return ctxMenuAddr; };
		int  getCtxMenuAddrType(void){ return ctxMenuAddrType; };
		int  getCursorAddr(void){ return cursorLineAddr; };
		void setPC_placement( int mode, int ofs = 0 );
		void setBreakpointAtSelectedLine(void);
		int  isBreakpointAtLine( int line );
		int  isBreakpointAtAddr( int cpuAddr, int romAddr );
		void determineLineBreakpoints(void);
		void setFont( const QFont &font );
		void setIsPopUp(bool value);
		void pushAddrHist(void);
		void navHistBack(void);
		void navHistForward(void);

		QColor  opcodeColor;
		QColor  addressColor;
		QColor  immediateColor;
		QColor  commentColor;
		QColor  labelColor;
		QColor  pcBgColor;

		QFont getFont(void){ return font; };

	protected:
		bool event(QEvent *event) override;
		void paintEvent(QPaintEvent *event) override;
		void keyPressEvent(QKeyEvent *event) override;
		void keyReleaseEvent(QKeyEvent *event) override;
		void mousePressEvent(QMouseEvent * event) override;
		void mouseReleaseEvent(QMouseEvent * event) override;
		void mouseMoveEvent(QMouseEvent * event) override;
		void mouseDoubleClickEvent(QMouseEvent * event) override;
		void resizeEvent(QResizeEvent *event) override;
		void wheelEvent(QWheelEvent *event) override;
		void contextMenuEvent(QContextMenuEvent *event) override;
		void loadHighlightToClipboard(void);
		void toggleBreakpoint(int line);

		void calcFontData(void);
		void calcMinimumWidth(void);
		void calcLineOffsets(void);
		QPoint convPixToCursor( QPoint p );
		bool textIsHighlighted(void);
		void setHighlightEndCoord( int x, int y );
		void loadClipboard( const char *txt );
		void drawText( QPainter *painter, int x, int y, const char *txt );
		void drawAsmLine( QPainter *painter, int x, int y, const char *txt );
		void drawLabelLine( QPainter *painter, int x, int y, const char *txt );
		void drawCommentLine( QPainter *painter, int x, int y, const char *txt );
		void drawPointerPC( QPainter *painter, int xl, int yl );

	private:
		ConsoleDebugger *parent;
		QFont       font;
		QScrollBar *vbar;
		QScrollBar *hbar;
		QClipboard *clipboard;

		std::vector <dbg_nav_entry_t> navBckHist;
		std::vector <dbg_nav_entry_t> navFwdHist;
		dbg_nav_entry_t  curNavLoc;

		int ctxMenuLine;
		int ctxMenuAddr;
		int ctxMenuAddrType;
		int maxLineLen;
		int pxCharWidth;
		int pxCharHeight;
		int pxCursorHeight;
		int pxLineSpacing;
		int pxLineLead;
		int viewLines;
		int viewWidth;
		int viewHeight;
		int lineOffset;
		int maxLineOffset;
		int pxLineWidth;
		int pxLineXScroll;
		int cursorPosX;
		int cursorPosY;
		int cursorLineAddr;
		int pcLinePlacement;
		int pcLineOffset;
		int pcLocLinePos;
		int byteCodeLinePos;
		int opcodeLinePos;
		int operandLinePos;

		int  selAddrLine;
		int  selAddrChar;
		int  selAddrWidth;
		int  selAddrValue;
		char selAddrText[128];
		char selAddrType;

		int  txtHlgtAnchorChar;
		int  txtHlgtAnchorLine;
		int  txtHlgtStartChar;
		int  txtHlgtStartLine;
		int  txtHlgtEndChar;
		int  txtHlgtEndLine;

		int  wheelPixelCounter;

		dbg_asm_entry_t  *asmPC;
		std::vector <dbg_asm_entry_t*> asmEntry;

		bool  useDarkTheme;
		bool  displayROMoffsets;
		bool  symbolicDebugEnable;
		bool  registerNameEnable;
		bool  mouseLeftBtnDown;
		bool  showByteCodes;
		bool  isPopUp;

};

class DebuggerStackDisplay : public QPlainTextEdit
{
   Q_OBJECT

	public:
		DebuggerStackDisplay(QWidget *parent = 0);
		~DebuggerStackDisplay(void);

		void updateText(void);
		void setFont( const QFont &font );

	protected:
		void keyPressEvent(QKeyEvent *event) override;
		void contextMenuEvent(QContextMenuEvent *event) override;
		void recalcCharsPerLine(void);
		
		int  pxCharWidth;
		int  pxLineSpacing;
		int  charsPerLine;
		int  stackBytesPerLine;
		bool showAddrs;

	public slots:
		void toggleShowAddr(void);
		void sel1BytePerLine(void);
		void sel2BytesPerLine(void);
		void sel3BytesPerLine(void);
		void sel4BytesPerLine(void);
};

class asmLookAheadPopup : public fceuCustomToolTip
{
   Q_OBJECT
	public:
	   asmLookAheadPopup( int line, QWidget *parent = nullptr );
	   ~asmLookAheadPopup(void);

		QAsmView    *asmView;
	private:
};

class ppuRegPopup : public fceuCustomToolTip
{
   Q_OBJECT
	public:
	   ppuRegPopup( QWidget *parent = nullptr );
	   ~ppuRegPopup(void);

	private:
};

class ppuCtrlRegDpy : public QLineEdit
{
   Q_OBJECT

	public:
	   ppuCtrlRegDpy( QWidget *parent = nullptr );
	   ~ppuCtrlRegDpy(void);

	protected:
		bool event(QEvent *event) override;

	private:
		ppuRegPopup  *popup;

	public slots:
};

class DebuggerTabBar : public QTabBar
{
	Q_OBJECT

	public:
		DebuggerTabBar( QWidget *parent = nullptr );
		~DebuggerTabBar( void );

	public slots:

	signals:
		void beginDragOut(int);
	protected:
		void mousePressEvent(QMouseEvent * event) override;
		void mouseReleaseEvent(QMouseEvent * event) override;
		void mouseMoveEvent(QMouseEvent * event) override;
		void contextMenuEvent(QContextMenuEvent *event) override;
	private:
		bool theDragPress;
		bool theDragOut;

	private slots:
};

class DebuggerTabWidget : public QTabWidget
{
   Q_OBJECT

	public:
		DebuggerTabWidget( int row, int col, QWidget *parent = nullptr );
		~DebuggerTabWidget( void );

		void popPage(QWidget *page);
		bool indexValid(int idx);

		void buildContextMenu(QContextMenuEvent *event);

		int  row(void){ return _row; }
		int  col(void){ return _col; }
	protected:
		void mouseMoveEvent(QMouseEvent * event) override;
		void dragEnterEvent(QDragEnterEvent *event) override;
		void dropEvent(QDropEvent *event) override;
		void contextMenuEvent(QContextMenuEvent *event) override;
	private:
		DebuggerTabBar  *bar;
		int  _row;
		int  _col;
};

class DebugBreakOnDialog : public QDialog
{
   Q_OBJECT

	public:
		DebugBreakOnDialog(int type, QWidget *parent = 0);
		~DebugBreakOnDialog(void);

		unsigned long long int getThreshold(void){ return threshold; }

	protected:
		void closeEvent(QCloseEvent *event) override;
		void updateLabel(void);
		void updateCurrent(void);

		int           type;
		QRadioButton *oneShotBtn;
		QRadioButton *contBtn;
		QRadioButton *absBtn;
		QRadioButton *relBtn;
		QLineEdit    *countEntryBox;
		QLabel       *currLbl;
		QLabel       *descLbl;
		int           prevPauseState;
		long long int  totalCount;
		long long int  deltaCount;
		unsigned long long int threshold;

	public slots:
		void closeWindow(int ret);
		void setThreshold( unsigned long long int val );
		void setThreshold( const QString &text );
		void incrThreshold(int);
		void refModeChanged(bool);
		void syncToCurrent(void);
		void resetCounters(void);
		void resetDeltas(void);
};

class ConsoleDebugger : public QDialog
{
   Q_OBJECT

	public:
		ConsoleDebugger(QWidget *parent = 0);
		~ConsoleDebugger(void);

		void updateWindowData(void);
		void updateRegisterView(void);
		void updateTabVisibility(void);
		void breakPointNotify(int bpNum);
		void openBpEditWindow(int editIdx = -1, watchpointinfo *wp = NULL, bool forceAccept = false );
		void openDebugSymbolEditWindow( int addr );
		void setBookmarkSelectedAddress( int addr );
		int  getBookmarkSelectedAddress(void){ return selBmAddrVal; };
		void edit_BM_name( int addr );
		void queueUpdate(void);

		QLabel    *asmLineSelLbl;

		void setCpuStatusFont( const QFont &font );
		void setPpuStatusFont( const QFont &font );
	protected:
		void closeEvent(QCloseEvent *event) override;
		//void keyPressEvent(QKeyEvent *event) override;
		//void keyReleaseEvent(QKeyEvent *event) override;

		//QTreeWidget *tree;
		QToolBar    *toolBar;
		QScrollBar  *vbar;
		QScrollBar  *hbar;
		QAsmView    *asmView;
		DebuggerStackDisplay *stackText;
		//QLineEdit *seekEntry;
		QLineEdit *pcEntry;
		QLineEdit *regAEntry;
		QLineEdit *regXEntry;
		QLineEdit *regYEntry;
		QLineEdit *regPEntry;
		//QLineEdit *cpuCycExdVal;
		//QLineEdit *instrExdVal;
		QLineEdit *selBmAddr;
		QLineEdit *cpuCyclesVal;
		QLineEdit *cpuInstrsVal;
		QLineEdit *ppuBgAddr;
		QLineEdit *ppuSprAddr;
		QFrame    *cpuFrame;
		QFrame    *ppuFrame;
		QGroupBox *stackFrame;
		QFrame    *bpFrame;
		QGroupBox *sfFrame;
		QFrame    *bmFrame;
		QTreeWidget *bpTree;
		QTreeWidget *bmTree;
		QPushButton *bpAddBtn;
		QPushButton *bpEditBtn;
		QPushButton *bpDelBtn;
		QCheckBox *N_chkbox;
		QCheckBox *V_chkbox;
		QCheckBox *U_chkbox;
		QCheckBox *B_chkbox;
		QCheckBox *D_chkbox;
		QCheckBox *I_chkbox;
		QCheckBox *Z_chkbox;
		QCheckBox *C_chkbox;

		ppuCtrlRegDpy *ppuCtrlReg;
		ppuCtrlRegDpy *ppuMaskReg;
		ppuCtrlRegDpy *ppuStatReg;
		QLineEdit     *ppuAddrDsp;
		QLineEdit     *oamAddrDsp;
		QLineEdit     *ppuScanLineDsp;
		QLineEdit     *ppuPixelDsp;
		QLineEdit     *ppuScrollX;
		QLineEdit     *ppuScrollY;
		QGridLayout   *ppuDataGrid;

		QAction   *brkOnCycleExcAct;
		QAction   *brkOnInstrExcAct;

		DebuggerTabWidget *tabView[2][4];
		QWidget   *asmViewContainerWidget;
		QWidget   *bpTreeContainerWidget;
		QWidget   *bmTreeContainerWidget;
		QWidget   *ppuStatContainerWidget;
		QLabel    *emuStatLbl;
		QLabel    *cpuCyclesLbl1;
		QLabel    *cpuInstrsLbl1;
		QTimer    *periodicTimer;
		QFont      font;
		QFont      cpuFont;

		QVBoxLayout   *mainLayoutv;
		QSplitter     *mainLayouth;
		QSplitter     *vsplitter[2];
		QVBoxLayout   *asmDpyVbox;

		ColorMenuItem *opcodeColorAct;
		ColorMenuItem *addressColorAct;
		ColorMenuItem *immediateColorAct;
		ColorMenuItem *commentColorAct;
		ColorMenuItem *labelColorAct;
		ColorMenuItem *pcColorAct;

		int   selBmAddrVal;
		bool  windowUpdateReq;

	private:
		void setRegsFromEntry(void);
		void bpListUpdate( bool reset = false );
		void bmListUpdate( bool reset = false );
		void buildAsmViewDisplay(void);
		void buildCpuListDisplay(void);
		void buildPpuListDisplay(void);
		void buildBpListDisplay(void);
		void buildBmListDisplay(void);
		void loadDisplayViews(void);
		void saveDisplayViews(void);

		QMenuBar *buildMenuBar(void);
		QToolBar *buildToolBar(void);

	public slots:
		void closeWindow(void);
		void asmViewCtxMenuGoTo(void);
		void asmViewCtxMenuAddBP(void);
		void asmViewCtxMenuAddBM(void);
		void asmViewCtxMenuAddSym(void);
		void asmViewCtxMenuOpenHexEdit(void);
		void asmViewCtxMenuRunToCursor(void);
		void moveTab( QWidget *w, int row, int column);
	private slots:
		void updatePeriodic(void);
		void hbarChanged(int value);
		void vbarChanged(int value);
		void debugRunCB(void);
		void debugStepIntoCB(void);
		void debugStepOutCB(void);
		void debugStepOverCB(void);
		void debugRunToCursorCB(void);
		void debugRunLineCB(void);
		void debugRunLine128CB(void);
		void openGotoAddrDialog(void);
		void openChangePcDialog(void);
		//void seekToCB(void);
		void seekPCCB(void);
		void add_BP_CB(void);
		void edit_BP_CB(void);
		void delete_BP_CB(void);
		void add_BM_CB(void);
		void edit_BM_CB(void);
		void delete_BM_CB(void);
		void setLayoutOption(int layout);
		void resizeToMinimumSizeHint(void);
		void resetCountersCB (void);
		void reloadSymbolsCB(void);
		void displayByteCodesCB(bool value);
		void displayROMoffsetCB(bool value);
		void symbolDebugEnableCB(bool value);
		void registerNameEnableCB(bool value);
		void autoOpenDebugCB( bool value );
		void debFileAutoLoadCB( bool value );
		void breakOnBadOpcodeCB(bool value);
		void breakOnNewCodeCB(bool value);
		void breakOnNewDataCB(bool value);
		void breakOnCyclesCB( bool value );
		void breakOnInstructionsCB( bool value );
		void bpItemClicked( QTreeWidgetItem *item, int column);
		void bmItemClicked( QTreeWidgetItem *item, int column);
		void bmItemDoubleClicked( QTreeWidgetItem *item, int column);
		void cpuCycleThresChanged(const QString &txt);
		void instructionsThresChanged(const QString &txt);
		void selBmAddrChanged(const QString &txt);
		void pcSetPlaceTop(void);
		void pcSetPlaceUpperMid(void);
		void pcSetPlaceCenter(void);
		void pcSetPlaceLowerMid(void);
		void pcSetPlaceBottom(void);
		void pcSetPlaceCustom(void);
		void changeAsmFontCB(void);
		void changeStackFontCB(void);
		void changeCpuFontCB(void);
		void navHistBackCB(void);
		void navHistForwardCB(void);

};

bool debuggerWindowIsOpen(void);
void debuggerWindowSetFocus(bool val = true);
bool debuggerWaitingAtBreakpoint(void);
void bpDebugSetEnable(bool val);
void saveGameDebugBreakpoints( bool force = false );
void loadGameDebugBreakpoints(void);
void debuggerClearAllBreakpoints(void);
void debuggerClearAllBookmarks(void);
void updateAllDebuggerWindows(void);

extern debuggerBookmarkManager_t dbgBmMgr;
