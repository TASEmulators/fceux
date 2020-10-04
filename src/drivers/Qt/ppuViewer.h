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
#include <QSlider>
#include <QLineEdit>
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

		QGroupBox  *paletteFrame;
		QLabel     *tileLabel[2];
		QCheckBox  *sprite8x16Cbox;
		QCheckBox  *maskUnusedCbox;
		QCheckBox  *invertMaskCbox;
		QSlider    *refreshSlider;
		QLineEdit  *scanLineEdit;

   public slots:
      void closeWindow(void);
	private slots:
		//void updatePeriodic(void);
		void sprite8x16Changed(int state);

};

int openPPUViewWindow( QWidget *parent );

