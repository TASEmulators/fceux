// LuaControl.h
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
#include <QTextEdit>

#include "Qt/main.h"

class LuaControlDialog_t : public QDialog
{
   Q_OBJECT

	public:
		LuaControlDialog_t(QWidget *parent = 0);
		~LuaControlDialog_t(void);

		void refreshState(void);

	protected:
		void closeEvent(QCloseEvent *bar);

		QLineEdit   *scriptPath;
		QLineEdit   *scriptArgs;
		QPushButton *browseButton;
		QPushButton *stopButton;
		QPushButton *startButton;
		QTextEdit   *luaOutput;
	private:

   public slots:
      void closeWindow(void);
	private slots:
		void openLuaScriptFile(void);
		void startLuaScript(void);
		void stopLuaScript(void);

};
