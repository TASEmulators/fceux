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
		void   doScan(void);
		void   clearMovieText(void);
		void   updateMovieText(void);
		int    addFileToList( const char *file, bool setActive = false );
		bool   checkMD5Sum( const char *path, const char *md5 );
		void   scanDirectory( const char *dirPath, const char *md5 );

   public slots:
      void closeWindow(void);
	private slots:
		void openMovie(void);
		void playMovie(void);
		void movieSelect(int index);
		//void  readOnlyReplayChanged( int state );

};
