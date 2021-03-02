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

#include "Qt/GuiConf.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"

//----------------------------------------------------
GuiConfDialog_t::GuiConfDialog_t(QWidget *parent)
	: QDialog(parent)
{
	int useNativeFileDialogVal;
	int useNativeMenuBarVal;
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *closeButton;
	QLabel      *lbl;
	QStringList  styleKeys;
	QString      selStyle;

	//resize( 512, 600 );
	//printf("Style: %s \n", style()->objectName().toStdString().c_str() );

	selStyle = style()->objectName();

	// sync with config
	g_config->getOption("SDL.UseNativeFileDialog", &useNativeFileDialogVal);
	g_config->getOption("SDL.UseNativeMenuBar", &useNativeMenuBarVal);

	setWindowTitle(tr("GUI Config"));

	mainLayout = new QVBoxLayout();

	useNativeFileDialog = new QCheckBox(tr("Use Native OS File Dialog"));
	useNativeMenuBar = new QCheckBox(tr("Use Native OS Menu Bar"));

	useNativeFileDialog->setChecked(useNativeFileDialogVal);
	useNativeMenuBar->setChecked(useNativeMenuBarVal);

	connect(useNativeFileDialog, SIGNAL(stateChanged(int)), this, SLOT(useNativeFileDialogChanged(int)));
	connect(useNativeMenuBar, SIGNAL(stateChanged(int)), this, SLOT(useNativeMenuBarChanged(int)));

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("Style:") );

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

	hbox->addWidget( lbl, 1 );
	hbox->addWidget( styleComboBox, 10 );

	mainLayout->addLayout(hbox);
	mainLayout->addWidget(useNativeFileDialog);
	mainLayout->addWidget(useNativeMenuBar);

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
