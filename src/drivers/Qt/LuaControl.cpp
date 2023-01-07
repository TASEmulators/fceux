/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
// LuaControl.cpp
//
#include <stdio.h>
#include <string.h>
#include <list>

#ifdef WIN32
#include <Windows.h>
#endif

#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

#include "../../fceu.h"

#ifdef _S9XLUA_H
#include "../../fceulua.h"
#endif

#include "common/os_utils.h"

#include "Qt/LuaControl.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleUtilities.h"

static bool luaScriptRunning = false;
static bool updateLuaDisplay = false;
static bool openLuaKillMsgBox = false;
static int luaKillMsgBoxRetVal = 0;

struct luaConsoleOutputBuffer
{
	int head;
	int tail;
	int size;
	char *buf;

	luaConsoleOutputBuffer(void)
	{
		tail = head = 0;
		size = 4096;

		buf = (char *)malloc(size);
	}

	~luaConsoleOutputBuffer(void)
	{
		if (buf)
		{
			free(buf);
			buf = NULL;
		}
	}

	void addLine(const char *l)
	{
		int i = 0;
		//printf("Adding Line %i: '%s'\n", head, l );
		while (l[i] != 0)
		{
			buf[head] = l[i];
			i++;

			head = (head + 1) % size;

			if (head == tail)
			{
				tail = (tail + 1) % size;
			}
		}
	}

	void clear(void)
	{
		tail = head = 0;
	}
};

static luaConsoleOutputBuffer outBuf;

static std::list<LuaControlDialog_t *> winList;

static void updateLuaWindows(void);
//----------------------------------------------------
LuaControlDialog_t::LuaControlDialog_t(QWidget *parent)
	: QDialog(parent, Qt::Window)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *closeButton;
	QLabel *lbl;
	std::string filename;
	QSettings settings;

	resize(512, 512);

	setWindowTitle(tr("Lua Script Control"));

	mainLayout = new QVBoxLayout();

	lbl = new QLabel(tr("Script File:"));

	scriptPath = new QLineEdit();
	scriptArgs = new QLineEdit();

	g_config->getOption("SDL.LastLoadLua", &filename);

	scriptPath->setText( tr(filename.c_str()) );
	scriptPath->setClearButtonEnabled(true);
	scriptArgs->setClearButtonEnabled(true);

	luaOutput = new QTextEdit();
	luaOutput->setReadOnly(true);

	hbox = new QHBoxLayout();

	browseButton = new QPushButton(tr("Browse"));
	stopButton = new QPushButton(tr("Stop"));

	if (luaScriptRunning)
	{
		startButton = new QPushButton(tr("Restart"));
	}
	else
	{
		startButton = new QPushButton(tr("Start"));
	}

	stopButton->setEnabled(luaScriptRunning);

	connect(browseButton, SIGNAL(clicked()), this, SLOT(openLuaScriptFile(void)));
	connect(stopButton, SIGNAL(clicked()), this, SLOT(stopLuaScript(void)));
	connect(startButton, SIGNAL(clicked()), this, SLOT(startLuaScript(void)));

	hbox->addWidget(browseButton);
	hbox->addWidget(stopButton);
	hbox->addWidget(startButton);

	mainLayout->addWidget(lbl);
	mainLayout->addWidget(scriptPath);
	mainLayout->addLayout(hbox);

	hbox = new QHBoxLayout();
	lbl = new QLabel(tr("Arguments:"));

	hbox->addWidget(lbl);
	hbox->addWidget(scriptArgs);

	mainLayout->addLayout(hbox);

	lbl = new QLabel(tr("Output Console:"));
	mainLayout->addWidget(lbl);
	mainLayout->addWidget(luaOutput);

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);

	winList.push_back(this);

	periodicTimer = new QTimer(this);

	connect(periodicTimer, &QTimer::timeout, this, &LuaControlDialog_t::updatePeriodic);

	periodicTimer->start(200); // 5hz

	restoreGeometry(settings.value("LuaWindow/geometry").toByteArray());
}

//----------------------------------------------------
LuaControlDialog_t::~LuaControlDialog_t(void)
{
	QSettings settings;
	std::list<LuaControlDialog_t *>::iterator it;

	//printf("Destroy Lua Control Window\n");

	periodicTimer->stop();

	for (it = winList.begin(); it != winList.end(); it++)
	{
		if ((*it) == this)
		{
			winList.erase(it);
			//printf("Removing Lua Window\n");
			break;
		}
	}
	settings.setValue("LuaWindow/geometry", saveGeometry());
}
//----------------------------------------------------
void LuaControlDialog_t::closeEvent(QCloseEvent *event)
{
	//printf("Lua Control Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------
void LuaControlDialog_t::closeWindow(void)
{
	//printf("Lua Control Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------
void LuaControlDialog_t::updatePeriodic(void)
{
	//printf("Update Lua\n");
	if (updateLuaDisplay)
	{
		updateLuaWindows();
		updateLuaDisplay = false;
	}

	if (openLuaKillMsgBox)
	{
		openLuaKillMessageBox();
		openLuaKillMsgBox = false;
	}
}
//----------------------------------------------------
void LuaControlDialog_t::openLuaKillMessageBox(void)
{
	int ret;
	QMessageBox msgBox(this);

	luaKillMsgBoxRetVal = 0;

	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setText(tr("The Lua script running has been running a long time.\nIt may have gone crazy. Kill it? (I won't ask again if you say No)\n"));
	msgBox.setStandardButtons(QMessageBox::Yes);
	msgBox.addButton(QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::No);

	ret = msgBox.exec();

	if (ret == QMessageBox::Yes)
	{
		luaKillMsgBoxRetVal = 1;
	}
}
//----------------------------------------------------
void LuaControlDialog_t::openLuaScriptFile(void)
{
#ifdef _S9XLUA_H
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	char dir[2048];
	char exePath[2048];
	const char *luaPath;
	QFileDialog dialog(this, tr("Open LUA Script"));
	QList<QUrl> urls;
	QDir d;

	fceuExecutablePath(exePath, sizeof(exePath));

	//urls = dialog.sidebarUrls();
	urls << QUrl::fromLocalFile(QDir::rootPath());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());
	urls << QUrl::fromLocalFile(QDir(FCEUI_GetBaseDirectory()).absolutePath());

	if (exePath[0] != 0)
	{
		d.setPath(QString(exePath) + "/../luaScripts");

		if (d.exists())
		{
			urls << QUrl::fromLocalFile(d.absolutePath());
		}
	}
#ifndef WIN32
	d.setPath("/usr/share/fceux/luaScripts");

	if (d.exists())
	{
		urls << QUrl::fromLocalFile(d.absolutePath());
	}
#endif

	luaPath = getenv("LUA_PATH");

	// Parse LUA_PATH and add to urls
	if (luaPath)
	{
		int i, j;
		char stmp[2048];

		i = j = 0;
		while (luaPath[i] != 0)
		{
			if (luaPath[i] == ';')
			{
				stmp[j] = 0;

				if (j > 0)
				{
					d.setPath(stmp);

					if (d.exists())
					{
						urls << QUrl::fromLocalFile(d.absolutePath());
					}
				}
				j = 0;
			}
			else
			{
				stmp[j] = luaPath[i];
				j++;
			}
			i++;
		}

		stmp[j] = 0;

		if (j > 0)
		{
			d.setPath(stmp);

			if (d.exists())
			{
				urls << QUrl::fromLocalFile(d.absolutePath());
			}
		}
	}

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("LUA Scripts (*.lua *.LUA) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter(QDir::AllEntries | QDir::AllDirs | QDir::Hidden);
	dialog.setLabelText(QFileDialog::Accept, tr("Load"));

	g_config->getOption("SDL.LastLoadLua", &last);

	if (last.size() == 0)
	{
#ifdef WIN32
		last.assign(FCEUI_GetBaseDirectory());
#else
		last.assign("/usr/share/fceux/luaScripts");
#endif
	}

	getDirFromFile(last.c_str(), dir, sizeof(dir));

	dialog.setDirectory(tr(dir));

	// Check config option to use native file dialog or not
	g_config->getOption("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	ret = dialog.exec();

	if (ret)
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if (fileList.size() > 0)
		{
			filename = fileList[0];
		}
	}

	if (filename.isNull())
	{
		return;
	}
	qDebug() << "selected file path : " << filename.toUtf8();

	g_config->setOption("SDL.LastLoadLua", filename.toStdString().c_str());

	scriptPath->setText(filename);

#endif
}
//----------------------------------------------------
void LuaControlDialog_t::startLuaScript(void)
{
#ifdef _S9XLUA_H
	outBuf.clear();
	FCEU_WRAPPER_LOCK();
	if (0 == FCEU_LoadLuaCode(scriptPath->text().toStdString().c_str(), scriptArgs->text().toStdString().c_str()))
	{
		char error_msg[2048];
		sprintf( error_msg, "Error: Could not open the selected lua script: '%s'\n", scriptPath->text().toStdString().c_str());
		FCEUD_PrintError(error_msg);
	}
	FCEU_WRAPPER_UNLOCK();
#endif
}
//----------------------------------------------------
void LuaControlDialog_t::stopLuaScript(void)
{
#ifdef _S9XLUA_H
	FCEU_WRAPPER_LOCK();
	FCEU_LuaStop();
	FCEU_WRAPPER_UNLOCK();
#endif
}
//----------------------------------------------------
void LuaControlDialog_t::refreshState(void)
{
	int i;
	std::string luaOutputText;

	if (luaScriptRunning)
	{
		stopButton->setEnabled(true);
		startButton->setText(tr("Restart"));
	}
	else
	{
		stopButton->setEnabled(false);
		startButton->setText(tr("Start"));
	}

	i = outBuf.tail;

	while (i != outBuf.head)
	{
		luaOutputText.append(1, outBuf.buf[i]);

		i = (i + 1) % outBuf.size;
	}

	luaOutput->setText(luaOutputText.c_str());

	luaOutput->moveCursor(QTextCursor::End);
}
//----------------------------------------------------
static void updateLuaWindows(void)
{
	std::list<LuaControlDialog_t *>::iterator it;

	for (it = winList.begin(); it != winList.end(); it++)
	{
		(*it)->refreshState();
	}
}
//----------------------------------------------------
void WinLuaOnStart(intptr_t hDlgAsInt)
{
	luaScriptRunning = true;

	//printf("Lua Script Running: %i \n", luaScriptRunning );

	updateLuaDisplay = true;
}
//----------------------------------------------------
void WinLuaOnStop(intptr_t hDlgAsInt)
{
	luaScriptRunning = false;

	//printf("Lua Script Running: %i \n", luaScriptRunning );

	updateLuaDisplay = true;
}
//----------------------------------------------------
void PrintToWindowConsole(intptr_t hDlgAsInt, const char *str)
{
	//printf("%s\n", str );

	outBuf.addLine(str);

	updateLuaDisplay = true;
}
//----------------------------------------------------
#ifdef WIN32
int LuaPrintfToWindowConsole(_In_z_ _Printf_format_string_ const char *format, ...)
#else
int LuaPrintfToWindowConsole(const char *__restrict format, ...) throw()
#endif
{
	int retval;
	va_list args;
	char msg[2048];
	va_start(args, format);
	retval = ::vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);

	msg[sizeof(msg) - 1] = 0;

	outBuf.addLine(msg);

	updateLuaDisplay = true;

	return (retval);
};
//----------------------------------------------------
int LuaKillMessageBox(void)
{
	//printf("Kill Lua Prompted\n");
	luaKillMsgBoxRetVal = 0;

	openLuaKillMsgBox = true;

	while (openLuaKillMsgBox)
	{
		msleep(100);
	}

	return luaKillMsgBoxRetVal;
}
//----------------------------------------------------
