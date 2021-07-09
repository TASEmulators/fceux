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
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QScrollArea>
#include <QSpinBox>
#include <QSlider>
#include <QDial>

#include "Qt/GuiConf.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"

static int writeQPaletteToFile( const char *path, QPalette *pal );
static int readQPaletteFromFile( const char *path, QPalette *pal );

static GuiPaletteEditDialog_t *editDialog = NULL;
//----------------------------------------------------
GuiConfDialog_t::GuiConfDialog_t(QWidget *parent)
	: QDialog(parent)
{
	int useNativeFileDialogVal;
	int useNativeMenuBarVal, pauseOnMenuAccessVal;
	int useCustomQssVal, useCustomQPalVal;
	QVBoxLayout *mainLayout, *vbox1, *vbox2;
	QHBoxLayout *hbox, *hbox1;
	QPushButton *closeButton, *button;
	QMenuBar    *menuBar;
	QMenu       *fileMenu, *colorMenu;
	QAction     *act;
	QLabel      *lbl;
	QStringList  styleKeys;
	QString      selStyle;
	QGroupBox   *frame, *qssFrame;
	QFrame      *hline;
	std::string  qssFile, qpalFile;

	//resize( 512, 600 );
	//printf("Style: %s \n", style()->objectName().toStdString().c_str() );

	selStyle = style()->objectName();

	// sync with config
	g_config->getOption("SDL.UseNativeFileDialog", &useNativeFileDialogVal);
	g_config->getOption("SDL.UseNativeMenuBar", &useNativeMenuBarVal);
	g_config->getOption("SDL.UseCustomQss", &useCustomQssVal);
	g_config->getOption("SDL.UseCustomQPal", &useCustomQPalVal);
	g_config->getOption("SDL.PauseOnMainMenuAccess", &pauseOnMenuAccessVal);

	setWindowTitle(tr("GUI Config"));

	menuBar = new QMenuBar(this);

	menuBar->setNativeMenuBar( useNativeMenuBarVal ? true : false );

	//-----------------------------------------------------------------------
	// Menu Start
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("&File"));

	// File -> Test Style
	act = new QAction(tr("&Test"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Test"));
	connect(act, SIGNAL(triggered()), this, SLOT(openTestWindow(void)) );
	
	fileMenu->addAction(act);

	// File -> Close
	act = new QAction(tr("&Close"), this);
	act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Close Window"));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );
	
	fileMenu->addAction(act);

	// Color
	colorMenu = menuBar->addMenu(tr("&Color"));

	// Color -> View QPalette
	act = new QAction(tr("&View QPalette"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("&View QPalette"));
	connect(act, SIGNAL(triggered()), this, SLOT(openQPalette(void)) );
	
	colorMenu->addAction(act);

	//-----------------------------------------------------------------------
	// Menu End
	//-----------------------------------------------------------------------
	
	mainLayout = new QVBoxLayout();

	mainLayout->setMenuBar( menuBar );

	useNativeFileDialog = new QCheckBox(tr("Use Native OS File Dialog"));
	useNativeMenuBar = new QCheckBox(tr("Use Native OS Menu Bar"));
	pauseOnMenuAccess = new QCheckBox(tr("Pause On Main Menu Access"));

	useNativeFileDialog->setChecked(useNativeFileDialogVal);
	useNativeMenuBar->setChecked(useNativeMenuBarVal);
	pauseOnMenuAccess->setChecked(pauseOnMenuAccessVal);

	connect(useNativeFileDialog, SIGNAL(stateChanged(int)), this, SLOT(useNativeFileDialogChanged(int)));
	connect(useNativeMenuBar, SIGNAL(stateChanged(int)), this, SLOT(useNativeMenuBarChanged(int)));
	connect(pauseOnMenuAccess, SIGNAL(stateChanged(int)), this, SLOT(pauseOnMenuAccessChanged(int)));

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
	vbox1->addWidget( qssFrame );

	useCustomPalette = new QCheckBox( tr("Use Custom QPalette") );
	useCustomPalette->setChecked(useCustomQPalVal);
	connect(useCustomPalette, SIGNAL(stateChanged(int)), this, SLOT(useCustomQPaletteChanged(int)));

	vbox2->addWidget( useCustomPalette );

	hbox   = new QHBoxLayout();
	button = new QPushButton(tr("Open"));
	button->setIcon(style()->standardIcon(QStyle::SP_FileDialogStart));
	connect(button, SIGNAL(clicked(void)), this, SLOT(openQPal(void)));
	hbox->addWidget(button);

	button = new QPushButton(tr("Clear"));
	button->setIcon(style()->standardIcon(QStyle::SP_LineEditClearButton));
	connect(button, SIGNAL(clicked(void)), this, SLOT(clearQPal(void)));
	hbox->addWidget(button);
	vbox2->addLayout( hbox );

	g_config->getOption("SDL.QPaletteFile", &qpalFile);

	custom_qpal_path = new QLineEdit();
	custom_qpal_path->setReadOnly(true);
	custom_qpal_path->setText(qpalFile.c_str());
	vbox2->addWidget( custom_qpal_path );

	hline = new QFrame(this);
	hline->setFrameShape(QFrame::HLine); 
	hline->setFrameShadow(QFrame::Sunken);
	hline->setLineWidth(2);
	vbox2->addWidget( hline );

	useCustomStyle = new QCheckBox( tr("Use Custom Stylesheet") );
	useCustomStyle->setChecked(useCustomQssVal);
	connect(useCustomStyle, SIGNAL(stateChanged(int)), this, SLOT(useCustomStyleChanged(int)));

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

	hbox1 = new QHBoxLayout();
	vbox1 = new QVBoxLayout();

	mainLayout->addLayout(hbox1);
	hbox1->addLayout(vbox1);
	hbox1->addWidget(frame);

	vbox1->addWidget(useNativeFileDialog, 1);
	vbox1->addWidget(useNativeMenuBar, 1);
	vbox1->addWidget(pauseOnMenuAccess, 1);
	vbox1->addStretch(10);

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
void GuiConfDialog_t::pauseOnMenuAccessChanged(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	g_config->setOption("SDL.PauseOnMainMenuAccess", value);

	consoleWindow->setMenuAccessPauseEnable( value );
}
//----------------------------------------------------
void GuiConfDialog_t::useCustomStyleChanged(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	g_config->setOption("SDL.UseCustomQss", value);

	QApplication::setStyle( new fceuStyle() );
}
//----------------------------------------------------
void GuiConfDialog_t::useCustomQPaletteChanged(int state)
{
	int value = (state == Qt::Unchecked) ? 0 : 1;

	g_config->setOption("SDL.UseCustomQPal", value);

	QApplication::setStyle( new fceuStyle() );
}
//----------------------------------------------------
void GuiConfDialog_t::styleChanged(int index)
{
	QString s;

	s = styleComboBox->currentText();

	g_config->setOption("SDL.GuiStyle", s.toStdString().c_str() );
	g_config->save();

	QApplication::setStyle( new fceuStyle() );
}
//----------------------------------------------------
void GuiConfDialog_t::clearQss(void)
{
	custom_qss_path->setText("");

	g_config->setOption("SDL.QtStyleSheet", "");
	g_config->save();

	QApplication::setStyle( new fceuStyle() );
}
//----------------------------------------------------
void GuiConfDialog_t::openQss(void)
{
	int ret, useNativeFileDialogVal; //, useCustom;
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

	g_config->setOption("SDL.QtStyleSheet", filename.toStdString().c_str() );
	g_config->save();

	QApplication::setStyle( new fceuStyle() );

	return;
}
//----------------------------------------------------
void GuiConfDialog_t::clearQPal(void)
{
	custom_qpal_path->setText("");

	g_config->setOption("SDL.QPaletteFile", "");
	g_config->save();

	QApplication::setStyle( new fceuStyle() );
}
//----------------------------------------------------
void GuiConfDialog_t::openQPal(void)
{
	int ret, useNativeFileDialogVal; //, useCustom;
	QString filename;
	std::string last, iniPath;
	char dir[512];
	char exePath[512];
	QFileDialog dialog(this, tr("Open Qt QPalette File (QPAL)"));
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

	dialog.setNameFilter(tr("Qt Stylesheets (*.qpal *.QPAL) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter(QDir::AllEntries | QDir::AllDirs | QDir::Hidden);
	dialog.setLabelText(QFileDialog::Accept, tr("Load"));

	g_config->getOption("SDL.QPaletteFile", &last);

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

	custom_qpal_path->setText(filename.toStdString().c_str());

	g_config->setOption("SDL.QPaletteFile", filename.toStdString().c_str() );
	g_config->save();

	QApplication::setStyle( new fceuStyle() );

	return;
}
//----------------------------------------------------
void GuiConfDialog_t::openQPalette(void)
{
	if ( editDialog )
	{
		return;
	}
	editDialog = new GuiPaletteEditDialog_t(this);

	editDialog->show();
}
//----------------------------------------------------
void GuiConfDialog_t::openTestWindow(void)
{
	guiStyleTestDialog *dialog = new guiStyleTestDialog(this);

	dialog->show();
}
//----------------------------------------------------
//---------------------------------------------------- 
// Custom Style Wrapper Class
//---------------------------------------------------- 
//---------------------------------------------------- 
fceuStyle::fceuStyle(void) : fceuStyle(styleBase()){}

fceuStyle::fceuStyle(QStyle *style) : QProxyStyle(style)
{
	//printf("New Style!!!\n");

	setObjectName( style->objectName() );
}

fceuStyle::~fceuStyle(void)
{
	//printf("Style Deleted: %s\n", objectName().toStdString().c_str() );

	if ( rccFilePath.size() > 0 )
	{
		if ( QResource::unregisterResource( QString::fromStdString(rccFilePath) ) )
		{
			//printf("Removed Resource: '%s'\n", rccFilePath.c_str() );
			rccFilePath.clear();
		}
	}
}

QStyle *fceuStyle::styleBase(QStyle *style) const
{
	std::string s;
	static QStyle *base;

	if ( g_config != NULL )
	{
		g_config->getOption("SDL.GuiStyle", &s );
	}

	if ( s.size() == 0 )
	{
		int i, idx = -1;
#ifdef WIN32
		QString defaultStyle("windows");
#elif __APPLE__
		QString defaultStyle("fusion");
#else
		QString defaultStyle("fusion");
#endif

		QStringList styleKeys = QStyleFactory::keys();

		for (i=0; i<styleKeys.size(); i++)
		{
			//printf("Style: '%s'\n", styleKeys[i].toStdString().c_str() );

			if ( defaultStyle.compare( styleKeys[i], Qt::CaseInsensitive ) == 0 )
			{
				//printf("Style Match: %s\n", styleKeys[i].toStdString().c_str() );
				idx = i;
				break;
			}
		}

		if ( (idx >= 0) && (idx < styleKeys.size()) )
		{
			s = styleKeys[idx].toStdString();
		}
		else
		{
			s.assign("fusion");
		}
	}

	if ( style == NULL )
	{
		base = QStyleFactory::create(QString::fromStdString(s));
	}
	else
	{
		base = style;
	}

	return base;
}

void fceuStyle::polish(QPalette &palette) 
{
	int useCustom;

	g_config->getOption("SDL.UseCustomQPal", &useCustom);
	//basePtr->polish(palette);
	//QStyle::polish(palette);
	
	// If the edit dialog is open, don't over write the palette
	if ( editDialog )
	{
		return;
	}

	palette = standardPalette();

	if ( !useCustom )
	{
		return;
	}
	std::string qPalFilePath;

	g_config->getOption("SDL.QPaletteFile", &qPalFilePath );

	if ( qPalFilePath.size() > 0 )
	{
		readQPaletteFromFile( qPalFilePath.c_str(), &palette );
	}

  // modify palette to dark
  //palette.setColor(QPalette::Window, QColor(53, 53, 53));
  //palette.setColor(QPalette::WindowText, Qt::white);
  //palette.setColor(QPalette::Disabled, QPalette::WindowText,
  //                 QColor(127, 127, 127));
  //palette.setColor(QPalette::Base, QColor(42, 42, 42));
  //palette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
  //palette.setColor(QPalette::ToolTipBase, Qt::white);
  //palette.setColor(QPalette::ToolTipText, QColor(53, 53, 53));
  //palette.setColor(QPalette::Text, Qt::white);
  //palette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
  //palette.setColor(QPalette::Dark, QColor(35, 35, 35));
  //palette.setColor(QPalette::Shadow, QColor(20, 20, 20));
  //palette.setColor(QPalette::Button, QColor(53, 53, 53));
  //palette.setColor(QPalette::ButtonText, Qt::white);
  //palette.setColor(QPalette::Disabled, QPalette::ButtonText,
  //                 QColor(127, 127, 127));
  //palette.setColor(QPalette::BrightText, Qt::red);
  //palette.setColor(QPalette::Link, QColor(42, 130, 218));
  //palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
  //palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
  //palette.setColor(QPalette::HighlightedText, Qt::white);
  //palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
  //                 QColor(127, 127, 127));
}

void fceuStyle::polish(QApplication *app) 
{
	int useCustom;
	std::string s;

	if (!app) return;

	//printf("Load Style Sheet!!!\n");
	// increase font size for better reading,
	// setPointSize was reduced from +2 because when applied this way in Qt5, the
	// font is larger than intended for some reason
	//QFont defaultFont = QApplication::font();
	//defaultFont.setPointSize(defaultFont.pointSize() + 1);
	//app->setFont(defaultFont);

	g_config->getOption("SDL.UseCustomQss", &useCustom);
	g_config->getOption("SDL.QtStyleSheet", &s);

	if ( useCustom && (s.size() > 0) )
	{
		// loadstylesheet
		QFile file( s.c_str() );

		if ( file.open(QIODevice::ReadOnly | QIODevice::Text) ) 
		{
			// set stylesheet
			QString qsStylesheet = QString::fromLatin1( file.readAll() );
			app->setStyleSheet(qsStylesheet);
			file.close();
		}

		if ( rccFilePath.size() == 0 )
		{
			char dir[1024], rccBase[256], tmpFile[2048];

			parseFilepath( s.c_str(), dir, rccBase, NULL );

			sprintf( tmpFile, "%s%s.rcc", dir, rccBase );

			//printf("RCC: '%s%s'\n", dir, rccBase );

			if ( QResource::registerResource( tmpFile ) )
			{
				//printf("Loaded RCC File: '%s'\n", tmpFile );
				rccFilePath.assign( tmpFile );
			}
		}
	}
	else
	{
		app->setStyleSheet(NULL);
	}
}
//----------------------------------------------------
//----------------------------------------------------
// QPalette Edit Dialog Window
//----------------------------------------------------
//----------------------------------------------------
static const char *getRoleText( QPalette::ColorRole role )
{
	const char *rTxt;

	switch ( role )
	{
		case QPalette::Window:
			rTxt = "Window";
		break;
		case QPalette::WindowText:
			rTxt = "WindowText";
		break;
		case QPalette::Base:
			rTxt = "Base";
		break;
		case QPalette::AlternateBase:
			rTxt = "AlternateBase";
		break;
		case QPalette::ToolTipBase:
			rTxt = "ToolTipBase";
		break;
		case QPalette::ToolTipText:
			rTxt = "ToolTipText";
		break;
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
		case QPalette::PlaceholderText:
			rTxt = "PlaceholderText";
		break;
#endif
		case QPalette::Text:
			rTxt = "Text";
		break;
		case QPalette::Button:
			rTxt = "Button";
		break;
		case QPalette::ButtonText:
			rTxt = "ButtonText";
		break;
		case QPalette::BrightText:
			rTxt = "BrightText";
		break;
		case QPalette::Light:
			rTxt = "Light";
		break;
		case QPalette::Midlight:
			rTxt = "Midlight";
		break;
		case QPalette::Mid:
			rTxt = "Mid";
		break;
		case QPalette::Dark:
			rTxt = "Dark";
		break;
		case QPalette::Shadow:
			rTxt = "Shadow";
		break;
		case QPalette::Highlight:
			rTxt = "Highlight";
		break;
		case QPalette::HighlightedText:
			rTxt = "HighlightedText";
		break;
		case QPalette::Link:
			rTxt = "Link";
		break;
		case QPalette::LinkVisited:
			rTxt = "LinkVisited";
		break;
		case QPalette::NoRole:
			rTxt = "NoRole";
		break;
		default:
			rTxt = NULL;
		break;
	}
	return rTxt;
}
//----------------------------------------------------
static int writeQPaletteToFile( const char *path, QPalette *pal )
{
	int i, j;
	QColor c;
	QPalette::ColorGroup g;
	QPalette::ColorRole  r;
	const char *gTxt, *rTxt;
	FILE *fp;

	fp = fopen( path, "w");

	if ( fp == NULL )
	{
		printf("Error: Failed to Open File for writing: '%s'\n", path );
		return -1;
	}

	for (j=0; j<3; j++)
	{

		if ( j == 1 )
		{
			g = QPalette::Disabled;
			gTxt = "Disabled";
		}
		else if ( j == 2 )
		{
			g = QPalette::Inactive;
			gTxt = "Inactive";
		}
		else
		{
			g = QPalette::Active;
			gTxt = "Active";
		}

		for (i=0; i<30; i++)
		{
			r = (QPalette::ColorRole)i;

			rTxt = getRoleText( r );

			if ( rTxt )
			{
				c = pal->color( g,  r );

				fprintf( fp, "%s::%s = %s \n", gTxt, rTxt, c.name().toStdString().c_str() );
			}
		}
	}

	fclose(fp);

	return 0;
}
//----------------------------------------------------
static int readQPaletteFromFile( const char *path, QPalette *pal )
{
	FILE *fp;
	char line[512];
	char gTxt[128], rTxt[256];
	char colorTxt[128];
	const char *rTxtMatch;
	int i,j,k;
	QColor color;
	QPalette::ColorGroup g;
	QPalette::ColorRole  r;

	fp = fopen( path, "r");

	if ( fp == NULL )
	{
		printf("Error: Failed to Open File for writing: '%s'\n", path );
		return -1;
	}
	while ( fgets( line, sizeof(line), fp ) != 0 )
	{
		//printf("%s", line );

		i=0;

		while ( isspace(line[i]) ) i++;

		j=0;
		while ( isalnum(line[i]) || (line[i] == '_') )
		{
			gTxt[j] = line[i]; j++; i++;
		}
		gTxt[j] = 0;

		if ( j == 0 ) continue;

		if (line[i] == ':')
		{
			while (line[i] == ':') i++;
		}

		j=0;
		while ( isalnum(line[i]) || (line[i] == '_'))
		{
			rTxt[j] = line[i]; j++; i++;
		}
		rTxt[j] = 0;

		if ( j == 0 ) continue;

		while ( isspace(line[i]) ) i++;

		if (line[i] == '=') i++;

		while ( isspace(line[i]) ) i++;

		if ( strcmp( gTxt, "Active") == 0 )
		{
			g = QPalette::Active;
		}
		else if ( strcmp( gTxt, "Inactive") == 0 )
		{
			g = QPalette::Inactive;
		}
		else if ( strcmp( gTxt, "Disabled") == 0 )
		{
			g = QPalette::Disabled;
		}
		else
		{
			continue;
		}

		rTxtMatch = NULL;

		for (k=0; k<30; k++)
		{

			rTxtMatch = getRoleText( (QPalette::ColorRole)k );

			if ( rTxtMatch )
			{
				if ( strcmp( rTxt, rTxtMatch ) == 0 )
				{
					r = (QPalette::ColorRole)k;
					break;
				}
			}
		}

		if ( rTxtMatch == NULL ) continue;

		color = pal->color( g, r );

		j=0;
		if (line[i] == '#')
		{
			colorTxt[j] = line[i]; j++; i++;

			while ( isxdigit( line[i] ) )
			{
				colorTxt[j] = line[i]; j++; i++;
			}
			colorTxt[j] = 0;

			color.setNamedColor( colorTxt );
		}

		//printf("Setting Color %s::%s = %s \n", gTxt, rTxt, colorTxt );
		pal->setColor( g, r, color );
	}


	fclose(fp);

	return 0;
}
//----------------------------------------------------
GuiPaletteEditDialog_t::GuiPaletteEditDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QScrollArea *scrollArea;
	QVBoxLayout *mainLayout;
	QVBoxLayout *vbox;
	QHBoxLayout *hbox1;
	QMenuBar    *menuBar;
	QMenu       *fileMenu;
	QAction     *act;
	GuiPaletteColorSelect *pcs;
	QPalette::ColorGroup g;
	QWidget *viewport;
	QFrame  *line;
	QLabel  *lbl;
	int useNativeMenuBarVal;

	setWindowTitle(tr("GUI Color Palette Edit"));

	g_config->getOption("SDL.UseNativeMenuBar", &useNativeMenuBarVal);

	menuBar = new QMenuBar(this);

	menuBar->setNativeMenuBar( useNativeMenuBarVal ? true : false );

	//-----------------------------------------------------------------------
	// Menu Start
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("&File"));

	// File -> Save As
	act = new QAction(tr("Save &As"), this);
	act->setShortcut(QKeySequence::SaveAs);
	act->setStatusTip(tr("Save As"));
	connect(act, SIGNAL(triggered()), this, SLOT(paletteSaveAs(void)) );
	
	fileMenu->addAction(act);

	// File -> Close
	act = new QAction(tr("&Close"), this);
	act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Close Window"));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );
	
	fileMenu->addAction(act);

	//-----------------------------------------------------------------------
	// Menu End
	//-----------------------------------------------------------------------

	scrollArea = new QScrollArea(this);
	viewport   = new QWidget(this);
	scrollArea->setWidget( viewport );
	scrollArea->setWidgetResizable(true);
	scrollArea->setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );
	scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );

	mainLayout = new QVBoxLayout(this);

	mainLayout->setMenuBar( menuBar );

	hbox1 = new QHBoxLayout(viewport);

	//viewport->setLayout( hbox1 );

	mainLayout->addWidget( scrollArea );

	for (int j=0; j<3; j++)
	{
		lbl = new QLabel();
		lbl->setAlignment( Qt::AlignCenter );

		if ( j == 1 )
		{
			g = QPalette::Disabled;
			lbl->setText( tr("Disabled") );
		}
		else if ( j == 2 )
		{
			g = QPalette::Inactive;
			lbl->setText( tr("Inactive") );
		}
		else
		{
			g = QPalette::Active;
			lbl->setText( tr("Active") );
		}
		line = new QFrame(this);
		line->setFrameShape(QFrame::VLine); // Vertical line
		line->setFrameShadow(QFrame::Sunken);
		line->setLineWidth(2);

		vbox = new QVBoxLayout();

		hbox1->addWidget( line );
		hbox1->addLayout( vbox );

		vbox->addWidget( lbl );

		for (int i=0; i<30; i++)
		{
			const char *rTxt = getRoleText( (QPalette::ColorRole)i );

			if ( rTxt )
			{
				pcs = new GuiPaletteColorSelect( g, (QPalette::ColorRole)i, this );

				vbox->addWidget( pcs );
			}
		}
	}

	setLayout( mainLayout );

}
//----------------------------------------------------
GuiPaletteEditDialog_t::~GuiPaletteEditDialog_t(void)
{
	if ( editDialog == this )
	{
		editDialog = NULL;
	}
}
//----------------------------------------------------
void GuiPaletteEditDialog_t::closeEvent(QCloseEvent *event)
{
	printf("GUI Palette Edit Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------
void GuiPaletteEditDialog_t::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------
void GuiPaletteEditDialog_t::paletteSaveAs(void)
{
	int ret, useNativeFileDialogVal; //, useCustom;
	QString filename;
	std::string last, iniPath;
	char dir[512];
	char exePath[512];
	QFileDialog dialog(this, tr("Save QPalette (qpal)"));
	QList<QUrl> urls;
	QDir d;
	QPalette pal = this->palette();

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

	dialog.setFileMode(QFileDialog::AnyFile);

	dialog.setNameFilter(tr("Qt QPalette Files (*.qpal *.QPAL) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter(QDir::AllEntries | QDir::AllDirs | QDir::Hidden);
	dialog.setLabelText(QFileDialog::Accept, tr("Save"));
	dialog.setDefaultSuffix( tr(".qpal") );

	g_config->getOption("SDL.QPaletteFile", &last);

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

	writeQPaletteToFile( filename.toStdString().c_str(), &pal );

	//g_config->setOption("SDL.QPaletteFile", filename.toStdString().c_str() );
	//g_config->save();

	//QApplication::setStyle( new fceuStyle() );

	return;

}
//----------------------------------------------------
//----------------------------------------------------
GuiPaletteColorSelect::GuiPaletteColorSelect( QPalette::ColorGroup group, QPalette::ColorRole role, QWidget *parent)
	: QWidget(parent)
{
	QHBoxLayout *hbox;
	QPushButton *editBox;

	hbox = new QHBoxLayout();

	setLayout(hbox);

	editBox = new QPushButton( tr("Edit") );

	lbl = new QLabel();
	cb  = new QCheckBox();

	lbl->setAlignment( Qt::AlignCenter );

	hbox->addWidget(editBox, 1);
	hbox->addWidget( cb,  1);
	hbox->addWidget(lbl, 10);

	this->group = group;
	this->role  = role;

	color = this->palette().color( group, role );

	setText();
	updateColor();

	connect( editBox, SIGNAL(clicked(void)), this, SLOT(colorEditClicked(void)) );

}
//----------------------------------------------------
GuiPaletteColorSelect::~GuiPaletteColorSelect(void)
{

}
//----------------------------------------------------
void GuiPaletteColorSelect::setText(void)
{
	const char *gTxt, *rTxt;
	char stmp[256];

	switch ( group )
	{
		case QPalette::Active:
			gTxt = "Active";
		break;
		case QPalette::Disabled:
			gTxt = "Disabled";
		break;
		case QPalette::Inactive:
			gTxt = "Inactive";
		break;
		default:
			gTxt = NULL;
		break;
	}

	rTxt = getRoleText( role );

	if ( (gTxt == NULL) || (rTxt == NULL) )
	{
		return;
	}
	sprintf( stmp, "%s :: %s", gTxt, rTxt );

	lbl->setText( tr(stmp) );

}
//----------------------------------------------------
void GuiPaletteColorSelect::updateColor(void)
{
	char stmp[256];
	QColor txtColor;

	if ( qGray( color.red(), color.green(), color.blue() ) > 128 )
	{
		txtColor.setRgb( 0, 0, 0 );
	}
	else
	{
		txtColor.setRgb( 255, 255, 255 );
	}
	sprintf( stmp, "QLabel { background-color : %s; color : %s; border-color : black; }",
		       color.name().toStdString().c_str(), txtColor.name().toStdString().c_str() );

	lbl->setStyleSheet( stmp );
}
//----------------------------------------------------
void GuiPaletteColorSelect::colorEditClicked(void)
{
	QString title;

	guiColorPickerDialog_t *editor = new guiColorPickerDialog_t( &color, this );

	title = QStringLiteral("Pick Palette Color for ") + lbl->text();

	editor->setWindowTitle( title );

	editor->show();
}
//----------------------------------------------------
void GuiPaletteColorSelect::updatePalette(void)
{
	QPalette pal;

	pal = this->palette();

	pal.setColor( group, role, color );

	QApplication::setPalette( pal );
}
//----------------------------------------------------
//----------------------------------------------------
// Color Picker
//----------------------------------------------------
//----------------------------------------------------
guiColorPickerDialog_t::guiColorPickerDialog_t( QColor *c, QWidget *parent )
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *okButton;
	QPushButton *cancelButton;
	QPushButton *resetButton;
	QStyle *style;

	style = this->style();

	setWindowTitle( "Pick Palette Color" );

	colorPtr = c;
	origColor = *c;

	mainLayout = new QVBoxLayout();

	setLayout( mainLayout );

	colorDialog = new QColorDialog(this);

	mainLayout->addWidget( colorDialog );

	colorDialog->setWindowFlags(Qt::Widget);
	colorDialog->setOption( QColorDialog::DontUseNativeDialog, true );
	colorDialog->setOption( QColorDialog::NoButtons, true );
	colorDialog->setCurrentColor( *c );
	
	connect( colorDialog, SIGNAL(colorSelected(const QColor &))      , this, SLOT(colorChanged( const QColor &)) );
	connect( colorDialog, SIGNAL(currentColorChanged(const QColor &)), this, SLOT(colorChanged( const QColor &)) );

	connect( colorDialog, SIGNAL(accepted(void)), this, SLOT(colorAccepted(void)) );
	connect( colorDialog, SIGNAL(rejected(void)), this, SLOT(colorRejected(void)) );

	hbox = new QHBoxLayout();
	mainLayout->addLayout( hbox );

	okButton     = new QPushButton( tr("OK") );
	cancelButton = new QPushButton( tr("Cancel") );
	resetButton  = new QPushButton( tr("Reset") );

	okButton->setIcon( style->standardIcon( QStyle::SP_DialogApplyButton ) );
	cancelButton->setIcon( style->standardIcon( QStyle::SP_DialogCancelButton ) );
	resetButton->setIcon( style->standardIcon( QStyle::SP_DialogResetButton ) );

	hbox->addWidget( resetButton, 1  );
	hbox->addStretch( 10 );
	hbox->addWidget( okButton, 1     );
	hbox->addWidget( cancelButton, 1 );

	connect( okButton    , SIGNAL(clicked(void)), this, SLOT(colorAccepted(void)) );
	connect( cancelButton, SIGNAL(clicked(void)), this, SLOT(colorRejected(void)) );
	connect( resetButton , SIGNAL(clicked(void)), this, SLOT(resetColor(void)) );
}
//----------------------------------------------------------------------------
guiColorPickerDialog_t::~guiColorPickerDialog_t(void)
{
	//printf("guiColorPicker Destroyed\n");
}
//----------------------------------------------------------------------------
void guiColorPickerDialog_t::closeEvent(QCloseEvent *event)
{
	//printf("guiColorPicker Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void guiColorPickerDialog_t::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void guiColorPickerDialog_t::colorChanged( const QColor &color )
{
	//printf("Color Changed: R:%i  G%i  B%i \n", color.red(), color.green(), color.blue() );

	*colorPtr = color;

	( (GuiPaletteColorSelect*)parent())->updateColor();
	( (GuiPaletteColorSelect*)parent())->updatePalette();
}
//----------------------------------------------------------------------------
void guiColorPickerDialog_t::colorAccepted(void)
{
	//printf("guiColorPicker Accepted\n");
	deleteLater();
}
//----------------------------------------------------------------------------
void guiColorPickerDialog_t::colorRejected(void)
{
	//printf("guiColorPicker Rejected\n");

	// Reset to original color
	*colorPtr = origColor;

	( (GuiPaletteColorSelect*)parent())->updateColor();
	( (GuiPaletteColorSelect*)parent())->updatePalette();

	deleteLater();
}
//----------------------------------------------------------------------------
void guiColorPickerDialog_t::resetColor(void)
{
	// Reset to original color
	*colorPtr = origColor;

	colorDialog->setCurrentColor( origColor );

	( (GuiPaletteColorSelect*)parent())->updateColor();
	( (GuiPaletteColorSelect*)parent())->updatePalette();
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// GUI Style Test Window
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
guiStyleTestDialog::guiStyleTestDialog(QWidget *parent)
	: QDialog( parent )
{
	QScrollArea *scrollArea;
	QVBoxLayout *mainLayout;
	QVBoxLayout *vbox, *vbox1, *vbox2;
	QHBoxLayout *hbox, *hbox1, *hbox2;
	QGridLayout *grid;
	QMenuBar    *menuBar;
	QMenu       *fileMenu;
	QAction     *act;
	QWidget *viewport;
	//QFrame  *line;
	QLabel  *lbl;
	QGroupBox *gbox;
	QPushButton *pushButton;
	QRadioButton *radioButton;
	QCheckBox *checkButton;
	QSlider *slider;
	QDial *dial;
	QLineEdit *lineEdit;
	QComboBox *comboBox;
	QSpinBox *spinBox;
	QTextEdit *textEdit;
	int useNativeMenuBarVal;

	setWindowTitle(tr("GUI Style Test Window"));

	g_config->getOption("SDL.UseNativeMenuBar", &useNativeMenuBarVal);

	menuBar = new QMenuBar(this);

	menuBar->setNativeMenuBar( useNativeMenuBarVal ? true : false );

	//-----------------------------------------------------------------------
	// Menu Start
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("&File"));

	// File -> Close
	act = new QAction(tr("&Close"), this);
	act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Close Window"));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );
	
	fileMenu->addAction(act);

	//-----------------------------------------------------------------------
	// Menu End
	//-----------------------------------------------------------------------

	scrollArea = new QScrollArea(this);
	viewport   = new QWidget(this);
	scrollArea->setWidget( viewport );
	scrollArea->setWidgetResizable(true);
	scrollArea->setSizeAdjustPolicy( QAbstractScrollArea::AdjustToContents );
	scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );

	mainLayout = new QVBoxLayout(this);
	vbox1      = new QVBoxLayout(viewport);
	hbox1      = new QHBoxLayout();

	vbox1->addLayout( hbox1 );

	mainLayout->setMenuBar( menuBar );

	mainLayout->addWidget( scrollArea );

	setLayout( mainLayout );

	vbox2 = new QVBoxLayout();
	hbox1->addLayout( vbox2 );

	gbox  = new QGroupBox( tr("Text") );
	grid  = new QGridLayout();
	vbox2->addWidget( gbox );
	gbox->setLayout( grid );

	lbl = new QLabel( tr("Label Enabled") );
	lbl->setEnabled(true);
	grid->addWidget( lbl, 0, 0 );

	lineEdit = new QLineEdit();
	lineEdit->setText( tr("Line Edit Enabled") );
	lineEdit->setEnabled(true);
	grid->addWidget( lineEdit, 0, 1 );

	lbl = new QLabel( tr("Label Disabled") );
	lbl->setEnabled(false);
	grid->addWidget( lbl, 1, 0 );

	lineEdit = new QLineEdit();
	lineEdit->setText( tr("Line Edit Disabled") );
	lineEdit->setEnabled(false);
	grid->addWidget( lineEdit, 1, 1 );

	lbl = new QLabel( tr("Label Link") );
	lbl->setEnabled(true);
	grid->addWidget( lbl, 2, 0 );

	lbl = new QLabel();
	lbl->setText("<a href=\"http://fceux.com\">Website Link</a>");
	lbl->setTextInteractionFlags(Qt::TextBrowserInteraction);
	lbl->setOpenExternalLinks(false);
	grid->addWidget( lbl, 2, 1 );

	lbl = new QLabel( tr("Label Sunken") );
	lbl->setEnabled(true);
	lbl->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	grid->addWidget( lbl, 3, 0 );

	lbl = new QLabel( tr("Label Raised") );
	lbl->setEnabled(true);
	lbl->setFrameStyle(QFrame::Panel | QFrame::Raised);
	grid->addWidget( lbl, 3, 1 );

	gbox  = new QGroupBox( tr("Button") );
	hbox2 = new QHBoxLayout();

	vbox2->addWidget( gbox );
	gbox->setLayout( hbox2 );

	pushButton = new QPushButton( tr("Normal") );
	pushButton->setEnabled(true);
	hbox2->addWidget( pushButton );

	pushButton = new QPushButton( tr("Disabled") );
	pushButton->setEnabled(false);
	hbox2->addWidget( pushButton );

	pushButton = new QPushButton( tr("Default") );
	pushButton->setEnabled(true);
	pushButton->setDefault(true);
	hbox2->addWidget( pushButton );

	gbox  = new QGroupBox( tr("Checkable") );
	gbox->setCheckable(true);
	grid  = new QGridLayout();

	vbox2->addWidget( gbox );
	gbox->setLayout( grid );

	radioButton = new QRadioButton( tr("Radio1") );
	radioButton->setEnabled(true);
	radioButton->setChecked(true);
	grid->addWidget( radioButton, 0, 0 );
	
	radioButton = new QRadioButton( tr("Radio2") );
	radioButton->setEnabled(true);
	radioButton->setChecked(false);
	grid->addWidget( radioButton, 0, 1 );
	
	radioButton = new QRadioButton( tr("Disabled") );
	radioButton->setEnabled(false);
	radioButton->setChecked(false);
	grid->addWidget( radioButton, 0, 2 );
	
	checkButton = new QCheckBox( tr("Checkbox") );
	checkButton->setEnabled(true);
	checkButton->setChecked(true);
	grid->addWidget( checkButton, 1, 0 );
	
	checkButton = new QCheckBox( tr("TriState") );
	checkButton->setEnabled(true);
	checkButton->setTristate(true);
	checkButton->setCheckState(Qt::PartiallyChecked);
	grid->addWidget( checkButton, 1, 1 );
	
	checkButton = new QCheckBox( tr("Disabled") );
	checkButton->setEnabled(false);
	checkButton->setChecked(true);
	grid->addWidget( checkButton, 1, 2 );
	
	vbox   = new QVBoxLayout();
	hbox   = new QHBoxLayout();
	hbox1->addLayout( vbox );

	slider = new QSlider(Qt::Horizontal);
	slider->setMinimum(-20);
	slider->setMaximum( 20);
	vbox->addWidget( slider );

	slider = new QSlider(Qt::Horizontal);
	slider->setEnabled(false);
	slider->setMinimum(-20);
	slider->setMaximum( 20);
	vbox->addWidget( slider );

	vbox->addLayout( hbox );
	slider = new QSlider(Qt::Vertical);
	slider->setMinimum(-20);
	slider->setMaximum( 20);
	hbox->addWidget( slider );

	slider = new QSlider(Qt::Vertical);
	slider->setEnabled(false);
	slider->setMinimum(-20);
	slider->setMaximum( 20);
	hbox->addWidget( slider );

	dial = new QDial();
	dial->setNotchesVisible(true);
	hbox->addWidget( dial );

	textEdit = new QTextEdit();
	textEdit->setText( tr("This is a text edit") );
	hbox->addWidget( textEdit );

	gbox  = new QGroupBox( tr("Selectable") );
	hbox2 = new QHBoxLayout();
	gbox->setLayout( hbox2 );

	vbox1->addWidget( gbox );
	comboBox = new QComboBox();
	comboBox->addItem( tr("AAA"), 0 );
	comboBox->addItem( tr("BBB"), 1 );
	comboBox->addItem( tr("CCC"), 2 );
	comboBox->setEnabled(true);
	hbox2->addWidget( comboBox );

	comboBox = new QComboBox();
	comboBox->addItem( tr("AAA"), 0 );
	comboBox->addItem( tr("BBB"), 1 );
	comboBox->addItem( tr("CCC"), 2 );
	comboBox->setEnabled(false);
	hbox2->addWidget( comboBox );

	spinBox = new QSpinBox();
	spinBox->setEnabled(true);
	hbox2->addWidget( spinBox );

	spinBox = new QSpinBox();
	spinBox->setEnabled(false);
	hbox2->addWidget( spinBox );

}
//----------------------------------------------------------------------------
guiStyleTestDialog::~guiStyleTestDialog(void)
{

}
//----------------------------------------------------------------------------
void guiStyleTestDialog::closeEvent(QCloseEvent *event)
{
	//printf("GUI Config Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------
void guiStyleTestDialog::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------
