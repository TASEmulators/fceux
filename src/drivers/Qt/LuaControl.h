// LuaControl.h
//

#pragma once

#include <stdio.h>

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
		void openLuaKillMessageBox(void);

      QTimer      *periodicTimer;
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
      void updatePeriodic(void);
		void openLuaScriptFile(void);
		void startLuaScript(void);
		void stopLuaScript(void);

};

// Formatted print
#ifdef  WIN32
	int LuaPrintfToWindowConsole(_In_z_ _Printf_format_string_ const char* const format, ...) ;
#elif  __linux__ 
	#ifdef __THROWNL
	int LuaPrintfToWindowConsole(const char *__restrict format, ...) 
		__THROWNL __attribute__ ((__format__ (__printf__, 1, 2)));
	#else
	int LuaPrintfToWindowConsole(const char *__restrict format, ...) 
		throw() __attribute__ ((__format__ (__printf__, 1, 2)));
	#endif
#else 
	int LuaPrintfToWindowConsole(const char *__restrict format, ...) throw();
#endif

void PrintToWindowConsole(intptr_t hDlgAsInt, const char* str);

int LuaKillMessageBox(void);
