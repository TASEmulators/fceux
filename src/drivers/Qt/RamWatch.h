// RamWatch.h
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

class RamWatchDialog_t : public QDialog
{
   Q_OBJECT

	public:
		RamWatchDialog_t(QWidget *parent = 0);
		~RamWatchDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

		QFont        font;
		QTreeWidget *tree;
		QPushButton *up_btn;
		QPushButton *down_btn;
		QPushButton *edit_btn;
		QPushButton *del_btn;
		QPushButton *new_btn;
		QPushButton *dup_btn;
		QPushButton *sep_btn;
		QPushButton *cht_btn;

		int  fontCharWidth;

	private:

   public slots:
      void closeWindow(void);
	private slots:

};
