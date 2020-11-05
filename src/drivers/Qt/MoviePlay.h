// MoviePlay.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>

#include "Qt/main.h"

class MoviePlayDialog_t : public QDialog
{
   Q_OBJECT

	public:
		MoviePlayDialog_t(QWidget *parent = 0);
		~MoviePlayDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QComboBox    *movSelBox;
		QPushButton  *movBrowseBtn;
		QCheckBox    *openReadOnly;
		QCheckBox    *pauseAtFrame;
		QLineEdit    *pauseAtFrameEntry;

		QLabel       *movLenLbl;
		QLabel       *movFramesLbl;
		QLabel       *recCountLbl;
		QLabel       *recFromLbl;
		QLabel       *romUsedLbl;
		QLabel       *romCsumLbl;
		QLabel       *curCsumLbl;
		QLabel       *emuUsedLbl;
		QLabel       *palUsedLbl;
		QLabel       *newppuUsedLbl;

	private:

   public slots:
      void closeWindow(void);
	private slots:
		void openMovie(void);
		//void  readOnlyReplayChanged( int state );

};
