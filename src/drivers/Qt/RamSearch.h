// RamSearch.h
//

#pragma once

#include <list>
#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QTreeView>
#include <QTreeWidget>

#include "Qt/main.h"

class RamSearchDialog_t : public QDialog
{
   Q_OBJECT

	public:
		RamSearchDialog_t(QWidget *parent = 0);
		~RamSearchDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QFont        font;
		QTreeWidget *tree;
		QTimer      *updateTimer;
		QPushButton *searchButton;
		QPushButton *resetButton;
		QPushButton *clearChangeButton;
		QPushButton *undoButton;
		QPushButton *elimButton;
		QPushButton *watchButton;
		QPushButton *addCheatButton;
		QPushButton *hexEditButton;

		QRadioButton *lt_btn;
		QRadioButton *gt_btn;
		QRadioButton *le_btn;
		QRadioButton *ge_btn;
		QRadioButton *eq_btn;
		QRadioButton *ne_btn;
		QRadioButton *df_btn;
		QRadioButton *md_btn;

		QRadioButton *pv_btn;
		QRadioButton *sv_btn;
		QRadioButton *sa_btn;
		QRadioButton *nc_btn;

		QRadioButton *ds1_btn;
		QRadioButton *ds2_btn;
		QRadioButton *ds4_btn;

		QRadioButton *signed_btn;
		QRadioButton *unsigned_btn;
		QRadioButton *hex_btn;

		QLineEdit    *diffByEdit;
		QLineEdit    *moduloEdit;
		QLineEdit    *specValEdit;
		QLineEdit    *specAddrEdit;
		QLineEdit    *numChangeEdit;

		QCheckBox    *searchROMCbox;
		QCheckBox    *misalignedCbox;
		QCheckBox    *autoSearchCbox;

		int  fontCharWidth;

	private:

   public slots:
      void closeWindow(void);
	private slots:
		void periodicUpdate(void);
		//void watchClicked( QTreeWidgetItem *item, int column);

};

