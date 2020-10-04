// ppuViewer.h
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

#include "Qt/main.h"

class ppuPatternView_t : public QWidget
{
	Q_OBJECT

	public:
		ppuPatternView_t(QWidget *parent = 0);
		~ppuPatternView_t(void);

	protected:
		void paintEvent(QPaintEvent *event);
};

class ppuPalatteView_t : public QWidget
{
	Q_OBJECT

	public:
		ppuPalatteView_t(QWidget *parent = 0);
		~ppuPalatteView_t(void);

	protected:
		void paintEvent(QPaintEvent *event);
};

class ppuViewerDialog_t : public QDialog
{
   Q_OBJECT

	public:
		ppuViewerDialog_t(QWidget *parent = 0);
		~ppuViewerDialog_t(void);

	protected:
      //QTimer    *inputTimer;

		ppuPatternView_t *patternView;
		ppuPalatteView_t *paletteView;

      void closeEvent(QCloseEvent *bar);
	private:

   public slots:
      void closeWindow(void);
	private slots:
		//void updatePeriodic(void);

};

int openPPUViewWindow( QWidget *parent );

