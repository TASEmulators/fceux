// CodeDataLogger.h
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
#include <QCloseEvent>

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

      void closeEvent(QCloseEvent *bar);

	private:

   public slots:
      void closeWindow(void);
	private slots:
};

