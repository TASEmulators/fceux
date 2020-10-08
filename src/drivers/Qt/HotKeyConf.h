// HotKeyConf.h
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

class HotKeyConfDialog_t : public QDialog
{
   Q_OBJECT

	public:
		HotKeyConfDialog_t(QWidget *parent = 0);
		~HotKeyConfDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);
		void keyPressEvent(QKeyEvent *event);
   	void keyReleaseEvent(QKeyEvent *event);
		void assignHotkey(QKeyEvent *event);

		QTreeWidget *tree;

	private:

   public slots:
      void closeWindow(void);
	private slots:

};
