// GamePadConf.h
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
#include <QCloseEvent>

#include "Qt/main.h"

class AboutWindow : public QDialog
{
   Q_OBJECT

	public:
		AboutWindow(QWidget *parent = 0);
		~AboutWindow(void);

	protected:
		void closeEvent(QCloseEvent *event);

	private:

	public slots:
		void closeWindow(void);

};
