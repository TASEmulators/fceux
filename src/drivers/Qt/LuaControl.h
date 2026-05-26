// LuaControl.h
//

#pragma once

#include <stdio.h>
#include <stdarg.h>

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

	QTimer *periodicTimer;
	QLineEdit *scriptPath;
	QLineEdit *scriptArgs;
	QPushButton *browseButton;
	QPushButton *stopButton;
	QPushButton *startButton;
	QTextEdit *luaOutput;

private:
public slots:
	void closeWindow(void);
private slots:
	void updatePeriodic(void);
	void openLuaScriptFile(void);
	void startLuaScript(void);
	void stopLuaScript(void);
};

void WinLuaOnStart(intptr_t hDlgAsInt);

void WinLuaOnStop(intptr_t hDlgAsInt);

// Formatted print
int LuaPrintfToWindowConsole( __FCEU_PRINTF_FORMAT const char *format, ...) __FCEU_PRINTF_ATTRIBUTE( 1, 2 );

void PrintToWindowConsole(intptr_t hDlgAsInt, const char *str);

int LuaKillMessageBox(void);

bool GetLuaArgs(char *dst, int len) { dst[0] = NULL; return true; } // TODO: stub. scriptArgs->text().toLocal8Bit().constData() ?

// extern bool LuaArgCompat;
bool LuaArgCompat = false;
