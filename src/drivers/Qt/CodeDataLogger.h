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
      QTimer    *updateTimer;
		QLabel    *prgLoggedCodeLabel;
		QLabel    *prgLoggedDataLabel;
		QLabel    *prgUnloggedLabel;
		QLabel    *chrLoggedCodeLabel;
		QLabel    *chrLoggedDataLabel;
		QLabel    *chrUnloggedLabel;
		QLabel    *cdlFileLabel;
		QCheckBox *autoSaveCdlCbox;
		QCheckBox *autoLoadCdlCbox;
		QCheckBox *autoResumeLogCbox;
      void closeEvent(QCloseEvent *bar);
	private:

   public slots:
      void closeWindow(void);
	private slots:
		void updatePeriodic(void);

};

