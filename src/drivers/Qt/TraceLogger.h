// TraceLogger.h
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
#include <QGroupBox>
#include <QScrollBar>
#include <QCloseEvent>
#include <QClipboard>

#include "Qt/SymbolicDebug.h"
#include "Qt/ConsoleDebugger.h"
#include "../../debug.h"

struct traceRecord_t
{
	struct {
		uint16_t  PC;
		uint8_t   A;
		uint8_t   X;
		uint8_t   Y;
		uint8_t   S;
		uint8_t   P;
	} cpu;

	uint8_t  opCode[3];
	uint8_t  opSize;
	uint8_t  asmTxtSize;
	char     asmTxt[64];

	uint64_t  frameCount;
	uint64_t  cycleCount;
	uint64_t  instrCount;
	uint64_t  flags;

	int32_t   callAddr;
	int32_t   romAddr;
	int32_t   bank;
	int32_t   skippedLines;

	traceRecord_t(void);

	int  appendAsmText( const char *txt );

   int  convToText( char *line, int *len = 0 );
};

class QTraceLogView : public QWidget
{
   Q_OBJECT

	public:
		QTraceLogView(QWidget *parent = 0);
		~QTraceLogView(void);

		void setScrollBars( QScrollBar *h, QScrollBar *v );
		void highlightClear(void);
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);
		void wheelEvent(QWheelEvent *event);
		void mousePressEvent(QMouseEvent * event);
		void mouseReleaseEvent(QMouseEvent * event);
		void mouseMoveEvent(QMouseEvent * event);
		void keyPressEvent(QKeyEvent *event);

		void calcFontData(void);
		QPoint convPixToCursor( QPoint p );
		bool textIsHighlighted(void);
		void setHighlightEndCoord( int x, int y );
		void loadClipboard( const char *txt );
		void contextMenuEvent(QContextMenuEvent *event);
		void drawText( QPainter *painter, int x, int y, const char *txt, int maxChars = 256 );
		void calcTextSel( int x, int y );
		void openBpEditWindow( int editIdx, watchpointinfo *wp, traceRecord_t *recp );
		void openDebugSymbolEditWindow( int addr, int bank = -1 );

	protected:
		QFont       font;
		QScrollBar *vbar;
		QScrollBar *hbar;
		traceRecord_t rec[64];

		int  pxCharWidth;
		int  pxCharHeight;
		int  pxLineSpacing;
		int  pxLineLead;
		int  pxCursorHeight;
		int  pxLineXScroll;
		int  pxLineWidth;
		int  viewLines;
		int  viewWidth;
		int  viewHeight;
		int  wheelPixelCounter;
		int  txtHlgtAnchorChar;
		int  txtHlgtAnchorLine;
		int  txtHlgtStartChar;
		int  txtHlgtStartLine;
		int  txtHlgtEndChar;
		int  txtHlgtEndLine;

		bool mouseLeftBtnDown;
		bool captureHighLightText;

		int  selAddrIdx;
		int  selAddrLine;
		int  selAddrChar;
		int  selAddrWidth;
		int  selAddrValue;
		char selAddrText[128];

		int          lineBufIdx[64];
		std::string  hlgtText;
		std::string  lineText[64];

		private slots:
		void ctxMenuAddBP(void);
		void ctxMenuAddSym(void);
};

class TraceLoggerDialog_t : public QDialog
{
   Q_OBJECT

	public:
		TraceLoggerDialog_t(QWidget *parent = 0);
		~TraceLoggerDialog_t(void);

	protected:
      QTimer      *updateTimer;
		QCheckBox   *logLastCbox;
		QCheckBox   *logFileCbox;
		QComboBox   *logMaxLinesComboBox;

		QCheckBox   *autoUpdateCbox;
		QCheckBox   *logRegCbox;
		QCheckBox   *logFrameCbox;
		QCheckBox   *logEmuMsgCbox;
		QCheckBox   *logProcStatFlagCbox;
		QCheckBox   *logCyclesCountCbox;
		QCheckBox   *logBreakpointCbox;
		QCheckBox   *useStackPointerCbox;
		QCheckBox   *toLeftDisassemblyCbox;
		QCheckBox   *logInstrCountCbox;
		QCheckBox   *logBankNumCbox;
		QCheckBox   *symTraceEnaCbox;
		QCheckBox   *logNewMapCodeCbox;
		QCheckBox   *logNewMapDataCbox;

		QPushButton *selLogFileButton;
		QPushButton *startStopButton;

		QTraceLogView *traceView;
		QScrollBar    *hbar;
		QScrollBar    *vbar;

		int  traceViewCounter;
		int  recbufHeadLp;

      void closeEvent(QCloseEvent *bar);

	private:

   public slots:
      void closeWindow(void);
	private slots:
      void updatePeriodic(void);
		void toggleLoggingOnOff(void);
      void logRegStateChanged(int state);
      void logFrameStateChanged(int state);
      void logEmuMsgStateChanged(int state);
      void symTraceEnaStateChanged(int state);
      void logProcStatFlagStateChanged(int state);
      void logCyclesCountStateChanged(int state);
      void logBreakpointStateChanged(int state);
      void useStackPointerStateChanged(int state);
      void toLeftDisassemblyStateChanged(int state);
      void logInstrCountStateChanged(int state);
      void logBankNumStateChanged(int state);
      void logNewMapCodeChanged(int state);
      void logNewMapDataChanged(int state);
		void logMaxLinesChanged(int index);
      void hbarChanged(int value);
      void vbarChanged(int value);
		void openLogFile(void);
};

int  initTraceLogBuffer( int maxRecs );

void openTraceLoggerWindow( QWidget *parent );
