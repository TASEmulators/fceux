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

class CodeDataLoggerDialog_t : public QDialog
{
   Q_OBJECT

	public:
		CodeDataLoggerDialog_t(QWidget *parent = 0);
		~CodeDataLoggerDialog_t(void);

	protected:
      QTimer      *updateTimer;
		QLabel      *prgLoggedCodeLabel;
		QLabel      *prgLoggedDataLabel;
		QLabel      *prgUnloggedLabel;
		QLabel      *chrLoggedCodeLabel;
		QLabel      *chrLoggedDataLabel;
		QLabel      *chrUnloggedLabel;
		QLabel      *cdlFileLabel;
		QLabel      *statLabel;
		QCheckBox   *autoSaveCdlCbox;
		QCheckBox   *autoLoadCdlCbox;
		QCheckBox   *autoResumeLogCbox;
		QPushButton *startPauseButton;
      void closeEvent(QCloseEvent *bar);

		void SaveStrippedROM(int invert);

	private:

   public slots:
      void closeWindow(void);
	private slots:
		void loadCdlFile(void);
		void saveCdlFile(void);
		void saveCdlFileAs(void);
		void updatePeriodic(void);
		void ResetCDLogClicked(void);
		void StartPauseCDLogClicked(void);
		void autoSaveCdlStateChange(int state);
		void autoLoadCdlStateChange(int state);
		void autoResumeCdlStateChange(int state);
		void SaveStrippedROMClicked(void);
		void SaveUnusedROMClicked(void);
};

void InitCDLog(void);
void ResetCDLog(void);
void FreeCDLog(void);
void StartCDLogging(void);
bool PauseCDLogging(void);
bool LoadCDLog(const char* nameo);
void RenameCDLog(const char* newName);
void CDLoggerROMClosed(void);
void CDLoggerROMChanged(void);
void SaveCDLogFile(void);

