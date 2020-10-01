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
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QGroupBox>
#include <QScrollBar>
#include <QCloseEvent>

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
	char     asmTxt[46];

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
};

class QTraceLogView : public QWidget
{
   Q_OBJECT

	public:
		QTraceLogView(QWidget *parent = 0);
		~QTraceLogView(void);

		void setScrollBars( QScrollBar *h, QScrollBar *v );
	protected:
		void paintEvent(QPaintEvent *event);
		void resizeEvent(QResizeEvent *event);

		void calcFontData(void);

	protected:
		QFont       font;
		QScrollBar *vbar;
		QScrollBar *hbar;

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
};

class TraceLoggerDialog_t : public QDialog
{
   Q_OBJECT

	public:
		TraceLoggerDialog_t(QWidget *parent = 0);
		~TraceLoggerDialog_t(void);

	protected:
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

      void closeEvent(QCloseEvent *bar);

	private:

   public slots:
      void closeWindow(void);
	private slots:
		void toggleLoggingOnOff(void);
};

