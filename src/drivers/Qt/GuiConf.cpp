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
// PaletteConf.cpp
//
#include <QTextEdit>
#include <QApplication>
#include <QStyleFactory>
#include <QFileDialog>

#include "Qt/GuiConf.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"

//----------------------------------------------------
GuiConfDialog_t::GuiConfDialog_t(QWidget *parent)
	: QDialog(parent)
{
	int useNativeFileDialogVal;
	int useNativeMenuBarVal;
	int useCustomQssVal;
	QVBoxLayout *mainLayout, *vbox1, *vbox2;
	QHBoxLayout *hbox;
	QPushButton *closeButton, *button;
	QLabel      *lbl;
	QStringList  styleKeys;
	QString      selStyle;
	QGroupBox   *frame, *qssFrame;
	std::string  qssFile;

	//resize( 512, 600 );
	//printf("Style: %s \n", style()->objectName().toStdString().c_str() );

	selStyle = style()->objectName();

	// sync with config
	g_config->getOption("SDL.UseNativeFileDialog", &useNativeFileDialogVal);
	g_config->getOption("SDL.UseNativeMenuBar", &useNativeMenuBarVal);
	g_config->getOption("SDL.UseCustomQss", &useCustomQssVal);

	setWindowTitle(tr("GUI Config"));

	mainLayout = new QVBoxLayout();

	useNativeFileDialog = new QCheckBox(tr("Use Native OS File Dialog"));
	useNativeMenuBar = new QCheckBox(tr("Use Native OS Menu Bar"));

	useNativeFileDialog->setChecked(useNativeFileDialogVal);
	useNativeMenuBar->setChecked(useNativeMenuBarVal);

	connect(useNativeFileDialog, SIGNAL(stateChanged(int)), this, SLOT(useNativeFileDialogChanged(int)));
	connect(useNativeMenuBar, SIGNAL(stateChanged(int)), this, SLOT(useNativeMenuBarChanged(int)));

	styleComboBox = new QComboBox();

	styleKeys = QStyleFactory::keys();

	for (int i=0; i<styleKeys.size(); i++)
	{
		styleComboBox->addItem( styleKeys[i], i );

		if ( selStyle.compare( styleKeys[i], Qt::CaseInsensitive ) == 0 )
		{
			//printf("Style Match: %s \n", selStyle.toStdString().c_str() );
			styleComboBox->setCurrentIndex(i);
		}
	}
	connect(styleComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(styleChanged(int)));

	frame = new QGroupBox( tr("Style:") );
	vbox1 = new QVBoxLayout();
	vbox2 = new QVBoxLayout();
	hbox  = new QHBoxLayout();

	frame->setLayout( vbox1 );
	vbox1->addLayout( hbox );

	lbl  = new QLabel( tr("Base:") );

	hbox->addWidget( lbl, 1 );
	hbox->addWidget( styleComboBox, 10 );

	qssFrame = new QGroupBox( tr("QSS") );
	qssFrame->setLayout( vbox2 );

	useCustomStyle = new QCheckBox( tr("Use Custom Stylesheet") );
	useCustomStyle->setChecked(useCustomQssVal);
	connect(useCustomStyle, SIGNAL(stateChanged(int)), this, SLOT(useCustomStyleChanged(int)));

	vbox1->addWidget( qssFrame );
	vbox2->addWidget( useCustomStyle );

	hbox   = new QHBoxLayout();
	button = new QPushButton(tr("Open"));
	button->setIcon(style()->standardIcon(QStyle::SP_FileDialogStart));
	connect(button, SIGNAL(clicked(void)), this, SLOT(openQss(void)));
	hbox->addWidget(button);

	button = new QPushButton(tr("Clear"));
	button->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));
	connect(button, SIGNAL(clicked(void)), this, SLOT(clearQss(void)));
	hbox->addWidget(button);
	vbox2->addLayout( hbox );

	g_config->getOption("SDL.QtStyleSheet", &qssFile);

	custom_qss_path = new QLineEdit();
	custom_qss_path->setReadOnly(true);
	custom_qss_path->setText(qssFile.c_str());
	vbox2->addWidget( custom_qss_path );

	mainLayout->addWidget(useNativeFileDialog);
	mainLayout->addWidget(useNativeMenuBar);
	mainLayout->addWidget(frame);

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));


	hbox = new QHBoxLayout();
	hbox->addStretch(3);
	hbox->addWidget( closeButton, 3 );
	hbox->addStretch(3);
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);
}

//----------------------------------------------------
GuiConfDialog_t::~GuiConfDialog_t(void)
{
	printf("Destroy GUI Config Close Window\n");
}
//----------------------------------------------------------------------------
void GuiConfDialog_t::closeEvent(QCloseEvent *event)
{
	printf("GUI Config Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------
void GuiConfDialog_t::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------
void GuiConfDialog_t::useNativeFileDialogChanged(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	g_config->setOption("SDL.UseNativeFileDialog", value);
}
//----------------------------------------------------
void GuiConfDialog_t::useNativeMenuBarChanged(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	g_config->setOption("SDL.UseNativeMenuBar", value);

	consoleWindow->menuBar()->setNativeMenuBar(value);
}
//----------------------------------------------------
void GuiConfDialog_t::useCustomStyleChanged(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	g_config->setOption("SDL.UseCustomQss", value);

	if ( consoleWindow != NULL )
	{
		if ( value )
		{
			std::string s;
			g_config->getOption("SDL.QtStyleSheet", &s);
			loadQss( s.c_str() );
		}
		else
		{
			consoleWindow->setStyleSheet(NULL);
		}
	}
}
//----------------------------------------------------
void GuiConfDialog_t::styleChanged(int index)
{
	QString s;
	QStyle *sty;

	s = styleComboBox->currentText();

	//printf("Style: '%s'\n", s.toStdString().c_str() );

	sty = QStyleFactory::create( s );

	if ( sty != nullptr )
	{
		QApplication::setStyle(sty);

		g_config->setOption("SDL.GuiStyle", s.toStdString().c_str() );
		g_config->save();
	}
}
//----------------------------------------------------
void GuiConfDialog_t::clearQss(void)
{
	//g_config->setOption("SDL.Palette", "");
	custom_qss_path->setText("");

	g_config->setOption("SDL.QtStyleSheet", "");
	g_config->save();

	if ( consoleWindow != NULL )
	{
	      consoleWindow->setStyleSheet(NULL);
	}
}
//----------------------------------------------------
void GuiConfDialog_t::openQss(void)
{
	int ret, useNativeFileDialogVal, useCustom;
	QString filename;
	std::string last, iniPath;
	char dir[512];
	char exePath[512];
	QFileDialog dialog(this, tr("Open Qt Stylesheet (QSS)"));
	QList<QUrl> urls;
	QDir d;

	fceuExecutablePath(exePath, sizeof(exePath));

	//urls = dialog.sidebarUrls();
	urls << QUrl::fromLocalFile(QDir::rootPath());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QDir(FCEUI_GetBaseDirectory()).absolutePath());

	if (exePath[0] != 0)
	{
		d.setPath(QString(exePath) + "/../qss");

		if (d.exists())
		{
			urls << QUrl::fromLocalFile(d.absolutePath());
			iniPath = d.absolutePath().toStdString();
		}
	}
#ifdef WIN32

#else
	d.setPath("/usr/share/fceux/qss");

	if (d.exists())
	{
		urls << QUrl::fromLocalFile(d.absolutePath());
		iniPath = d.absolutePath().toStdString();
	}
#endif

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("Qt Stylesheets (*.qss *.QSS) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter(QDir::AllEntries | QDir::AllDirs | QDir::Hidden);
	dialog.setLabelText(QFileDialog::Accept, tr("Load"));

	g_config->getOption("SDL.QtStyleSheet", &last);

	if (last.size() == 0)
	{
		last.assign(iniPath);
	}

	getDirFromFile(last.c_str(), dir);

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

	custom_qss_path->setText(filename.toStdString().c_str());

	g_config->getOption("SDL.UseCustomQss", &useCustom);

	if ( useCustom )
	{
		loadQss( filename.toStdString().c_str() );
	}

	g_config->setOption("SDL.QtStyleSheet", filename.toStdString().c_str() );
	g_config->save();

	return;
}
//----------------------------------------------------
void GuiConfDialog_t::loadQss( const char *filepath )
{
	if ( consoleWindow != NULL )
	{
	   QFile File(filepath);
	
	   if ( File.open(QFile::ReadOnly) )
	   {
	      QString StyleSheet = QLatin1String(File.readAll());
	
	      consoleWindow->setStyleSheet(StyleSheet);
	
	      //printf("Using Qt Stylesheet file '%s'\n", filepath );
	   }
	   else
	   {
	      //printf("Warning: Could not open Qt Stylesheet file '%s'\n", filepath );
	   }
	}
}
//----------------------------------------------------
