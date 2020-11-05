// MovieOptions.h
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
#include <QGroupBox>
#include <QTreeView>
#include <QTreeWidget>

#include "Qt/main.h"

class MovieOptionsDialog_t : public QDialog
{
   Q_OBJECT

	public:
		MovieOptionsDialog_t(QWidget *parent = 0);
		~MovieOptionsDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QCheckBox  *readOnlyReplay;
		QCheckBox  *pauseAfterPlay;
		QCheckBox  *closeAfterPlay;
		QCheckBox  *bindSaveStates;
		QCheckBox  *dpySubTitles;
		QCheckBox  *putSubTitlesAvi;
		QCheckBox  *autoBackUp;
		QCheckBox  *loadFullStates;

	private:

   public slots:
      void closeWindow(void);
	private slots:
		void  readOnlyReplayChanged( int state );
		void  pauseAfterPlayChanged( int state );
		void  closeAfterPlayChanged( int state );
		void  bindSaveStatesChanged( int state );
		void  dpySubTitlesChanged( int state );
		void  putSubTitlesAviChanged( int state );
		void  autoBackUpChanged( int state );
		void  loadFullStatesChanged( int state );

};
