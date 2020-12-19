// MsgLogViewer.h
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
#include <QTimer>
#include <QFrame>
#include <QGroupBox>
#include <QPlainTextEdit>

#include "Qt/main.h"

class MsgLogViewDialog_t : public QDialog
{
   Q_OBJECT

	public:
		MsgLogViewDialog_t(QWidget *parent = 0);
		~MsgLogViewDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QTimer *updateTimer;
		QPlainTextEdit *txtView;

		size_t totalLines;

	private:

   public slots:
      void closeWindow(void);
	private slots:
		void updatePeriodic(void);
		void clearLog(void);

};

