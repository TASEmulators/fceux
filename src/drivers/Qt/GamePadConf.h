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

class GamePadConfDialog_t : public QDialog
{
	public:
		GamePadConfDialog_t(QWidget *parent = 0);
		~GamePadConfDialog_t(void);

	protected:
		QComboBox *portSel;



	private slots:

};
