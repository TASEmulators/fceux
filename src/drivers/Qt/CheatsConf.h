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
#include <QLineEdit>
#include <QGroupBox>
#include <QTreeView>
#include <QTreeWidget>

#include "Qt/main.h"

class GuiCheatsDialog_t : public QDialog
{
   Q_OBJECT

	public:
		GuiCheatsDialog_t(QWidget *parent = 0);
		~GuiCheatsDialog_t(void);

	protected:

		QGroupBox   *actCheatFrame;
		QGroupBox   *cheatSearchFrame;
		QGroupBox   *cheatResultFrame;
		QPushButton *addCheatBtn;
		QPushButton *delCheatBtn;
		QPushButton *modCheatBtn;
		QPushButton *importCheatFileBtn;
		QPushButton *exportCheatFileBtn;
		QPushButton *srchResetBtn;
		QPushButton *knownValBtn;
		QTreeWidget *tree;
		QLineEdit   *cheatNameEntry;
		QLineEdit   *cheatAddrEntry;
		QLineEdit   *cheatValEntry;
		QLineEdit   *cheatCmpEntry;
		QLineEdit   *knownValEntry;

	private:

   public slots:
      void closeWindow(void);
	private slots:

};
