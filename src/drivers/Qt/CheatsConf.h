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
#include <QTextEdit>

#include "Qt/main.h"

class GuiCheatsDialog_t : public QDialog
{
   Q_OBJECT

	public:
		GuiCheatsDialog_t(QWidget *parent = 0);
		~GuiCheatsDialog_t(void);

		int addSearchResult( uint32_t a, uint8_t last, uint8_t current );

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
		QPushButton *eqValBtn;
		QPushButton *neValBtn;
		QPushButton *grValBtn;
		QPushButton *ltValBtn;
		QCheckBox   *useNeVal;
		QCheckBox   *useGrVal;
		QCheckBox   *useLtVal;
		QTreeWidget *tree;
		QTreeWidget *srchResults;
		QLineEdit   *cheatNameEntry;
		QLineEdit   *cheatAddrEntry;
		QLineEdit   *cheatValEntry;
		QLineEdit   *cheatCmpEntry;
		QLineEdit   *knownValEntry;
		QLineEdit   *neValEntry;
		QLineEdit   *grValEntry;
		QLineEdit   *ltValEntry;

		int cheat_search_known_value;

	private:
		void showCheatSearchResults(void);

   public slots:
      void closeWindow(void);
	private slots:
      void resetSearchCallback(void);
      void knownValueCallback(void);

};
