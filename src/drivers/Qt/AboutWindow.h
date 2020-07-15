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

#include "Qt/main.h"

class AboutWindow : public QDialog
{
   Q_OBJECT

	public:
		AboutWindow(QWidget *parent = 0);
		~AboutWindow(void);

	protected:

	private:

	private slots:

};
