// GameGenie.h
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
#include <QLineEdit>
#include <QTreeView>
#include <QTreeWidget>

#include "Qt/main.h"
#include "Qt/ConsoleUtilities.h"

class GameGenieDialog_t : public QDialog
{
   Q_OBJECT

	public:
		GameGenieDialog_t(QWidget *parent = 0);
		~GameGenieDialog_t(void);

	protected:
		void closeEvent(QCloseEvent *event);

		fceuHexIntValidtor *addrValidator;
		fceuHexIntValidtor *cmpValidator;
		fceuHexIntValidtor *valValidator;

		QLineEdit   *addr;
		QLineEdit   *cmp;
		QLineEdit   *val;
		QLineEdit   *ggCode;
		QPushButton *addCheatBtn;
		QTreeWidget *tree;

	private:
		void ListGGAddresses(void);

   public slots:
      void closeWindow(void);
	private slots:
		void addCheatClicked(void);
		void addrChanged(const QString &);
		void cmpChanged(const QString &);
		void valChanged(const QString &);
		void ggChanged(const QString &);
		void romAddrDoubleClicked(QTreeWidgetItem *item, int column);

};

void EncodeGG(char *str, int a, int v, int c);
