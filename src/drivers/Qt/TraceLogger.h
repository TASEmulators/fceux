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
};

